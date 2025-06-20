// 集群聊天服务器客户端实现
#include <iostream>
#include <thread>
#include <string>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <functional>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <atomic>
#include "json.hpp"

#include "user.hpp"
#include "public.hpp"
#include "group.hpp"

using namespace std;
using json = nlohmann::json;

// 记录当前系统登录的用户信息
User g_currentUser;
// 记录当前系统登录的用户的好友列表
vector<User> g_currentUserFriends;
// 记录当前系统登录的用户的群组列表
vector<Group> g_currentUserGroups;

// ******************** 函数声明 ********************
// 显示当前登录的用户的基本信息
void showCurrentUserData();

// 接收线程 ----  一共两个线程【多线程模型】,接收和发送是并行的  ---- main 主线程用于发送
void readTaskHandler(int clientSocket);
// 获取系统时间(聊天信息添加事件信息)
string getCurrentTime();
// 主聊天页面程序
void mainMenu(int clientSocket);

// 是否在主聊天页面
bool isMainMenuRunning = false;

// 定义信号量用于读写线程之间的通信
sem_t rwsem;

// 记录登录状态
atomic_bool g_isLoginSuccess{false};

// 聊天客户端程序实现，main线程用做发送线程,子线程用作接收线程
int main(int argc, char**argv)
{
    // 命令行参数检查
    if(argc < 3)
    {
        cerr << "Command invalid! Usage: " << argv[0] << " <ip> <port>" << endl;
        exit(-1);
    }
    // 解析通过命令行参数传入的服务器IP和端口
    char* ip = argv[1]; // 服务器IP
    int port = atoi(argv[2]); // 服务器端口

    // 创建套接字(socket)
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(clientSocket < 0)
    {
        cerr << "Create socket failed! " << strerror(errno) << endl;
        exit(-1);
    }

    // 设置服务器地址结构体
    sockaddr_in serverAddr;
    // 清空结构体，确保没有脏数据
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;  // 地址族
    serverAddr.sin_port = htons(port); // 端口号，转换为网络字节序
    // 将IP地址从字符串转换为二进制格式
    // Note: 在网络编程中inet_pton传入的参数是&serverAddr.sin_addr.s_addr,这两种方式都是可以的
    // 但是更推荐采用&serverAddr.sin_addr,因为它更符合现代C++的风格，且更易于理解和维护。
    if(inet_pton(AF_INET, ip, &serverAddr.sin_addr) <= 0)
    {
        cerr << "Invalid address or address not supported! " << strerror(errno) << endl;
        close(clientSocket);
        exit(-1);
    }
    // 执行connect连接
    if(connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        cerr << "Connect to server failed! " << strerror(errno) << endl;
        close(clientSocket);
        exit(-1);
    }
    // 初始化用于接收线程与读取线程之间的信号量
    sem_init(&rwsem, 0, 0); // 初始化信号量, 初始值为0
    // 创建接收线程，并设置线程分离 --> 不阻塞主线程
    thread readTask(readTaskHandler, clientSocket);
    readTask.detach(); // 分离线程, 使其独立运行
    // main线程用于发送数据
    for(;;)
    {
        // 显示登录首页面, 登录, 注册, 退出
        cout << "==================== Chat Client ====================" << endl;
        cout << "1. Login" << endl;
        cout << "2. Register" << endl;
        cout << "3. Exit" << endl;
        cout << "=====================================================" << endl;
        cout << "Please choose an option: ";
        int choice;
        cin >> choice;
        cin.get();   // 清空输入缓冲区
        switch(choice)
        {
            // 处理登录业务
            case 1:
            {
                int id = 0;   // 登录需要用户id
                char password[50];
                cout << "Please enter your id: ";
                cin >> id;
                cin.get();   // 清空输入缓冲区
                cout << "Please enter your password: ";
                cin.getline(password, sizeof(password)); // 获取密码

                // 根据用户信息组装 Json 数据
                json js;
                js["msgid"] = LOGIN_MSG;   // 登录消息
                js["id"] = id;   
                js["password"] = password; // 密码

                // 将 Json 数据 ”序列化“ 为字符串
                string request = js.dump(); // 序列化为字符串

                g_isLoginSuccess = false;

                // 发送登录请求
                int len = send(clientSocket, request.c_str(), strlen(request.c_str()) + 1, 0);
                if(len < 0)
                {
                    cerr << "Send request failed! " << request << endl;
                    close(clientSocket);
                    exit(-1);
                }

                sem_wait(&rwsem);  // 等待接收线程接受完数据,并处理完登录的响应消息之后通知这里

                if(g_isLoginSuccess)
                {
                    // 进入聊天主菜单页面
                    isMainMenuRunning = true; // 设置主菜单运行状态为true
                    cout << "Login success! Welcome to the chat system!" << endl;
                    mainMenu(clientSocket);
                }
            }
            break;
            // 处理注册业务
            case 2:
            {
                char name[50];   // 比string 更好,因为string需要动态分配内存,还可以限制长度
                char password[50];
                cout << "Please enter your name: ";
                cin.getline(name, sizeof(name)); // 获取用户名, 读取一行包括空格, cin 和 scanf 不能读空格
                cout << "Please enter your password: ";
                cin.getline(password, sizeof(password)); // 获取密码

                // 根据用户信息组装 Json 数据
                json js;
                js["msgid"] = REG_MSG;   // 注册消息
                js["name"] = name;       // 用户名
                js["password"] = password; // 密码

                // 将 Json 数据 ”序列化“ 为字符串
                string request = js.dump(); // 序列化为字符串
                // 发送注册请求
                // 这里的send函数需要注意, 因为是发送字符串, 所以需要加上'\0'结尾符
                int len = send(clientSocket, request.c_str(), strlen(request.c_str()) + 1, 0);
                if(len < 0)
                {
                    cerr << "Send request failed! " << request << endl;
                    close(clientSocket);
                    exit(-1);
                }
                
                sem_wait(&rwsem);  // 等待接收线程接受完数据,并处理完注册的响应消息之后通知这里
                
            }
            break;   // 退出当前的switch语句
            // 处理退出业务
            case 3:
            {
                cout << "Exit system..." << endl;
                close(clientSocket); // 关闭套接字
                sem_destroy(&rwsem); // 销毁信号量
                exit(0);   // 退出程序
            }
            default:
            {
                cout << "Input Error! Please try again." << endl;
                break; // 继续循环,重新显示菜单
            }
        }
    }
}

