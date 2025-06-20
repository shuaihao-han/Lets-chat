#include "groupmodel.hpp"

// 创建群组
bool GroupModel::createGroup(Group &group)
{
    // 1. 准备SQL语句
    char sql[1024] = {0};
    sprintf(sql, "INSERT INTO allgroup(groupname, groupdesc) VALUES ('%s', '%s')", 
            group.getName().c_str(), group.getDesc().c_str());  
    // 2. 执行SQL语句
    MySQL mysql;   // 创建MySQL对象
    if (mysql.connect())
    {
        if (mysql.update(sql)) // 执行更新操作
        {
            group.setId(mysql_insert_id(mysql.getConnection())); // 获取插入成功的群组id
            return true; // 创建群组成功
        }
    }
    return false;
}

// 加入群组
bool GroupModel::joinGroup(int groupid, int userid, string role)
{
    // 1. 准备SQL语句
    char sql[1024] = {0};
    sprintf(sql, "INSERT INTO groupuser(groupid, userid, grouprole) VALUES (%d, %d, '%s')", 
            groupid, userid, role.c_str());
    // 2. 执行SQL语句
    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql)) // 执行更新操作
        {
            return true; // 加入群组成功
        }
    }
    return false;  // 加入群组失败
}

// 查询用户所在群组 -- 联合查询, 直接取出群组的全部信息
// 根据用户id查询用户所在的群组列表(userid --> groupid), 在根据群组id查询群组信息 (groupid --> groupusers)
vector<Group> GroupModel::queryGroups(int userid)
{
    // (一). 先查询用户所在的所有群组的 群组信息
    // 1. 准备SQL语句
    char sql[1024] = {0};
    sprintf(sql, "SELECT g.id, g.groupname, g.groupdesc FROM allgroup g "
                 "INNER JOIN groupuser gu ON g.id = gu.groupid WHERE gu.userid = %d", userid);
    vector<Group> groups;  // 用于存储查询到的群组信息
    // 2. 执行SQL语句
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql); // 执行查询操作
        if(res != nullptr) // 查询成功
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr) // 遍历查询结果
            {
                Group group;
                group.setId(atoi(row[0])); // 设置群组id
                group.setName(row[1]);     // 设置群组名称
                group.setDesc(row[2]);     // 设置群组描述
                groups.push_back(group);    // 添加到群组列表中
            }
            mysql_free_result(res); // 释放查询结果集
        }
        // (二).查询每个群组的其他用户信息---群组用户id列表
        for(auto &group : groups) // 遍历群组列表, 注意这里需要是引用,因为下面需要对该对象进行修改
        {
            // 1. 准备SQL语句
            sprintf(sql, "SELECT u.id, u.name, u.state, gu.grouprole FROM user u "
                         "INNER JOIN groupuser gu ON u.id = gu.userid "
                         "WHERE gu.groupid = %d", group.getId());
            // 2. 执行SQL语句
            MYSQL_RES *res = mysql.query(sql); // 执行查询操作
            if(res != nullptr) // 查询成功
            {
                MYSQL_ROW row;
                while((row = mysql_fetch_row(res)) != nullptr) // 遍历查询结果
                {
                    GroupUser groupUser;
                    groupUser.setId(atoi(row[0])); // 设置用户id
                    groupUser.setName(row[1]);     // 设置用户名
                    groupUser.setState(row[2]);    // 设置用户状态(在线/离线)
                    groupUser.setRole(row[3]);     // 设置用户在群组中的角色
                    group.getUsers().push_back(groupUser);   // 添加到群组用户列表中
                }
                mysql_free_result(res); // 释放查询结果集
            }
        }
    }
    return groups; // 返回查询到的群组列表
}

// 根据指定群组id查询群组用户id列表，除了自己，用于群聊消息的转发
vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    // 1.准备SQL语句
    char sql[1024] = {0};
    // 查询除了自己的用户id之外的所有群组用户id, 只需要id就可以确定消息转发对象, 没有必要获取其他信息
    sprintf(sql, "SELECT userid FROM groupuser WHERE groupid = %d AND userid != %d", groupid, userid);
    vector<int> userIds; // 用于存储群组用户id列表
    // 2.执行SQL语句
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql); // 执行查询操作
        if(res != nullptr) // 查询成功
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr) // 遍历查询结果
            {
                userIds.push_back(atoi(row[0])); // 将用户id添加到列表中
            }
            mysql_free_result(res); // 释放查询结果集
        }
    }
    return userIds; // 返回群组用户id列表
}
