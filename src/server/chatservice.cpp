#include "chatservice.hpp"
#include "public.hpp"

// 注册消息以及对应的回调操作
ChatService::ChatService()
{
    // 1. 注册登录业务回调函数
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    // 2. 注册注册业务回调函数
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    // 3. 注册一对一聊天业务回调函数
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChat, this, _1, _2, _3)});
    // 4. 注册添加好友业务回调函数
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriend, this, _1, _2, _3)});
    // 5. 注册创建群组业务回调函数
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    // 6. 注册添加群组业务回调函数
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    // 7. 注册群组聊天业务回调函数
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});
    // 8. 注册客户端注销业务的回调函数
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::Logout, this, _1, _2, _3)});

    // 连接 Redis 服务器
    if(_redis.connect())
    {
        // 设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}

// 服务器异常退出,重置用户相关信息
void ChatService::reset()
{
    // 1. 更新所有在线用户的状态为离线
    _userModel.resetState(); // 重置所有用户状态为离线
}

MsgHandler ChatService::getHandler(int msgid)
{
    // 记录错误日志，msgid没有对应的事件处理回调【不合法】
    auto it = _msgHandlerMap.find(msgid);
    if(it == _msgHandlerMap.end())
    {
        // 返回一个默认的处理器 --> 空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time) {
            LOG_ERROR << "msgid: " << msgid << " can not find handler!";
        };
    }
    else
    {
        return _msgHandlerMap[msgid]; // 根据消息id获取对应的处理器
    }
}

// 处理登录业务
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();  // 获取用户id
    string pwd = js["password"];
    // 传入id和密码查询用户信息
    User user = _userModel.query(id); // 查询用户信息
    if(user.getId() != -1 && user.getPwd() == pwd){
        if(user.getState() == "online")
        {
            // 该用户已经登录,不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK; // 登录消息id
            response["erron"] = 2;   // 错误码 2 表示用户已经登录
            response["errmsg"] = "This user is already online!"; // 错误提示信息
            conn->send(response.dump()); // 将json数据序列化为字符串并发送给客户端
        }
        // 登录成功, 记录用户连接信息
        // 锁只需要保证_userConnMap的线程安全访问，因此使用 {} 来限制锁的作用域
        // 使得所得粒度尽量小
        {
            std::lock_guard<std::mutex> lock(_connMutex); // 加锁，保护_userConnMap的线程安全
            _userConnMap.insert({id, conn}); // 将用户id和连接存入映射表
        }

        // id 用户登录成功后向 redis 订阅 channel(id)
        _redis.subscribe(id);

        // 更新用户状态信息 state offline --> online
        user.setState("online"); // 设置用户状态为在线
        _userModel.updateState(user); // 更新用户状态到数据库
        
        // 登录成功返回消息响应
        json response;
        response["msgid"] = LOGIN_MSG_ACK; // 登录消息id
        response["erron"] = 0;   // 错误码 0 表示成功
        response["id"] = user.getId(); // 返回用户id
        response["name"] = user.getName(); // 返回用户昵称

        // 查询用户是否有离线消息
        vector<string> offlineMsgs = _offlineMsgModel.query(id); // 查询离线消息
        if(!offlineMsgs.empty())
        {
            // 有离线消息, 将离线消息添加到json中
            response["offlinemsgs"] = offlineMsgs; // 添加离线消息到响应
            // 读取离线消息后,将离线消息从数据库中删除
            _offlineMsgModel.remove(id);
        }else{
            response["offlinemsgs"] = vector<string>(); // 没有离线消息，返回空数组
        }

        // 查询该用户的好友信息并返回
        vector<User> friends = _friendModel.query(id); // 查询好友列表
        if(!friends.empty())   // 如果好友列表不为空 --> 返回给用户
        {
            // 有好友信息, 将好友信息添加到json中
            vector<string> friendList; // 用于存储好友信息的json字符串
            for(auto &friendUser : friends)
            {
                json friendJson;
                friendJson["id"] = friendUser.getId(); // 好友id
                friendJson["name"] = friendUser.getName(); // 好友昵称
                friendJson["state"] = friendUser.getState(); // 好友状态
                friendList.push_back(friendJson.dump()); // 将好友信息序列化为字符串并添加到列表中
            }
            response["friends"] = friendList; // 添加好友列表到响应
        }

        // 查询该用户的群组信息并返回
        vector<Group> groups = _groupModel.queryGroups(id); // 查询用户所在的群组列表
        if(!groups.empty())   // 如果群组列表不为空 --> 返回给用户
        {
            vector<string> groupList; // 用于存储群组信息的json字符串
            for(auto &group : groups)
            {
                json groupJson;
                groupJson["id"] = group.getId(); // 群组id
                groupJson["groupname"] = group.getName(); // 群组名称
                groupJson["groupdesc"] = group.getDesc(); // 群组描述
                vector<string> userList; // 用于存储群组用户信息的json字符串
                for(auto &groupuser : group.getUsers())
                {
                    json userJson;
                    userJson["id"] = groupuser.getId(); // 用户id
                    userJson["name"] = groupuser.getName(); // 用户昵称
                    userJson["state"] = groupuser.getState(); // 用户状态
                    userJson["role"] = groupuser.getRole(); // 用户角色
                    userList.push_back(userJson.dump()); // 将用户信息序列化为字符串并添加到列表中
                    // 将json序列化字符串输出以供调试
                    // cout << "Group User: " << userJson.dump();
                }
                groupJson["users"] = userList; // 添加群组用户列表到群组信息中
                groupList.push_back(groupJson.dump()); // 将群组信息序列化为字符串并添加到列表中
            }
            response["groups"] = groupList; // 添加群组列表到响应
        }

        conn->send(response.dump()); // 将json数据序列化为字符串并发送给客户端
    }else{
        // 该用户不存在或者用户存在但是密码错误 --> 登录失败,返回提示信息
        json response;
        response["msgid"] = LOGIN_MSG_ACK; // 登录消息id
        response["erron"] = 1;   // 错误码 1 表示失败
        response["errmsg"] = "Username or password error!"; // 错误提示信息
        conn->send(response.dump()); // 将json数据序列化为字符串并发送给客户端
    }
}

