#ifndef CHATSERVICE_HPP
#define CHATSERVICE_HPP

#include <muduo/net/TcpConnection.h>
#include <muduo/net/EventLoop.h>
#include <muduo/base/Timestamp.h>
#include <muduo/base/Logging.h>
#include <unordered_map>
#include <functional>
#include <iostream>
#include <vector>
#include <mutex>
#include "json.hpp"
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"

using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

// 表示处理消息的事件回调方法类型
using MsgHandler = std::function<void(const TcpConnectionPtr&, json &js, Timestamp)>;

// 聊天服务器业务类
class ChatService {
public:
    // 获取单例对象的接口函数
    static ChatService* instance()
    {
        static ChatService service; // 线程安全的单例对象
        return &service;
    }
    // 处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);

    // 处理客户端异常断开连接
    void clientCloseException(const TcpConnectionPtr &conn);

    // 处理客户端注销业务的事件处理器
    void Logout(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 一对一聊天服务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理添加群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    void handleRedisSubscribeMessage(int userid, const string &msg);

    // 服务器异常退出,重置用户相关信息
    void reset();

private:
    ChatService();  // 私有化默认构造函数
    // 存储消息id和其对应的业务处理方法
    unordered_map<int, MsgHandler> _msgHandlerMap; // 消息处理器映射表

    // 因为聊天服务器的用户登录后需要长时间处于连接状态以进行消息的收发
    // 所以需要存储在线用户的通信连接
    unordered_map<int, TcpConnectionPtr> _userConnMap; // 用户id和连接的映射表

    // 定义互斥锁, 用于保证多线程环境下对用户连接映射表（_userConnMap）的安全访问
    std::mutex _connMutex; // 互斥锁，保护_userConnMap的线程安全
    
    // 数据操作类对象
    UserModel _userModel;   // 用户数据操作对象
    OfflineMsgModel _offlineMsgModel; // 离线消息数据操作对象
    FriendModel _friendModel; // 好友数据操作对象
    // 处理群组业务
    GroupModel _groupModel;

    // 定义 redis 操作对象
    Redis _redis;
};

#endif