void doRegResponse(json &response)
{
    if(response["erron"].get<int>() != 0)  // 注册失败
    {
        cerr << response["errmsg"] << endl;
    }else{
        // 注册成功, 显示注册成功消息
        cout << "[INFO] Registration Success! Your ID is: " << response["id"].get<int>() << ", do not forget it!" << endl;
    }
}

void doLoginResponse(json &response)
{
    if(response["erron"].get<int>() != 0)  // 登录失败
    {
        cerr << response["errmsg"] << endl;
        g_isLoginSuccess = false;
    }else{
        // 按照业务代码【chatservice.cpp的login部分】进行处理
        // 1. 显示登录成功消息
        cout << "[INFO] Login Success!" << endl;
        // 客户端记录登录用户的信息, 不需要设置 password 和 state(已经由服务器端设置了)
        g_currentUser.setId(response["id"].get<int>()); // 设置当前用户id
        g_currentUser.setName(response["name"]); // 设置当前用户名
        // *************************** 2.好友列表 ******************
        if(response.contains("friends"))  // 检查响应消息中是否包含 "friends" 字段
        {
            // 初始化
            g_currentUserFriends.clear();

            vector<string> friends = response["friends"]; // 获取好友列表
            g_currentUserFriends.clear(); // 清空好友列表
            for(auto &frienduser : friends)
            {
                // 类型是vector<string>, 不是vector<User>, 根据服务器业务,存的是js.dump() 字符串
                json friendJson = json::parse(frienduser); // 反序列化, 解析好友信息
                User user; // 创建用户对象
                user.setId(friendJson["id"].get<int>()); // 设置用户id
                user.setName(friendJson["name"]); // 设置用户名
                user.setState(friendJson["state"]); // 设置用户状态
                g_currentUserFriends.push_back(user); // 添加到好友列表中
            }
        }
        // *************************** 3.群组列表 ******************
        if(response.contains("groups"))
        {
            // 初始化
            g_currentUserGroups.clear();

            vector<string> groups = response["groups"]; // 获取群组列表
            g_currentUserGroups.clear(); // 清空群组列表
            for(auto group : groups)
            {
                // 类型是vector<string>, 不是vector<Group>,  根据服务器业务,存的是js.dump() 字符串
                json groupJson = json::parse(group); // 反序列化, 解析群组信息
                Group g; // 创建群组对象
                g.setId(groupJson["id"].get<int>()); // 设置群组id
                g.setName(groupJson["groupname"]); // 设置群组名称
                g.setDesc(groupJson["groupdesc"]); // 设置群组描述
                // 获取群组用户列表
                vector<string> users = groupJson["users"];
                for(auto &user : users)
                {
                    json userJson = json::parse(user); // 解析用户信息
                    GroupUser u; // 创建群组用户对象
                    u.setId(userJson["id"].get<int>()); // 设置用户id
                    u.setName(userJson["name"]); // 设置用户名
                    u.setState(userJson["state"]); // 设置用户状态
                    u.setRole(userJson["role"]); // 设置用户角色
                    g.getUsers().push_back(u); // 添加到群组用户列表中
                }
                g_currentUserGroups.push_back(g); // 添加到群组列表中
            }
        }

        // 显示当前登录用户的基本信息---包含好友列表和群组列表
        showCurrentUserData();

        // *************************** 4.显示离线消息 ******************
        if(response.contains("offlinemsgs"))
        {
            vector<string> offlineMsgs = response["offlinemsgs"]; // 获取离线消息
            if(!offlineMsgs.empty())
            {
                cout << "You have " << offlineMsgs.size() << " offline messages:" << endl;
                for(auto &msg : offlineMsgs)
                {
                    json msgJson = json::parse(msg); // 解析离线消息
                    // 离线消息也区分群消息与一对一消息,分消息类别进行输出
                    if(msgJson["msgid"].get<int>() == ONE_CHAT_MSG)
                    {
                        cout << msgJson["time"].get<string>() << " [" 
                             << msgJson["id"].get<int>() << "] " 
                             << msgJson["name"].get<string>() << " said: " 
                             << msgJson["msg"].get<string>() << endl;
                    }
                    else if(msgJson["msgid"].get<int>() == GROUP_CHAT_MSG)
                    {
                        cout << "群消息" << msgJson["time"].get<string>() << " [" 
                             << msgJson["groupid"].get<int>() << "] " 
                             << msgJson["name"].get<string>() << " said: " 
                             << msgJson["msg"].get<string>() << endl;
                    }
                }
            }
        }
        g_isLoginSuccess = true; // 设置登录成功状态为true
    }
}

