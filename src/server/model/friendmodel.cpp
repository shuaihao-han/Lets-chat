#include "friendmodel.hpp"
#include "db.h"

// 添加好友关系
bool FriendModel::insert(int userid, int friendid)
{
    // 1. 准备sql语句
    char sql[1024] = {0};
    sprintf(sql, "INSERT INTO friend (userid, friendid) VALUES (%d, %d)", userid, friendid);

    // 2. 执行sql语句
    MySQL mysql;
    if (mysql.connect())
    {
        return mysql.update(sql); // 执行更新操作
    }
    return false; // 插入失败
}

// 返回用户的好友列表
vector<User> FriendModel::query(int userid)
{
    // 1. 准备sql语句
    char sql[1024] = {0};
    sprintf(sql, "SELECT u.id, u.name, u.state FROM user u INNER JOIN friend f ON u.id = f.friendid WHERE f.userid = %d", userid);
    vector<User> vec; // 用于存储好友列表
    // 2. 执行sql语句
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql); // 执行查询操作
        if (res != nullptr) // 查询成功
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr) // 遍历查询结果
            {
                User user;
                user.setId(atoi(row[0])); // 设置用户id
                user.setName(row[1]);     // 设置用户名
                user.setState(row[2]);    // 设置状态
                vec.push_back(user);      // 添加到好友列表中
            }
            mysql_free_result(res); // 释放查询结果集
        }
    }
    return vec; // 返回好友列表
}

// 删除好友关系
bool FriendModel::remove(int userid, int friendid)
{

}