// 处理注册业务 name password
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);

    // 调用insert方法将用户信息插入到数据库中
    bool state = _userModel.insert(user);
    if(state)
    {
        // 用户注册成功
        json response;
        response["msgid"] = REG_MSG_ACK; // 注册成功的消息id
        response["erron"] = 0;   // 错误码 0 表示成功
        response["id"] = user.getId(); // 返回用户id
        conn->send(response.dump()); // 将json数据序列化为字符串并发送给客户端
    }else{
        // 用户注册失败
        json response;
        response["msgid"] = REG_MSG_ACK; // 注册成功的消息id
        response["erron"] = 1;   // 错误码 1 表示失败
        response["errmsg"] = "registion failed"; // 错误提示信息
        conn->send(response.dump()); // 将json数据序列化为字符串并发送给客户端
    }
}

void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    // 需要做两件事情
    User user;
    // 注意可能有用户在访问_userConnMap，因此需要加锁保护 --> 线程安全
    // 1. 从映射表_userConnMap中删除该用户的连接
    {
        std::lock_guard<std::mutex> lock(_connMutex); 
        for(auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if(it->second == conn) // 找到对应的连接
            {
                int id = it->first; // 获取用户id
                user.setId(id); // 设置用户id
                _userConnMap.erase(it); // 从映射表中删除该用户的连接
                LOG_INFO << "user: " << id << " has closed the connection!";
                break; // 找到后退出循环
            }
        }
    }

    _redis.unsubscribe(user.getId()); // 取消订阅该用户的频道

    // 2. 更新用户状态信息 state online --> offline
    if(user.getId() != -1)
    {
        user.setState("offline"); // 设置用户状态为离线
        _userModel.updateState(user); // 更新用户状态到数据库
    }
}

void ChatService::Logout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // 与clientCloseException不同的是，异常退出无法获取用户的id
    // 只能通过查找对应的TCP连接来进行删除操作
    // 但是用户的主动注销可以获得用户id，基于用户id进行相关业务
    // 获取待注销的用户id
    int id = js["id"].get<int>();
    // 1. 删除该用户对应的TCP连接
    {
        std::lock_guard<std::mutex> lock(_connMutex); // 加锁，保护_userConnMap的线程安全
        auto it = _userConnMap.find(id); // 查找用户连接
        if(it != _userConnMap.end())
        {
            _userConnMap.erase(it); // 从映射表中删除该用户的连接
        }
    }

    // 用户注销，相当于就是下线，取消在redis中的订阅
    _redis.unsubscribe(id); // 取消订阅该用户的频道

    // 2. 更新用户状态信息 state online --> offline
    User user;
    user.setId(id); // 设置用户id
    user.setState("offline"); // 设置用户状态为离线
    _userModel.updateState(user); // 更新用户状态到数据库
}

// 一对一聊天业务实现
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // 获取接收方id
    int toid = js["to"].get<int>();
    {
        lock_guard<std::mutex> lock(_connMutex); // 加锁，保护_userConnMap的线程安全
        auto it = _userConnMap.find(toid); // 查找接收方的连接
        if(it != _userConnMap.end())
        {
            // 找到接收方的连接 --> toid 在线  --> 服务器主动推送消息给toid用户
            // 其实就是做了一个消息转发的操作
            it->second->send(js.dump());
            return ;
        }
    }
    // toid 不在线, 判断它是否在其他 ChatServer 上登录
    User user = _userModel.query(toid); // 查询接收方用户信息
    if(user.getState() == "online")
    {
        // 用户在其他设备上登录 --> 通过 redis 将 js 转发给该用户
        _redis.publish(toid, js.dump()); // 发布消息到 redis 的频道
        return ;
    }
    _offlineMsgModel.insert(toid, js.dump()); // 存储离线消息到数据库
}

