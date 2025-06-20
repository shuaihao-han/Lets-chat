#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <muduo/net/TcpServer.h> 
#include <muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;

class ChatServer
{
private:
    TcpServer _server;   // 组合的muduo库, 实现服务器功能的类对象
    EventLoop *_loop;   // 指向事件循环的指针

    // 上报连接相关信息的回调函数
    void onConnection(const TcpConnectionPtr &conn);
    // 专门处理用户的 读写事件的回调函数
    void onMessage(const TcpConnectionPtr &conn, // 连接
                   Buffer *buffer,               // 缓冲区
                   Timestamp time);               // 时间
public:
    // 初始化聊天服务器对象
    ChatServer(EventLoop *loop,               // 时间循环--反应堆
               const InetAddress &listenAddr, // 服务器地址结构--IP+PORT
               const string &nameArg);
    ~ChatServer();
    void start();
};

#endif