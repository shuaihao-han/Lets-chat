#ifndef FRIENDMODEL_HPP
#define FRIENDMODEL_HPP

#include "user.hpp"
#include <vector>
using namespace std;

// 维护好友信息的操作接口方法
class FriendModel
{
public:
    // 添加好友关系
    bool insert(int userid, int friendid);

    // 返回用户的好友列表
    vector<User> query(int userid);

    // 删除好友关系
    bool remove(int userid, int friendid);
};

#endif