// 添加好友业务实现
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>(); // 获取用户id
    int friendid = js["friendid"].get<int>(); // 获取好友id
    // 调用FriendModel的insert方法添加好友关系
    bool state = _friendModel.insert(userid, friendid);
    if(state)
    {
        // 添加好友成功
        json response;
        response["msgid"] = ADD_FRIEND_MSG; // 添加好友消息id
        response["erron"] = 0;   // 错误码 0 表示成功
        response["id"] = userid; // 返回用户id
        response["friendid"] = friendid; // 返回好友id
        conn->send(response.dump()); // 将json数据序列化为字符串并发送给客户端
    }else{
        // 添加好友失败
        json response;
        response["msgid"] = ADD_FRIEND_MSG; // 添加好友消息id
        response["erron"] = 1;   // 错误码 1 表示失败
        response["errmsg"] = "add friend failed"; // 错误提示信息
        conn->send(response.dump()); // 将json数据序列化为字符串并发送给客户端
    }
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // 获取群组名称和描述
    int userid = js["id"].get<int>(); // 获取用户id
    string name = js["groupname"];
    string desc = js["groupdesc"];
    Group group; // 创建群组对象
    group.setName(name); // 设置群组名称
    group.setDesc(desc); // 设置群组描述

    // 调用GroupModel的createGroup方法创建群组
    if(_groupModel.createGroup(group))
    {
        // 创建群组成功
        // 将创建者加入群组,并将其身份设置为"creator"
        _groupModel.joinGroup(group.getId(), userid, "creator"); // 将创建者加入群组，角色为创建者
        // 返回创建成功的相应以进行提示
        json response;
        response["msgid"] = CREATE_GROUP_MSG; // 创建群组消息id
        response["erron"] = 0;   // 错误码 0 表示成功
        response["groupid"] = group.getId(); // 返回群组id
        conn->send(response.dump()); // 将json数据序列化为字符串并发送给客户端
    }else{
        // 创建群组失败
        json response;
        response["msgid"] = CREATE_GROUP_MSG; // 创建群组消息id
        response["erron"] = 1;   // 错误码 1 表示失败
        response["errmsg"] = "create group failed"; // 错误提示信息
        conn->send(response.dump()); // 将json数据序列化为字符串并发送给客户端
    }
}

// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // 获取用户id和群组id
    int userid = js["id"].get<int>(); // 获取用户id
    int groupid = js["groupid"].get<int>(); // 获取群组id
    // 用户加入群组说明不是创建者
    string role = "normal"; // 设置用户角色为普通成员
    // 调用GroupModel的joinGroup方法将用户加入群组
    _groupModel.joinGroup(groupid, userid, role); // 将用户加入群组，角色为普通成员
    // 返回加入群组成功的响应 (省略)
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // 获取群组id和消息发送者id
    int groupid = js["groupid"].get<int>(); // 获取群组id
    int userid = js["id"].get<int>(); // 获取用户id
    // 获取群组用户列表
    vector<int> groupUsers = _groupModel.queryGroupUsers(userid, groupid); // 查询群组用户列表
    // 向群聊中的用户发送消息
    // 注意这里需要加锁保护_userConnMap的线程安全访问
    lock_guard<mutex> lock(_connMutex); // 加锁，保护_userConnMap的线程安全
    for(int id:groupUsers)
    {
        auto it = _userConnMap.find(id); // 查找群组用户的连接
        if(it != _userConnMap.end())
        {
            // 找到群组用户的连接 --> 在线 --> 服务器主动推送消息给群组用户
            it->second->send(js.dump()); // 将消息发送给群组用户
        }else{
            // 查询该用户是否在其他 ChatServer 上登录
            User user = _userModel.query(id); // 查询群组用户信息
            if(user.getState() == "online")
            {
                // 群组用户在其他设备上登录 --> 通过 redis 将 js 转发给该用户
                _redis.publish(id, js.dump()); // 发布消息到 redis 的频道
            }else{
                // 群组用户不在线, 存储离线消息
                _offlineMsgModel.insert(id, js.dump()); // 存储离线消息到数据库
            }   
        }
    }
}

void ChatService::handleRedisSubscribeMessage(int userid, const string &msg)
{
    // 通过userid获取用户连接
    std::lock_guard<std::mutex> lock(_connMutex); // 加锁，保护_userConnMap的线程安全
    auto it = _userConnMap.find(userid); // 查找用户连接
    if(it != _userConnMap.end())
    {
        // 找到用户连接 --> 在线 --> 服务器主动推送消息给用户
        it->second->send(msg); // 将消息发送给用户
        return ;
    }

    // 用户不在线, 存储离线消息
    _offlineMsgModel.insert(userid, msg); // 存储离线消息到数据库
}