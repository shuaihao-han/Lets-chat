#include "offlinemessagemodel.hpp"
#include "db.h"

// 存储用户的离线消息
void OfflineMsgModel::insert(int userid, string msg)
{
    // 1. 组装 SQL 语句
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage values(%d, '%s')", userid, msg.c_str());
    // 2. 执行 SQL 语句
    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            LOG_INFO << "Insert offline message for user: " << userid << " success!";
        }
        else
        {
            LOG_ERROR << "Insert offline message for user: " << userid << " failed!";
        }
    }
    else
    {
        LOG_ERROR << "MySQL connection failed!";
    }
}

// 删除用户的离线消息
void OfflineMsgModel::remove(int userid)
{
    // 1. 组装 SQL 语句
    char sql[1024] = {0};
    sprintf(sql, "delete from offlinemessage where userid = %d", userid);
    // 2. 执行 SQL 语句
    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            LOG_INFO << "Remove offline messages for user: " << userid << " success!";
        }
        else
        {
            LOG_ERROR << "Remove offline messages for user: " << userid << " failed!";
        }
    }
    else
    {
        LOG_ERROR << "MySQL connection failed!";
    }
}

// 查询用户的离线消息
vector<string> OfflineMsgModel::query(int userid)
{
    vector<string> msgs;
    // 1. 组装 SQL 语句
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid = %d", userid);
    
    // 2. 执行 SQL 语句
    MySQL mysql;
    if(mysql.connect())
    {
        // 执行查询操作
        MYSQL_RES *res = mysql.query(sql);
        if(res)
        {
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                msgs.push_back(row[0]); // 将查询到的消息添加到 msgs 中
            }
            mysql_free_result(res); // 释放结果集
        }
        else
        {
            LOG_ERROR << "Query offline messages for user: " << userid << " failed!";
        }
    }
    else
    {
        LOG_ERROR << "MySQL connection failed!";
    }
    
    return msgs; // 返回查询到的离线消息
}