// 用户信息显示代码实现
void showCurrentUserData()
{
    cout << "==================== Login user ====================" << endl;
    cout << "User ID: " << g_currentUser.getId() << " User Name: " << g_currentUser.getName() << endl;
    cout << "-------------------- Friend list --------------------" << endl;
    if(!g_currentUserFriends.empty())
    {
        for(auto &frienduser : g_currentUserFriends)
        {
            cout << "Friend ID: " << frienduser.getId() << ", Name: " << frienduser.getName() 
                 << ", State: " << frienduser.getState() << endl;
        }
    }else{
        cout << "Your friend list is empty!" << endl;
    }
    cout << "-------------------- Group list --------------------" << endl;
    if(!g_currentUserGroups.empty())
    {
        for(auto &group : g_currentUserGroups)
        {
            cout << "Group ID: " << group.getId() << ", GroupName: " << group.getName() 
                 << ", GroupDescription: " << group.getDesc() << endl;
            cout << "Group Users: " << endl;
            for(auto &user : group.getUsers())
            {
                cout << "User ID: " << user.getId() << ", Name: " << user.getName() 
                     << ", State: " << user.getState() << ", Role: " << user.getRole() << endl;
            }
        }
    }else{
        cout << "You are not in any group!" << endl;
    }
    cout << "=====================================================" << endl;
}

