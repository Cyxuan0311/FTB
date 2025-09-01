#include "UI/MySQLDialog.hpp"
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/event.hpp>
#include <iostream>
#include <sstream>

using namespace ftxui;

namespace UI {

MySQLDialog::MySQLDialog() 
    : dialog_open_(false), connected_(false), active_tab_(0) {
    
    // 初始化输入框
    hostname_input_ = Input(&hostname_, "localhost");
    port_input_ = Input(&port_, "3306");
    username_input_ = Input(&username_, "root");
    password_input_ = Input(&password_, "");
    database_input_ = Input(&database_, "");
    query_input_ = Input(&query_, "SELECT * FROM table_name");
    
    // 初始化选择器
    database_selector_ = Menu(&databases_, &selected_database_);
    table_selector_ = Menu(&tables_, &selected_table_);
    
    // 初始化按钮
    connect_button_ = Button("连接", [this] { onConnect(); });
    disconnect_button_ = Button("断开", [this] { onDisconnect(); });
    execute_query_button_ = Button("执行查询", [this] { onExecuteQuery(); });
    create_table_button_ = Button("创建表", [this] { onCreateTable(); });
    delete_table_button_ = Button("删除表", [this] { onDeleteTable(); });
    insert_data_button_ = Button("插入数据", [this] { onInsertData(); });
    update_data_button_ = Button("更新数据", [this] { onUpdateData(); });
    delete_data_button_ = Button("删除数据", [this] { onDeleteData(); });
    
    // 设置默认值
    hostname_ = "localhost";
    port_ = "3306";
    username_ = "root";
    password_ = "";
    database_ = "";
    
    // 创建MySQL连接对象
    mysql_connection_ = std::make_unique<Connection::MySQLConnection>();
}

void MySQLDialog::showDialog(ftxui::ScreenInteractive& screen) {
    screen_ = &screen;
    dialog_open_ = true;
    
    auto dialog_component = createMainDialog();
    
    // 创建模态对话框
    auto modal = Modal(dialog_component, [this] {
        dialog_open_ = false;
        screen_->Exit();
    });
    
    screen->Loop(modal);
}

ftxui::Component MySQLDialog::createMainDialog() {
    // 创建标签页
    auto tabs = Container::Tab({
        createConnectionSettings(),
        createDatabaseOperations(),
        createTableOperations(),
        createQueryExecutor()
    }, &active_tab_);
    
    // 标签页标题
    auto tab_titles = Container::Horizontal({
        Button("连接设置", [this] { active_tab_ = 0; }),
        Button("数据库操作", [this] { active_tab_ = 1; }),
        Button("表操作", [this] { active_tab_ = 2; }),
        Button("查询执行", [this] { active_tab_ = 3; })
    });
    
    // 状态栏
    auto status_bar = Text(status_text_);
    
    // 主容器
    auto main_container = Container::Vertical({
        tab_titles,
        tabs,
        status_bar
    });
    
    return main_container;
}

ftxui::Component MySQLDialog::createConnectionSettings() {
    auto local_connection = Container::Vertical({
        Text("本地MySQL连接"),
        Container::Horizontal({
            Text("主机: "), hostname_input_
        }),
        Container::Horizontal({
            Text("端口: "), port_input_
        }),
        Container::Horizontal({
            Text("用户名: "), username_input_
        }),
        Container::Horizontal({
            Text("密码: "), password_input_
        }),
        Container::Horizontal({
            Text("数据库: "), database_input_
        }),
        Container::Horizontal({
            connect_button_,
            disconnect_button_
        })
    });
    
    auto remote_connection = Container::Vertical({
        Text("远程MySQL连接"),
        Container::Horizontal({
            Text("主机: "), hostname_input_
        }),
        Container::Horizontal({
            Text("端口: "), port_input_
        }),
        Container::Horizontal({
            Text("用户名: "), username_input_
        }),
        Container::Horizontal({
            Text("密码: "), password_input_
        }),
        Container::Horizontal({
            Text("数据库: "), database_input_
        }),
        Container::Horizontal({
            connect_button_,
            disconnect_button_
        })
    });
    
    return Container::Vertical({
        local_connection,
        Separator(),
        remote_connection
    });
}

ftxui::Component MySQLDialog::createDatabaseOperations() {
    return Container::Vertical({
        Text("数据库操作"),
        Container::Horizontal({
            Text("选择数据库: "), database_selector_
        }),
        Container::Horizontal({
            Button("刷新数据库列表", [this] { refreshDatabases(); })
        }),
        Container::Horizontal({
            Button("创建数据库", [this] { 
                if (!database_.empty()) {
                    std::string query = "CREATE DATABASE IF NOT EXISTS " + database_;
                    auto result = mysql_connection_->executeNonQuery(query);
                    if (result.success) {
                        status_text_ = "数据库创建成功";
                        refreshDatabases();
                    } else {
                        status_text_ = "数据库创建失败: " + result.error_message;
                    }
                }
            })
        })
    });
}

ftxui::Component MySQLDialog::createTableOperations() {
    return Container::Vertical({
        Text("表操作"),
        Container::Horizontal({
            Text("选择表: "), table_selector_
        }),
        Container::Horizontal({
            create_table_button_,
            delete_table_button_
        }),
        Container::Horizontal({
            insert_data_button_,
            update_data_button_,
            delete_data_button_
        })
    });
}

ftxui::Component MySQLDialog::createQueryExecutor() {
    return Container::Vertical({
        Text("SQL查询执行"),
        Container::Horizontal({
            Text("SQL语句: "), query_input_
        }),
        Container::Horizontal({
            execute_query_button_
        }),
        Container::Horizontal({
            Text("查询结果:")
        }),
        Container::Horizontal({
            Text(query_result_text_)
        })
    });
}

void MySQLDialog::onConnect() {
    if (!validateInput()) {
        status_text_ = "请填写完整的连接信息";
        return;
    }
    
    Connection::MySQLConnectionParams params;
    params.hostname = hostname_;
    params.port = std::stoi(port_);
    params.username = username_;
    params.password = password_;
    params.database = database_;
    params.is_local = (hostname_ == "localhost" || hostname_ == "127.0.0.1");
    
    if (mysql_connection_->connect(params)) {
        connected_ = true;
        status_text_ = "连接成功";
        refreshDatabases();
        if (connection_callback_) {
            connection_callback_(params);
        }
    } else {
        status_text_ = "连接失败: " + mysql_connection_->getLastError();
    }
}

void MySQLDialog::onDisconnect() {
    if (mysql_connection_) {
        mysql_connection_->disconnect();
        connected_ = false;
        status_text_ = "已断开连接";
        databases_.clear();
        tables_.clear();
    }
}

void MySQLDialog::onExecuteQuery() {
    if (!connected_) {
        status_text_ = "请先连接数据库";
        return;
    }
    
    if (query_.empty()) {
        status_text_ = "请输入SQL查询语句";
        return;
    }
    
    auto result = mysql_connection_->executeQuery(query_);
    displayQueryResult(result);
    
    if (result.success) {
        status_text_ = "查询执行成功";
    } else {
        status_text_ = "查询执行失败: " + result.error_message;
    }
}

void MySQLDialog::onCreateTable() {
    if (!connected_) {
        status_text_ = "请先连接数据库";
        return;
    }
    
    if (selected_database_.empty()) {
        status_text_ = "请先选择数据库";
        return;
    }
    
    // 这里可以弹出一个创建表的对话框
    status_text_ = "创建表功能待实现";
}

void MySQLDialog::onDeleteTable() {
    if (!connected_) {
        status_text_ = "请先连接数据库";
        return;
    }
    
    if (selected_table_.empty()) {
        status_text_ = "请先选择要删除的表";
        return;
    }
    
    std::string query = "DROP TABLE " + selected_table_;
    auto result = mysql_connection_->executeNonQuery(query);
    
    if (result.success) {
        status_text_ = "表删除成功";
        refreshTables();
    } else {
        status_text_ = "表删除失败: " + result.error_message;
    }
}

void MySQLDialog::onInsertData() {
    if (!connected_) {
        status_text_ = "请先连接数据库";
        return;
    }
    
    if (selected_table_.empty()) {
        status_text_ = "请先选择要插入数据的表";
        return;
    }
    
    status_text_ = "插入数据功能待实现";
}

void MySQLDialog::onUpdateData() {
    if (!connected_) {
        status_text_ = "请先连接数据库";
        return;
    }
    
    if (selected_table_.empty()) {
        status_text_ = "请先选择要更新数据的表";
        return;
    }
    
    status_text_ = "更新数据功能待实现";
}

void MySQLDialog::onDeleteData() {
    if (!connected_) {
        status_text_ = "请先连接数据库";
        return;
    }
    
    if (selected_table_.empty()) {
        status_text_ = "请先选择要删除数据的表";
        return;
    }
    
    status_text_ = "删除数据功能待实现";
}

bool MySQLDialog::validateInput() {
    return !hostname_.empty() && !port_.empty() && 
           !username_.empty() && !password_.empty();
}

void MySQLDialog::refreshDatabases() {
    if (!connected_) {
        return;
    }
    
    databases_ = mysql_connection_->getDatabases();
    if (!databases_.empty()) {
        selected_database_ = databases_[0];
        refreshTables();
    }
}

void MySQLDialog::refreshTables() {
    if (!connected_ || selected_database_.empty()) {
        return;
    }
    
    tables_ = mysql_connection_->getTables(selected_database_);
    if (!tables_.empty()) {
        selected_table_ = tables_[0];
    }
}

void MySQLDialog::displayQueryResult(const Connection::MySQLQueryResult& result) {
    std::ostringstream oss;
    
    if (result.success) {
        if (!result.columns.empty()) {
            // 显示列名
            for (const auto& col : result.columns) {
                oss << col << "\t";
            }
            oss << "\n";
            
            // 显示分隔线
            for (size_t i = 0; i < result.columns.size(); ++i) {
                oss << "----\t";
            }
            oss << "\n";
            
            // 显示数据行
            for (const auto& row : result.rows) {
                for (const auto& cell : row) {
                    oss << cell << "\t";
                }
                oss << "\n";
            }
        } else {
            oss << "影响行数: " << result.affected_rows;
        }
    } else {
        oss << "错误: " << result.error_message;
    }
    
    query_result_text_ = oss.str();
}

} // namespace UI 