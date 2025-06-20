#ifndef OFFLINEMESSAGEMODEL_HPP
#define OFFLINEMESSAGEMODEL_HPP

#include <string>
#include <vector>
#include <muduo/base/Logging.h>
using namespace std;
using namespace muduo;

// 提供离线消息表的操作方法
class OfflineMsgModel
{
private:
    
public:
    // 存储用户的离线消息
    void insert(int userid, string msg);

    // 删除用户的离线消息
    void remove(int userid);

    // 查询用户的离线消息
    vector<string> query(int userid);
};

#endif