// 接收线程 ----  一共两个线程【多线程模型】,接收和发送是并行的  ---- main 主线程用于发送
// 持续监听服务器发送到客户端的消息
void readTaskHandler(int clientSocket)
{
    for(;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientSocket, buffer, 1024, 0);  // 阻塞接收数据
        if(len == -1 || len == 0)
        {
            // 连接异常或者关闭
            cerr << "Connection closed or error occurred! " << strerror(errno) << endl;
            close(clientSocket); // 关闭套接字
            exit(-1);   // 退出程序
        }
        // 解析chatserver转发的数据，反序列化生成json数据对象
        json js = json::parse(buffer); // 反序列化, 将字符串转为json对象
        // 根据消息 id 进行相应的处理
        int msg_type = js["msgid"].get<int>();
        if(msg_type == ONE_CHAT_MSG)
        {
            cout << js["time"].get<string>() << " [" 
                 << js["id"].get<int>() << "] " 
                 << js["name"].get<string>() << " said: " 
                 << js["msg"].get<string>() << endl;
            continue;
        }
        if(msg_type == GROUP_CHAT_MSG)
        {
            cout << "群消息" << js["time"].get<string>() << " [" 
                 << js["groupid"].get<int>() << "] " 
                 << js["name"].get<string>() << " said: " 
                 << js["msg"].get<string>() << endl;
            continue;
        }
        if(msg_type == LOGIN_MSG_ACK)
        {
            // 登录响应消息
            doLoginResponse(js); // 处理登录响应
            sem_post(&rwsem); // 通知主线程登录响应接收并处理完成
            continue;
        }
        if(msg_type == REG_MSG_ACK)
        {
            doRegResponse(js);
            sem_post(&rwsem);  // 通知主线程注册响应接收并处理完成
            continue;
        }
    }
}

// handler合集
// 显示帮助信息
void help(int clientfd = 0, string msg = "");

// 一对一聊天
void chat(int clientfd, string msg);

// 添加好友
void addfriend(int clientfd, string msg);

// 创建群组
void creategroup(int clientfd, string msg);

// 添加群组
void addgroup(int clientfd, string msg);

// 群组聊天
void groupchat(int clientfd, string msg);
// 退出系统
void logout(int clientfd, string msg);

// 获取系统时间(聊天信息添加事件信息)
string getCurrentTime()
{
    // 获取当前时间
    time_t now = time(0); // 获取当前时间戳
    tm *ltm = localtime(&now); // 将时间戳转换为本地时间
    char buffer[80];
    // 格式化时间字符串
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", ltm);
    return string(buffer); // 返回格式化后的时间字符串
}

// 系统支持的客户端命令列表
unordered_map<string, string> commandMap = {
    {"help", "显示所有支持的命令, 格式 help"},
    {"chat", "一对一聊天, 格式 chat:friend:msg"},
    {"addfriend", "添加好友, 格式 addfriend:friendid"},
    {"creategroup", "创建群组, 格式 creategroup:groupname:groupdesc"},
    {"addgroup", "添加群组, 格式 addgroup:groupid"},
    {"groupchat", "群组聊天, 格式 groupchat:groupid:msg"},
    {"logout/quit", "退出系统/注销, 格式 logout"}
};

// 注册系统支持的客户端命令处理
// function<void(int, string)> 可以理解成函数对象类型
unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat",chat},  // 一对一聊天
    {"addfriend", addfriend}, // 添加朋友
    {"creategroup", creategroup}, // 创建群组
    {"addgroup", addgroup}, // 添加群组
    {"groupchat", groupchat}, // 群组聊天
    {"logout", logout}
};

// 主聊天页面程序
void mainMenu(int clientSocket)
{
    help();
    char buffer[1024] = {0}; // 用于接收用户输入的命令
    while(isMainMenuRunning)
    {
        cin.getline(buffer, 1024);
        string commandbuf(buffer); // 将输入的命令转换为字符串
        string command;    // 存储命令
        int idx = commandbuf.find(':'); // 查找第一个冒号的位置
        if(idx == string::npos) // 没有找到  ==-1->不建议用
        {
            command = commandbuf; // 直接赋值
        }else{
            command = commandbuf.substr(0, idx); // 截取冒号前的部分作为命令
        }
        // 根据命令查找对应的处理函数
        auto it = commandHandlerMap.find(command); 
        if(it == commandHandlerMap.end())
        {
            // 没有找到对应的命令处理函数
            cout << "Command invalid! Please try again." << endl;
            continue; // 继续循环,重新输入命令
        }
        // 调用相应命令的事件处理回调函数
        it->second(clientSocket, commandbuf.substr(idx + 1, commandbuf.size() - idx)); // 调用对应的处理函数,传入clientfd和命令参数
    }
}

void help(int clientfd, string msg)
{
    // 将commandmap中的信息打印一遍一共用户学习使用方法
    cout << "==================== Chat Client Commands ====================" << endl;
    for(auto &command : commandMap)
    {
        cout << command.first << " : " << command.second << endl;
    }
    cout << "==============================================================" << endl;
}

