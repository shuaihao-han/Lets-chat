// 用于群组聊天业务的数据层数据操作类
#ifndef GROUPMODEL_HPP
#define GROUPMODEL_HPP

#include <iostream>
#include "db.h"
#include "group.hpp"
using namespace std;

class GroupModel
{
public:
    // 创建群组
    bool createGroup(Group &group);

    // 加入群组
    bool joinGroup(int groupid, int userid, string role);

    // 获取用户所在的群组列表
    vector<Group> queryGroups(int userid);

    // 根据指定群组id查询群组用户id列表，除了自己，用于群聊消息的转发
    vector<int> queryGroupUsers(int userid, int groupid);
};

#endif