// 表的数据操作类(提供真正对表进行操作的类)
#ifndef USERMODEL_H
#define USERMODEL_H

#include "user.hpp"

// User表的数据操作类
class UserModel
{
public:
    // User表的增加方法，返回增加成功或者失败
    bool insert(User &user); // 添加用户

    // 根据用户id查询用户的信息
    User query(int id); // 查询用户信息

    // 更新用户的状态信息 state
    bool updateState(User user);
    // 重置用户的状态信息
    void resetState();
};

#endif