#include "usermodel.hpp"
#include "db.h"

// User表增加方法的实现
bool UserModel::insert(User &user)
{
    // 1. 准备sql语句
    char sql[1024] = {0};
    // sprintf 函数用于字符串连接, c_str() 函数将 C++ 字符串转换为 C 风格字符串(string --> char*)
    sprintf(sql, "INSERT INTO user(name, password, state) VALUES('%s', '%s', '%s')",
            user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());

    // 2. 执行sql语句
    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql)) // 执行更新操作
        {
            // 获取插入成功的用户id
            user.setId(mysql_insert_id(mysql.getConnection())); // 获取插入数据的id
            return true; // 插入成功
        }
    }
    return false; // 插入失败
}

User UserModel::query(int id)
{
    // 1. 准备sql语句
    char sql[1024] = {0};
    sprintf(sql, "SELECT * FROM user WHERE id = %d", id);

    // 2. 执行sql语句
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql); // 执行查询操作
        if(res != nullptr)   // 查询成功
        {
            // 3. 处理查询结果
            // 获取查询结果的第一行, 返回值是 MYSQL_ROW 类型
            MYSQL_ROW row = mysql_fetch_row(res); 
            if(row != nullptr)  // 如果查询结果不为空
            {
                User user;
                // atoi() 将字符串转换为整数
                user.setId(atoi(row[0]));   // 设置用户id
                user.setName(row[1]);      // 设置用户名
                user.setPwd(row[2]);       // 设置密码
                user.setState(row[3]);     // 设置状态
                mysql_free_result(res);    // 注意需要释放查询结果集
                return user;               // 返回用户信息
            }
        }
    }
    
    return User();
}

// 更新用户状态信息
bool UserModel::updateState(User user)
{
    // 1. 准备sql语句
    char sql[1024] = {0};
    sprintf(sql, "UPDATE user SET state = '%s' WHERE id = %d",
            user.getState().c_str(), user.getId());
    // 2. 执行sql语句
    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql)) // 执行更新操作
        {
            return true;   // 返回更新成功
        }
    }
    return false; // 返回更新失败
}

void UserModel::resetState()
{
    // 1. 准备sql语句
    const char *sql = "UPDATE user SET state = 'offline'"; // 重置所有用户状态为离线
    // 2. 执行sql语句
    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql); // 执行更新操作
    }
}