#include "chatserver.hpp"
#include "chatservice.hpp"
#include <functional>
#include <string>
#include "json.hpp"
using namespace std;
using namespace placeholders;
using json = nlohmann::json;

// 初始化聊天服务器对象
ChatServer::ChatServer(EventLoop *loop,               // 时间循环--反应堆
            const InetAddress &listenAddr, // 服务器地址结构--IP+PORT
            const string &nameArg)         // 服务器名字
    : _loop(loop), _server(loop, listenAddr, nameArg)
    {
        // 1. 给服务器注册用户连接的 创建 和 断开 回调 #5
        // void setConnectionCallback(const ConnectionCallback& cb){..}  函数原型
        // bind函数的作用建议问Qwen
        _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1)); // 传入的就是 回调函数, 而在这个类里, 写的回调函数是 成员方法, 有this指针, 但是只需要第二个传参,  因此使用 绑定器: this固定, const TcpConnectionPtr& 交给 传入者

        // 2. 给服务器注册用户 读写时间回调
        _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
        _server.setThreadNum(4);
    }

ChatServer::~ChatServer()
{

}

// 启动服务
void ChatServer::start()
{
    _server.start();
}


void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    // 客户端断开连接
    if(!conn->connected())
    {
        ChatService::instance()->clientCloseException(conn); // 处理客户端异常断开连接
        conn->shutdown(); // 关闭连接
    }
}

// 专门处理用户的 读写事件
void ChatServer::onMessage(const TcpConnectionPtr &conn, // 连接
                Buffer *buffer,               // 缓冲区
                Timestamp time)               // 时间
{
    // 1. 读取用户发送的消息
    string msg = buffer->retrieveAllAsString();  // 获取缓冲区的所有数据, 并清空缓冲区
    // 数据反序列化
    json js = json::parse(msg); // 解析json数据

    // 通过消息id获取对应的处理器
    // 达到的目的：完全解耦网络模块和业务模块的代码
    // 获取消息对应的处理器
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>()); 
    msgHandler(conn, js, time); // 回调消息绑定好的消息事件处理器，传入连接、json数据和时间戳
}
