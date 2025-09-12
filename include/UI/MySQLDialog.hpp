#ifndef MYSQL_DIALOG_HPP
#define MYSQL_DIALOG_HPP

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <string>
#include <functional>
#include <memory>
#include "../../include/Connection/MySQLConnection.hpp"

namespace UI {

// MySQL数据库管理对话框类
class MySQLDialog {
public:
    MySQLDialog();
    ~MySQLDialog() = default;

    // 显示对话框
    void showDialog(ftxui::ScreenInteractive& screen);
    
    // 设置连接回调
    void setConnectionCallback(std::function<void(const Connection::MySQLConnectionParams&)> callback) {
        connection_callback_ = callback;
    }

private:
    // 创建主对话框组件
    ftxui::Component createMainDialog();
    
    // 创建现代化标签页界面
    ftxui::Component createModernTabbedInterface();
    
    // 创建标签页组件
    ftxui::Component createConnectionTab();
    ftxui::Component createSettingsTab();
    
    // 创建现代化面板组件
    ftxui::Component createLeftNavigationPanel();
    ftxui::Component createCenterWorkPanel();
    ftxui::Component createRightPropertiesPanel();
    ftxui::Component createTopToolbar();
    ftxui::Component createBottomStatusBar();
    
    // 创建连接设置组件
    ftxui::Component createConnectionSettings();
    
    // 创建数据库操作组件
    ftxui::Component createDatabaseOperations();
    
    // 创建表操作组件
    ftxui::Component createTableOperations();
    
    // 创建查询执行组件
    ftxui::Component createQueryExecutor();
    
    // 创建增强的SQL编辑器
    ftxui::Component createEnhancedSQLEditor();
    
    // 创建数据表格视图
    ftxui::Component createDataTableView();
    
    // 创建连接管理器
    ftxui::Component createConnectionManager();
    
    // 创建表结构查看器
    ftxui::Component createTableStructureViewer();
    
    // 创建性能分析器
    ftxui::Component createPerformanceAnalyzer();
    
    // 处理连接按钮点击
    void onConnect();
    
    // 处理测试连接按钮点击
    void onTestConnection();
    
    // 处理断开连接按钮点击
    void onDisconnect();
    
    // 处理执行查询按钮点击
    void onExecuteQuery();
    
    // 处理创建表按钮点击
    void onCreateTable();
    
    // 处理删除表按钮点击
    void onDeleteTable();
    
    // 处理插入数据按钮点击
    void onInsertData();
    
    // 处理更新数据按钮点击
    void onUpdateData();
    
    // 处理删除数据按钮点击
    void onDeleteData();
    
    // 验证输入
    bool validateInput();
    
    // 刷新数据库列表
    void refreshDatabases();
    
    // 刷新表列表
    void refreshTables();
    
    // 显示查询结果
    void displayQueryResult(const Connection::MySQLQueryResult& result);

private:
    // 输入框组件
    ftxui::Component hostname_input_;
    ftxui::Component port_input_;
    ftxui::Component username_input_;
    ftxui::Component password_input_;
    ftxui::Component database_input_;
    ftxui::Component query_input_;
    
    // 选择器组件
    ftxui::Component database_selector_;
    ftxui::Component table_selector_;
    
    // 按钮组件
    ftxui::Component connect_button_;
    ftxui::Component disconnect_button_;
    ftxui::Component execute_query_button_;
    ftxui::Component create_table_button_;
    ftxui::Component delete_table_button_;
    ftxui::Component insert_data_button_;
    ftxui::Component update_data_button_;
    ftxui::Component delete_data_button_;
    
    // 状态文本
    std::string status_text_;
    std::string query_result_text_;
    
    // 输入值
    std::string hostname_;
    std::string port_;
    std::string username_;
    std::string password_;
    std::string database_;
    std::string query_;
    
    // 选择的值
    std::string selected_database_;
    std::string selected_table_;
    
    // 选择索引（用于Menu组件）
    int selected_database_index_;
    int selected_table_index_;
    
    // 列表数据
    std::vector<std::string> databases_;
    std::vector<std::string> tables_;
    
    // 对话框状态
    bool dialog_open_;
    bool connected_;
    
    // MySQL连接对象
    std::unique_ptr<Connection::MySQLConnection> mysql_connection_;
    
    // 回调函数
    std::function<void(const Connection::MySQLConnectionParams&)> connection_callback_;
    
    // 屏幕引用
    ftxui::ScreenInteractive* screen_;
    
    // 当前活动标签页
    int active_tab_;
    
    // 现代化界面相关变量
    int selected_connection_index_;
    int selected_database_tree_index_;
    int selected_table_tree_index_;
    std::vector<std::string> connection_history_;
    std::vector<std::string> query_history_;
    std::string current_sql_query_;
    bool show_query_history_;
    bool show_connection_manager_;
    
    // 数据表格相关
    std::vector<std::vector<std::string>> current_table_data_;
    std::vector<std::string> current_table_columns_;
    int current_page_;
    int page_size_;
    int total_rows_;
    
    // 连接管理相关
    struct ConnectionConfig {
        std::string name;
        std::string hostname;
        std::string port;
        std::string username;
        std::string password;
        std::string database;
        bool is_active;
    };
    std::vector<ConnectionConfig> saved_connections_;
    int selected_connection_config_index_;
    
    // 表结构相关
    struct TableColumn {
        std::string name;
        std::string type;
        std::string null;
        std::string key;
        std::string default_value;
        std::string extra;
    };
    std::vector<TableColumn> current_table_structure_;
};

} // namespace UI

#endif // MYSQL_DIALOG_HPP 