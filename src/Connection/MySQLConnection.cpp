#include "Connection/MySQLConnection.hpp"
#include <iostream>
#include <sstream>

namespace Connection {

MySQLConnection::MySQLConnection() : mysql_(nullptr), connected_(false) {
    // 初始化MySQL库
    if (mysql_library_init(0, nullptr, nullptr) != 0) {
        last_error_ = "MySQL库初始化失败";
        return;
    }
}

MySQLConnection::~MySQLConnection() {
    disconnect();
    mysql_library_end();
}

bool MySQLConnection::initializeConnection() {
    mysql_ = mysql_init(nullptr);
    if (!mysql_) {
        last_error_ = "MySQL初始化失败";
        return false;
    }
    
    // 设置连接超时
    int timeout = 10;
    mysql_options(mysql_, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    
    return true;
}

bool MySQLConnection::setConnectionParams(const MySQLConnectionParams& params) {
    if (!mysql_) {
        return false;
    }
    
    // 设置字符集
    mysql_options(mysql_, MYSQL_SET_CHARSET_NAME, "utf8mb4");
    
    return true;
}

bool MySQLConnection::connect(const MySQLConnectionParams& params) {
    if (connected_) {
        disconnect();
    }
    
    if (!initializeConnection()) {
        return false;
    }
    
    if (!setConnectionParams(params)) {
        return false;
    }
    
    // 尝试连接
    if (!mysql_real_connect(mysql_, 
                           params.hostname.c_str(),
                           params.username.c_str(),
                           params.password.c_str(),
                           params.database.empty() ? nullptr : params.database.c_str(),
                           params.port,
                           nullptr, 0)) {
        last_error_ = mysql_error(mysql_);
        mysql_close(mysql_);
        mysql_ = nullptr;
        return false;
    }
    
    current_params_ = params;
    connected_ = true;
    last_error_.clear();
    
    return true;
}

void MySQLConnection::disconnect() {
    if (mysql_) {
        mysql_close(mysql_);
        mysql_ = nullptr;
    }
    connected_ = false;
}

MySQLQueryResult MySQLConnection::executeQuery(const std::string& query) {
    MySQLQueryResult result;
    
    if (!connected_ || !mysql_) {
        result.error_message = "未连接到数据库";
        return result;
    }
    
    if (mysql_query(mysql_, query.c_str()) != 0) {
        result.error_message = mysql_error(mysql_);
        return result;
    }
    
    MYSQL_RES* res = mysql_store_result(mysql_);
    if (!res) {
        // 非SELECT查询
        result.affected_rows = mysql_affected_rows(mysql_);
        result.success = true;
        return result;
    }
    
    // 获取列信息
    MYSQL_FIELD* fields = mysql_fetch_fields(res);
    int num_fields = mysql_num_fields(res);
    
    for (int i = 0; i < num_fields; ++i) {
        result.columns.push_back(fields[i].name);
    }
    
    // 获取行数据
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        std::vector<std::string> row_data;
        for (int i = 0; i < num_fields; ++i) {
            row_data.push_back(row[i] ? row[i] : "");
        }
        result.rows.push_back(row_data);
    }
    
    mysql_free_result(res);
    result.success = true;
    return result;
}

MySQLQueryResult MySQLConnection::executeNonQuery(const std::string& query) {
    MySQLQueryResult result;
    
    if (!connected_ || !mysql_) {
        result.error_message = "未连接到数据库";
        return result;
    }
    
    if (mysql_query(mysql_, query.c_str()) != 0) {
        result.error_message = mysql_error(mysql_);
        return result;
    }
    
    result.affected_rows = mysql_affected_rows(mysql_);
    result.success = true;
    return result;
}

std::vector<std::string> MySQLConnection::getDatabases() {
    std::vector<std::string> databases;
    
    if (!connected_ || !mysql_) {
        return databases;
    }
    
    MYSQL_RES* res = mysql_list_dbs(mysql_, nullptr);
    if (!res) {
        return databases;
    }
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        if (row[0]) {
            databases.push_back(row[0]);
        }
    }
    
    mysql_free_result(res);
    return databases;
}

std::vector<std::string> MySQLConnection::getTables(const std::string& database) {
    std::vector<std::string> tables;
    
    if (!connected_ || !mysql_) {
        return tables;
    }
    
    // 切换到指定数据库
    if (mysql_select_db(mysql_, database.c_str()) != 0) {
        return tables;
    }
    
    MYSQL_RES* res = mysql_list_tables(mysql_, nullptr);
    if (!res) {
        return tables;
    }
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        if (row[0]) {
            tables.push_back(row[0]);
        }
    }
    
    mysql_free_result(res);
    return tables;
}

MySQLQueryResult MySQLConnection::getTableStructure(const std::string& table) {
    std::string query = "DESCRIBE " + table;
    return executeQuery(query);
}

bool MySQLConnection::isConnected() const {
    return connected_ && mysql_;
}

std::string MySQLConnection::getLastError() const {
    return last_error_;
}

} // namespace Connection 