void addfriend(int clientfd, string msg)
{
    // 添加好友命令格式: addfriend:friendid
    int friendid = atoi(msg.c_str()); // 将字符串转换为整数
    json js;
    js["msgid"] = ADD_FRIEND_MSG; // 添加好友消息id
    js["id"] = g_currentUser.getId(); // 当前用户id
    js["friendid"] = friendid; // 好友id
    // 发送添加好友请求
    string request = js.dump(); // 序列化为字符串
    if(send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0) < 0)
    {
        cerr << "Send request failed! " << request << endl;
    }
}

void chat(int clientfd, string msg)
{
    int idx = msg.find(":");
    if(idx == string::npos)
    {
        cout << "Command invalid! Please use format: chat:friendid:msg" << endl;
        return;
    }
    int friendid = atoi(msg.substr(0, idx).c_str()); // 获取好友id
    string message = msg.substr(idx + 1, msg.size() - idx); // 获取聊天内容
    json js;
    js["msgid"] = ONE_CHAT_MSG; // 一对一聊天消息id
    js["id"] = g_currentUser.getId(); // 当前用户id
    js["name"] = g_currentUser.getName(); // 当前用户昵称
    js["to"] = friendid; // 接收方用户 id --> 与服务端保持一致
    js["msg"] = message; // 聊天内容
    js["time"] = getCurrentTime(); // 获取当前时间

    // 发送聊天请求
    string request = js.dump(); // 序列化为字符串
    if(send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0) < 0)
    {
        cerr << "Send request failed! " << request << endl;
    }
}

// 创建群组
void creategroup(int clientfd, string msg)
{
    // 创建群组命令格式: groupname:groupdesc
    int idx = msg.find(":");
    if(idx == string::npos)
    {
        cout << "Command invalid! Please use format: creategroup:groupname:groupdesc" << endl;
        return;
    }
    string groupname = msg.substr(0, idx); // 获取群组名称
    string groupdesc = msg.substr(idx + 1, msg.size() - idx); // 获取群组描述
    json js;
    js["msgid"] = CREATE_GROUP_MSG; // 创建群组消息id
    js["id"] = g_currentUser.getId(); // 当前用户id
    js["groupname"] = groupname; // 群组名称
    js["groupdesc"] = groupdesc; // 群组描述

    // 发送创建群组请求
    string request = js.dump(); // 序列化为字符串
    if(send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0) < 0)
    {
        cerr << "Send request failed! " << request << endl;
    }
}

// 添加群组
void addgroup(int clientfd, string msg)
{
    // 添加群组命令格式 addgroup:groupid
    int groupid = atoi(msg.c_str()); // 将字符串转换为整数
    json js;
    js["msgid"] = ADD_GROUP_MSG; // 添加群组消息id
    js["id"] = g_currentUser.getId(); // 当前用户id
    js["groupid"] = groupid; // 群组id
    // 发送添加群的json请求
    string request = js.dump(); // 序列化为字符串
    if(send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0) < 0)
    {
        cerr << "Send request failed! " << request << endl;
    }
}

// 群组聊天
void groupchat(int clientfd, string msg)
{
    // 群聊命令格式groupchat:groupid:msg
    int idx = msg.find(":");
    if(idx == string::npos)
    {
        cout << "Command invalid! Please use format: groupchat:groupid:msg" << endl;
        return;
    }
    int groupid = atoi(msg.substr(0, idx).c_str()); // 获取群组id
    string message = msg.substr(idx + 1, msg.size() - idx); // 获取聊天内容
    json js;
    js["msgid"] = GROUP_CHAT_MSG; // 群组聊天消息id
    js["id"] = g_currentUser.getId(); // 当前用户id
    js["name"] = g_currentUser.getName(); // 当前用户昵称
    js["groupid"] = groupid; // 群组id
    js["msg"] = message; // 聊天内容
    js["time"] = getCurrentTime(); // 获取当前时间
    // 发送聊天群组请求
    string request = js.dump(); // 序列化为字符串
    if(send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0) < 0)
    {
        cerr << "Send request failed! " << request << endl;
    }
}
// 退出系统
void logout(int clientfd, string msg)
{
    json js;
    js["msgid"] = LOGINOUT_MSG; // 注销消息id
    js["id"] = g_currentUser.getId(); // 当前用户id

    // 发送注销请求
    string request = js.dump(); // 序列化为字符串

    if(send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0) < 0)
    {
        cerr << "Send Logout request failed! " << request << endl;
    }else{
        isMainMenuRunning = false; // 设置主菜单不再运行
    }
}