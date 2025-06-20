#ifndef GROUPUSER_HPP
#define GROUPUSER_HPP

#include "user.hpp"
#include <string>

using namespace std;

// 群组用户, 多了一个角色属性, 从User类继承
class GroupUser : public User {
public:
    void setRole(string role)
    {
        _role = role;
    }
    string getRole()
    {
        return _role;
    }
private:
    string _role; // 群组角色
};

#endif