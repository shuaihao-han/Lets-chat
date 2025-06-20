
#ifndef GROUP_HPP
#define GROUP_HPP

#include <string>
#include <vector>
#include "groupuser.hpp"

using namespace std;

// group的ORM类
class Group
{
private:
    int _id; // 群组id
    string _name; // 群组名称
    string _desc; // 群组描述
    vector<GroupUser> _users; // 群组成员的用户id列表
public:
    Group(int id = -1, string name = "", string desc = "")
        : _id(id), _name(name), _desc(desc) {}

    void setId(int id) { _id = id; }
    void setName(string name) { _name = name; }
    void setDesc(string desc) { _desc = desc; }

    int getId() { return _id; }
    string getName() { return _name; }
    string getDesc() { return _desc; }
    // 注意这里返回的是_user的引用，不能是vector<GroupUser>,否则会出现浅拷贝问题
    // i.e.：将vector<GroupUser>作为返回值会导致返回的对象是一个临时对象，无法修改原有的_user
    // 使用引用可以将修改作用到原有的_user上
    vector<GroupUser>& getUsers() { return _users; } 
};

#endif // GROUP_HPP