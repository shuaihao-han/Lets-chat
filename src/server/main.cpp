#include "chatserver.hpp"
#include "chatservice.hpp"
#include <iostream>
#include <signal.h>

using namespace std;

// 处理服务器 Crtl+C 结束后，重置user的状态信息
void resetHandler(int)
{
    // 重置用户状态信息
    ChatService::instance()->reset();
    exit(0); // 退出程序
}

int main(int argc, char** argv)
{
    if(argc < 3)
    {
        cerr << "Command invalid! Usage: " << argv[0] << " <ip> <port>" << endl;
        exit(-1);
    }
    // 程序运行时捕捉到 SIGINT 信号（即 Ctrl+C），就去执行 resetHandler 函数；
    signal(SIGINT, resetHandler); 

    EventLoop loop;
    // 解析命令行参数，获取服务器IP和端口
    char* ip = argv[1];
    uint16_t port = atoi(argv[2]); // 将端口号从字符串转换为整数
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();   // 开启事件循环
    return 0;
}