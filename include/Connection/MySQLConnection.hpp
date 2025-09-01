#ifndef MYSQL_CONNECTION_HPP
#define MYSQL_CONNECTION_HPP

#include <string>
#include <vector>
#include <memory>
#include <mysql/mysql.h>

namespace Connection {

// MySQL连接参数结构
struct MySQLConnectionParams {
    std::string hostname;
    int port;
    std::string username;
    std::string password;
    std::string database;
    bool is_local;
    
    MySQLConnectionParams() : port(3306), is_local(false) {}
};

// MySQL查询结果结构
struct MySQLQueryResult {
    std::vector<std::string> columns;
    std::vector<std::vector<std::string>> rows;
    int affected_rows;
    std::string error_message;
    bool success;
    
    MySQLQueryResult() : affected_rows(0), success(false) {}
};

// MySQL连接管理类
class MySQLConnection {
public:
    MySQLConnection();
    ~MySQLConnection();
    
    // 连接数据库
    bool connect(const MySQLConnectionParams& params);
    
    // 断开连接
    void disconnect();
    
    // 执行查询
    MySQLQueryResult executeQuery(const std::string& query);
    
    // 执行非查询语句（INSERT, UPDATE, DELETE等）
    MySQLQueryResult executeNonQuery(const std::string& query);
    
    // 获取数据库列表
    std::vector<std::string> getDatabases();
    
    // 获取表列表
    std::vector<std::string> getTables(const std::string& database);
    
    // 获取表结构
    MySQLQueryResult getTableStructure(const std::string& table);
    
    // 检查连接状态
    bool isConnected() const;
    
    // 获取最后错误信息
    std::string getLastError() const;

private:
    MYSQL* mysql_;
    MySQLConnectionParams current_params_;
    std::string last_error_;
    bool connected_;
    
    // 初始化MySQL连接
    bool initializeConnection();
    
    // 设置连接参数
    bool setConnectionParams(const MySQLConnectionParams& params);
};

} // namespace Connection

#endif // MYSQL_CONNECTION_HPP 