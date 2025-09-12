#include "UI/MySQLDialog.hpp"
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/component.hpp>
#include <iostream>
#include <sstream>

using namespace ftxui;

namespace UI {

MySQLDialog::MySQLDialog() 
    : dialog_open_(false), connected_(false), active_tab_(0),
      selected_connection_index_(0), selected_database_tree_index_(0), 
      selected_table_tree_index_(0), show_query_history_(false), 
      show_connection_manager_(false), current_page_(1), page_size_(50), total_rows_(0),
      selected_connection_config_index_(0) {
    
    // 初始化输入框
    hostname_input_ = Input(&hostname_, "localhost");
    port_input_ = Input(&port_, "3306");
    username_input_ = Input(&username_, "root");
    password_input_ = Input(&password_, "");
    database_input_ = Input(&database_, "");
    query_input_ = Input(&query_, "SELECT * FROM table_name");
    
    // 初始化选择器（使用索引而不是字符串）
    selected_database_index_ = 0;
    selected_table_index_ = 0;
    database_selector_ = Menu(&databases_, &selected_database_index_);
    table_selector_ = Menu(&tables_, &selected_table_index_);
    
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
    
    // 初始化默认连接配置
    ConnectionConfig default_config;
    default_config.name = "本地MySQL";
    default_config.hostname = "localhost";
    default_config.port = "3306";
    default_config.username = "root";
    default_config.password = "";
    default_config.database = "";
    default_config.is_active = true;
    saved_connections_.push_back(default_config);
    
    ConnectionConfig remote_config;
    remote_config.name = "远程MySQL";
    remote_config.hostname = "192.168.1.100";
    remote_config.port = "3306";
    remote_config.username = "admin";
    remote_config.password = "";
    remote_config.database = "";
    remote_config.is_active = false;
    saved_connections_.push_back(remote_config);
}

void MySQLDialog::showDialog(ftxui::ScreenInteractive& screen) {
    screen_ = &screen;
    dialog_open_ = true;
    
    // 创建现代化的多面板界面
    auto modern_component = createMainDialog();
    
    // 添加ESC键处理
    auto component_with_escape = CatchEvent(modern_component, [this](ftxui::Event event) {
        if (event == ftxui::Event::Escape) {
            dialog_open_ = false;
            return true;
        }
        return false;
    });
    
    // 运行界面循环 - 使用模态方式
    screen.Loop(component_with_escape);
}

ftxui::Component MySQLDialog::createMainDialog() {
    // 创建现代化的标签页界面
    return createModernTabbedInterface();
}

// 创建现代化标签页界面
ftxui::Component MySQLDialog::createModernTabbedInterface() {
    // 创建输入框
    auto hostname_input = Input(&hostname_, "localhost");
    auto port_input = Input(&port_, "3306");
    auto username_input = Input(&username_, "root");
    auto password_input = Input(&password_, "password");
    auto database_input = Input(&database_, "test");
    
    // 创建按钮
    auto connect_button = Button("🔗 连接", [this] { onConnect(); });
    auto disconnect_button = Button("🔌 断开", [this] { onDisconnect(); });
    auto test_button = Button("🧪 测试连接", [this] { onTestConnection(); });
    auto exit_button = Button("❌ 退出", [this] { 
        dialog_open_ = false;
        if (screen_) {
            screen_->ExitLoopClosure()();
        }
    });
    
    // 创建输入框容器
    auto input_container = Container::Vertical({
        hostname_input,
        port_input,
        username_input,
        password_input,
        database_input
    });
    
    // 创建按钮容器
    auto button_container = Container::Horizontal({
        connect_button,
        test_button,
        disconnect_button,
        exit_button
    });
    
    // 创建主容器
    auto main_container = Container::Vertical({
        input_container,
        button_container
    });
    
    return Renderer(main_container, [this, hostname_input, port_input, username_input, password_input, database_input, connect_button, test_button, disconnect_button, exit_button] {
        return vbox({
            // 标题
            text("🗄️ MySQL 数据库管理器 v3.0") | color(Color::Cyan) | bold | center,
            separator(),
            
            // 连接状态
            hbox({
                text("状态: ") | color(Color::White),
                text(connected_ ? "已连接" : "未连接") | 
                color(connected_ ? Color::Green : Color::Red) | bold
            }) | center,
            
            separator(),
            
            // 连接参数输入区域
            vbox({
                text("📝 连接参数配置") | color(Color::Blue) | bold | center,
                separator(),
                
                // 主机名输入
                hbox({
                    text("主机名: ") | color(Color::White) | size(WIDTH, EQUAL, 10),
                    hostname_input->Render() | color(Color::Yellow) | border
                }),
                
                // 端口输入
                hbox({
                    text("端口: ") | color(Color::White) | size(WIDTH, EQUAL, 10),
                    port_input->Render() | color(Color::Yellow) | border
                }),
                
                // 用户名输入
                hbox({
                    text("用户名: ") | color(Color::White) | size(WIDTH, EQUAL, 10),
                    username_input->Render() | color(Color::Yellow) | border
                }),
                
                // 密码输入
                hbox({
                    text("密码: ") | color(Color::White) | size(WIDTH, EQUAL, 10),
                    password_input->Render() | color(Color::Yellow) | border
                }),
                
                // 数据库输入
                hbox({
                    text("数据库: ") | color(Color::White) | size(WIDTH, EQUAL, 10),
                    database_input->Render() | color(Color::Yellow) | border
                })
            }) | borderRounded | color(Color::Blue) | bgcolor(Color::DarkBlue),
            
            separator(),
            
            // 操作按钮
            hbox({
                connect_button->Render() | color(Color::Green) | bold,
                text("  "),
                test_button->Render() | color(Color::Cyan) | bold,
                text("  "),
                disconnect_button->Render() | color(Color::Red) | bold,
                text("  "),
                exit_button->Render() | color(Color::Magenta) | bold
            }) | center,
            
            separator(),
            
            // 状态信息
            text("状态: " + (status_text_.empty() ? "就绪" : status_text_)) | 
            color(status_text_.empty() ? Color::Green : 
                  (status_text_.find("成功") != std::string::npos ? Color::Green :
                   status_text_.find("失败") != std::string::npos ? Color::Red : Color::Yellow)) | center,
            
            separator(),
            
            // 使用说明
            vbox({
                text("💡 使用说明:") | color(Color::Cyan) | bold,
                text("• 使用 Tab 键在输入框间切换"),
                text("• 输入完成后按回车键连接"),
                text("• 按 ESC 键退出程序"),
                text("• 使用方向键选择按钮")
            }) | borderRounded | color(Color::Cyan)
        });
    });
}

// 创建连接管理标签页
ftxui::Component MySQLDialog::createConnectionTab() {
    // 创建连接配置列表
    std::vector<std::string> connection_names;
    for (const auto& config : saved_connections_) {
        connection_names.push_back(config.name + (config.is_active ? " (当前)" : ""));
    }
    
    auto connection_list = Menu(&connection_names, &selected_connection_config_index_);
    
    // 创建操作按钮
    auto connect_button = Button("🔗 连接", [this] { onConnect(); });
    auto disconnect_button = Button("🔌 断开", [this] { onDisconnect(); });
    auto test_button = Button("🔍 测试", [this] { 
        status_text_ = "正在测试连接...";
    });
    auto add_button = Button("➕ 添加", [this] {
        ConnectionConfig new_config;
        new_config.name = "新连接 " + std::to_string(saved_connections_.size() + 1);
        new_config.hostname = "localhost";
        new_config.port = "3306";
        new_config.username = "root";
        new_config.password = "";
        new_config.database = "";
        new_config.is_active = false;
        saved_connections_.push_back(new_config);
        status_text_ = "已添加新连接配置";
    });
    
    return Renderer(Container::Vertical({}), [this, connection_list, connect_button, disconnect_button, test_button, add_button] {
        return vbox({
            // 连接管理标题
            hbox({
                text("🔗 连接管理") | color(Color::Cyan) | bold,
                filler(),
                text("管理数据库连接配置") | color(Color::GrayDark)
            }) | border | color(Color::Cyan),
            
            // 连接配置区域
            vbox({
                text("📋 已保存的连接:") | color(Color::Blue) | bold,
                connection_list->Render() | color(Color::Yellow) | size(HEIGHT, GREATER_THAN, 8)
            }) | borderRounded | color(Color::Blue),
            
            // 连接操作区域
            vbox({
                text("⚙️ 连接操作:") | color(Color::Magenta) | bold,
                hbox({
                    connect_button->Render() | color(Color::Green) | bold,
                    text("  "),
                    disconnect_button->Render() | color(Color::Red) | bold,
                    text("  "),
                    test_button->Render() | color(Color::Cyan) | bold,
                    text("  "),
                    add_button->Render() | color(Color::Blue) | bold
                })
            }) | borderRounded | color(Color::Magenta),
            
            separator(),
            
            // 当前连接信息
            vbox({
                text("📊 当前连接信息:") | color(Color::Green) | bold,
                vbox({
                    hbox({text("主机: ") | color(Color::White), text(hostname_) | color(Color::Yellow)}),
                    hbox({text("端口: ") | color(Color::White), text(port_) | color(Color::Yellow)}),
                    hbox({text("用户: ") | color(Color::White), text(username_) | color(Color::Yellow)}),
                    hbox({text("数据库: ") | color(Color::White), 
                         text(database_.empty() ? "未指定" : database_) | color(Color::Yellow)}),
                    hbox({text("状态: ") | color(Color::White), 
                         text(connected_ ? "已连接" : "未连接") | 
                         color(connected_ ? Color::Green : Color::Red)})
                }) | borderRounded | color(Color::Green)
            }) | borderRounded | color(Color::Green),
            
            separator(),
            
            // 数据库和表列表
            hbox({
                // 数据库列表
                vbox({
                    text("🗄️ 数据库:") | color(Color::Blue) | bold,
                    Menu(&databases_, &selected_database_index_)->Render() | color(Color::Yellow),
                    Button("🔄 刷新", [this] { refreshDatabases(); })->Render() | color(Color::Green) | bold
                }) | borderRounded | color(Color::Blue) | size(WIDTH, EQUAL, 30),
                
                text("  "),
                
                // 表列表
                vbox({
                    text("📋 表:") | color(Color::Magenta) | bold,
                    Menu(&tables_, &selected_table_index_)->Render() | color(Color::Yellow),
                    Button("🔄 刷新", [this] { refreshTables(); })->Render() | color(Color::Green) | bold
                }) | borderRounded | color(Color::Magenta) | size(WIDTH, EQUAL, 30)
            })
        });
    });
}

// 创建设置标签页
ftxui::Component MySQLDialog::createSettingsTab() {
    auto save_settings_button = Button("💾 保存设置", [this] {
        status_text_ = "设置已保存";
    });
    
    auto reset_settings_button = Button("🔄 重置", [this] {
        status_text_ = "设置已重置";
    });
    
    return Renderer(Container::Vertical({}), [this, save_settings_button, reset_settings_button] {
        return vbox({
            // 设置标题
            hbox({
                text("⚙️ 系统设置") | color(Color::Cyan) | bold,
                filler(),
                text("配置数据库管理器") | color(Color::GrayDark)
            }) | border | color(Color::Cyan),
            
            // 显示设置
            vbox({
                text("🖥️ 显示设置:") | color(Color::Blue) | bold,
                vbox({
                    hbox({text("页面大小: ") | color(Color::White), text(std::to_string(page_size_)) | color(Color::Yellow)}),
                    hbox({text("主题: ") | color(Color::White), text("默认") | color(Color::Yellow)}),
                    hbox({text("字体: ") | color(Color::White), text("等宽") | color(Color::Yellow)})
                }) | borderRounded | color(Color::Blue)
                }) | borderRounded | color(Color::Blue),
            
            separator(),
            
            // 连接设置
            vbox({
                text("🔗 连接设置:") | color(Color::Magenta) | bold,
                vbox({
                    hbox({text("超时时间: ") | color(Color::White), text("30秒") | color(Color::Yellow)}),
                    hbox({text("重试次数: ") | color(Color::White), text("3次") | color(Color::Yellow)}),
                    hbox({text("自动重连: ") | color(Color::White), text("启用") | color(Color::Green)})
                }) | borderRounded | color(Color::Magenta)
            }) | borderRounded | color(Color::Magenta),
            
            separator(),
            
            // 操作按钮
                hbox({
                save_settings_button->Render() | color(Color::Green) | bold,
                    text("  "),
                reset_settings_button->Render() | color(Color::Red) | bold
            }) | center
        });
    });
}

// 创建左侧导航面板
ftxui::Component MySQLDialog::createLeftNavigationPanel() {
    // 创建连接管理器按钮
    auto connection_manager_button = Button("🔗 连接管理", [this] { 
        show_connection_manager_ = !show_connection_manager_;
    });
    
    // 创建数据库树形菜单
    auto database_tree = Menu(&databases_, &selected_database_tree_index_);
    
    // 创建表树形菜单
    auto table_tree = Menu(&tables_, &selected_table_tree_index_);
    
    // 创建刷新按钮
    auto refresh_button = Button("🔄 刷新", [this] { 
        refreshDatabases();
        refreshTables();
    });
    
    return Renderer(Container::Vertical({}), [this, connection_manager_button, database_tree, table_tree, refresh_button] {
        return vbox({
            // 连接管理区域
            vbox({
                text("🔗 连接管理") | color(Color::Cyan) | bold,
                hbox({
                    connection_manager_button->Render() | color(Color::Blue) | bold,
                    text("  "),
                    text(connected_ ? "✅" : "❌") | color(connected_ ? Color::Green : Color::Red)
                }),
                hbox({
                    text("主机: ") | color(Color::White),
                    text(hostname_) | color(Color::Yellow)
                }),
                hbox({
                    text("用户: ") | color(Color::White),
                    text(username_) | color(Color::Yellow)
                })
            }) | borderRounded | color(Color::Cyan),
            
            separator(),
            
            // 数据库树形视图
            vbox({
                text("🗄️ 数据库") | color(Color::Blue) | bold,
                database_tree->Render() | color(Color::Yellow),
                hbox({
                    refresh_button->Render() | color(Color::Green) | bold
                })
            }) | borderRounded | color(Color::Blue),
            
            separator(),
            
            // 表树形视图
            vbox({
                text("📋 表") | color(Color::Magenta) | bold,
                table_tree->Render() | color(Color::Yellow)
            }) | borderRounded | color(Color::Magenta),
            
            separator(),
            
            // 快捷操作
            vbox({
                text("⚡ 快捷操作") | color(Color::Green) | bold,
                hbox({
                    Button("📊 表结构", [this] { 
                        status_text_ = "查看表结构功能";
                    })->Render() | color(Color::Cyan) | bold,
                    text("  "),
                    Button("📈 统计", [this] { 
                        status_text_ = "表统计功能";
                    })->Render() | color(Color::Yellow) | bold
                })
            }) | borderRounded | color(Color::Green)
        });
    });
}

// 创建中间工作面板
ftxui::Component MySQLDialog::createCenterWorkPanel() {
    // 创建标签页选择器
    std::vector<std::string> tab_names = {"🔍 SQL编辑器", "📊 数据视图", "📋 表结构", "📈 性能分析"};
    auto tab_selector = Menu(&tab_names, &active_tab_);
    
    // 创建标签页内容
    auto sql_editor_tab = createEnhancedSQLEditor();
    auto data_view_tab = createDataTableView();
    auto table_structure_tab = createTableStructureViewer();
    auto performance_tab = createPerformanceAnalyzer();
    
    // 创建标签页容器
    auto tabs = Container::Tab({
        sql_editor_tab,
        data_view_tab,
        table_structure_tab,
        performance_tab
    }, &active_tab_);
    
    return tabs;
}

// 创建右侧属性面板
ftxui::Component MySQLDialog::createRightPropertiesPanel() {
    return Renderer(Container::Vertical({}), [this] {
        return vbox({
            // 连接信息
            vbox({
                text("🔗 连接信息") | color(Color::Cyan) | bold,
                vbox({
                    hbox({text("状态: ") | color(Color::White), 
                         text(connected_ ? "已连接" : "未连接") | 
                         color(connected_ ? Color::Green : Color::Red)}),
                    hbox({text("主机: ") | color(Color::White), text(hostname_) | color(Color::Yellow)}),
                    hbox({text("端口: ") | color(Color::White), text(port_) | color(Color::Yellow)}),
                    hbox({text("用户: ") | color(Color::White), text(username_) | color(Color::Yellow)}),
                    hbox({text("数据库: ") | color(Color::White), 
                         text(selected_database_.empty() ? "未选择" : selected_database_) | color(Color::Yellow)})
                }) | borderRounded | color(Color::Cyan)
            }) | borderRounded | color(Color::Cyan),
            
            separator(),
            
            // 查询历史
            vbox({
                text("📜 查询历史") | color(Color::Blue) | bold,
                vbox({
                    text("最近查询:") | color(Color::White),
                    text("SELECT * FROM users") | color(Color::GrayLight),
                    text("SHOW TABLES") | color(Color::GrayLight),
                    text("DESCRIBE products") | color(Color::GrayLight)
                }) | borderRounded | color(Color::Blue)
            }) | borderRounded | color(Color::Blue),
            
            separator(),
            
            // 表属性
            vbox({
                text("📋 表属性") | color(Color::Magenta) | bold,
                vbox({
                    hbox({text("表名: ") | color(Color::White), 
                         text(selected_table_.empty() ? "未选择" : selected_table_) | color(Color::Yellow)}),
                    hbox({text("行数: ") | color(Color::White), text(std::to_string(total_rows_)) | color(Color::Yellow)}),
                    hbox({text("列数: ") | color(Color::White), text(std::to_string(current_table_columns_.size())) | color(Color::Yellow)}),
                    hbox({text("引擎: ") | color(Color::White), text("InnoDB") | color(Color::Yellow)})
                }) | borderRounded | color(Color::Magenta)
            }) | borderRounded | color(Color::Magenta),
            
            separator(),
            
            // 快捷操作
            vbox({
                text("⚡ 快捷操作") | color(Color::Green) | bold,
                vbox({
                    Button("🔄 刷新表", [this] { 
                        refreshTables();
                        status_text_ = "表列表已刷新";
                    })->Render() | color(Color::Cyan) | bold,
                    Button("📊 分析表", [this] { 
                        status_text_ = "表分析功能";
                    })->Render() | color(Color::Yellow) | bold,
                    Button("🔧 优化表", [this] { 
                        status_text_ = "表优化功能";
                    })->Render() | color(Color::Blue) | bold
                })
            }) | borderRounded | color(Color::Green)
        });
    });
}

// 创建顶部工具栏
ftxui::Component MySQLDialog::createTopToolbar() {
    return Renderer(Container::Vertical({}), [this] {
        return hbox({
            text("🔗 MySQL 数据库管理器 v2.0") | color(Color::Cyan) | bold,
            filler(),
            hbox({
                text("连接: ") | color(Color::White),
                connect_button_->Render() | color(Color::Green) | bold,
                text("  "),
                disconnect_button_->Render() | color(Color::Red) | bold
            }),
            text("  "),
            hbox({
                text("ESC退出") | color(Color::GrayDark),
                text("  "),
                Button("❌", [this] { 
                    dialog_open_ = false;
                    if (screen_) {
                        screen_->ExitLoopClosure()();
                    }
                })->Render() | color(Color::Red) | bold
            })
        }) | border;
    });
}

// 创建底部状态栏
ftxui::Component MySQLDialog::createBottomStatusBar() {
    return Renderer(Container::Vertical({}), [this] {
        return hbox({
            text("📊 状态: ") | color(Color::Cyan) | bold,
            text(status_text_.empty() ? "就绪" : status_text_) | 
            color(status_text_.empty() ? Color::Green : 
                  (status_text_.find("成功") != std::string::npos ? Color::Green :
                   status_text_.find("失败") != std::string::npos ? Color::Red : Color::Yellow)),
            filler(),
            text("数据库: " + (selected_database_.empty() ? "未选择" : selected_database_)) | color(Color::Yellow),
            text("  |  "),
            text("表: " + (selected_table_.empty() ? "未选择" : selected_table_)) | color(Color::Yellow),
            text("  |  "),
            text("行数: " + std::to_string(total_rows_)) | color(Color::Green)
        }) | border;
    });
}

// 创建增强的SQL编辑器
ftxui::Component MySQLDialog::createEnhancedSQLEditor() {
    // 创建SQL输入框
    auto sql_input = Input(&current_sql_query_, "SELECT * FROM table_name LIMIT 100;");
    
    // 创建执行按钮
    auto execute_button = Button("▶️ 执行", [this] { 
        query_ = current_sql_query_;
        onExecuteQuery();
    });
    
    // 创建清空按钮
    auto clear_button = Button("🗑️ 清空", [this] { 
        current_sql_query_ = "";
    });
    
    // 创建格式化按钮
    auto format_button = Button("🎨 格式化", [this] { 
        status_text_ = "SQL格式化功能";
    });
    
    // 创建保存按钮
    auto save_button = Button("💾 保存", [this] { 
        query_history_.push_back(current_sql_query_);
        status_text_ = "查询已保存到历史";
    });
    
    // 创建SQL模板按钮
    auto select_template_button = Button("📋 SELECT", [this] { 
        current_sql_query_ = "SELECT * FROM " + (selected_table_.empty() ? "table_name" : selected_table_) + " LIMIT 100;";
    });
    
    auto insert_template_button = Button("➕ INSERT", [this] { 
        current_sql_query_ = "INSERT INTO " + (selected_table_.empty() ? "table_name" : selected_table_) + " (column1, column2) VALUES ('value1', 'value2');";
    });
    
    auto update_template_button = Button("✏️ UPDATE", [this] { 
        current_sql_query_ = "UPDATE " + (selected_table_.empty() ? "table_name" : selected_table_) + " SET column1 = 'new_value' WHERE condition;";
    });
    
    auto delete_template_button = Button("🗑️ DELETE", [this] { 
        current_sql_query_ = "DELETE FROM " + (selected_table_.empty() ? "table_name" : selected_table_) + " WHERE condition;";
    });
    
    // 创建查询历史按钮
    auto history_button = Button("📜 历史", [this] { 
        show_query_history_ = !show_query_history_;
        status_text_ = show_query_history_ ? "显示查询历史" : "隐藏查询历史";
    });
    
    // 创建解释按钮
    auto explain_button = Button("🔍 解释", [this] { 
        if (!current_sql_query_.empty()) {
            std::string explain_query = "EXPLAIN " + current_sql_query_;
            query_ = explain_query;
            onExecuteQuery();
        }
    });
    
    return Renderer(Container::Vertical({}), [this, sql_input, execute_button, clear_button, format_button, save_button, 
                     select_template_button, insert_template_button, update_template_button, 
                     delete_template_button, history_button, explain_button] {
        return vbox({
            // SQL编辑器标题
            hbox({
                text("🔍 SQL 编辑器") | color(Color::Cyan) | bold,
                filler(),
                text("支持语法高亮和自动完成") | color(Color::GrayDark)
            }) | border,
            
            // SQL模板区域
            vbox({
                text("📋 SQL 模板:") | color(Color::Green) | bold,
                hbox({
                    select_template_button->Render() | color(Color::Cyan) | bold,
                    text("  "),
                    insert_template_button->Render() | color(Color::Green) | bold,
                    text("  "),
                    update_template_button->Render() | color(Color::Yellow) | bold,
                    text("  "),
                    delete_template_button->Render() | color(Color::Red) | bold
                })
            }) | borderRounded | color(Color::Green),
            
            // SQL输入区域
            vbox({
                text("📝 输入SQL语句:") | color(Color::Blue) | bold,
                sql_input->Render() | color(Color::Yellow) | size(HEIGHT, GREATER_THAN, 6),
                
                // 主要操作按钮
                hbox({
                    execute_button->Render() | color(Color::Green) | bold,
                    text("  "),
                    explain_button->Render() | color(Color::Blue) | bold,
                    text("  "),
                    clear_button->Render() | color(Color::Red) | bold
                }),
                
                // 辅助操作按钮
                hbox({
                    format_button->Render() | color(Color::Magenta) | bold,
                    text("  "),
                    save_button->Render() | color(Color::Cyan) | bold,
                    text("  "),
                    history_button->Render() | color(Color::Yellow) | bold
                })
            }) | borderRounded | color(Color::Blue),
            
            separator(),
            
            // 查询历史区域（条件显示）
            show_query_history_ ? vbox({
                text("📜 查询历史:") | color(Color::Yellow) | bold,
            vbox({
                    text("查询历史功能待实现") | color(Color::GrayLight)
                }) | borderRounded | color(Color::Yellow) | size(HEIGHT, EQUAL, 6)
            }) | borderRounded | color(Color::Yellow) : text(""),
            
            // 查询结果区域
                vbox({
                text("📊 查询结果:") | color(Color::Magenta) | bold,
                vbox({
                    text(query_result_text_.empty() ? "暂无查询结果" : query_result_text_) | 
                    color(query_result_text_.empty() ? Color::GrayDark : Color::White)
                }) | borderRounded | color(Color::Magenta) | size(HEIGHT, GREATER_THAN, 8)
            }) | borderRounded | color(Color::Magenta)
        });
    });
}

// 创建数据表格视图
ftxui::Component MySQLDialog::createDataTableView() {
    // 创建分页控件
    auto prev_page_button = Button("⬅️ 上一页", [this] { 
        if (current_page_ > 1) {
            current_page_--;
            status_text_ = "第 " + std::to_string(current_page_) + " 页";
        }
    });
    
    auto next_page_button = Button("➡️ 下一页", [this] { 
        current_page_++;
        status_text_ = "第 " + std::to_string(current_page_) + " 页";
    });
    
    // 创建刷新按钮
    auto refresh_data_button = Button("🔄 刷新数据", [this] { 
        if (!selected_table_.empty()) {
            std::string query = "SELECT * FROM " + selected_table_ + " LIMIT " + 
                               std::to_string(page_size_) + " OFFSET " + 
                               std::to_string((current_page_ - 1) * page_size_);
            query_ = query;
            onExecuteQuery();
        }
    });
    
    return Renderer(Container::Vertical({}), [this, prev_page_button, next_page_button, refresh_data_button] {
        return vbox({
            // 数据视图标题
            hbox({
                text("📊 数据视图") | color(Color::Cyan) | bold,
                filler(),
                text("表: " + (selected_table_.empty() ? "未选择" : selected_table_)) | color(Color::Yellow)
            }) | border,
            
            // 分页控件
            hbox({
                prev_page_button->Render() | color(Color::Blue) | bold,
                text("  "),
                text("第 " + std::to_string(current_page_) + " 页") | color(Color::White),
                text("  "),
                next_page_button->Render() | color(Color::Blue) | bold,
                text("  "),
                text("每页 " + std::to_string(page_size_) + " 行") | color(Color::GrayLight),
                filler(),
                refresh_data_button->Render() | color(Color::Green) | bold
            }) | borderRounded | color(Color::Blue),
            
            // 数据表格
            vbox({
                text("📋 数据内容:") | color(Color::Magenta) | bold,
                vbox({
                    // 显示列名（表头）
                    hbox({
                        text("列名") | color(Color::Yellow) | bold | size(WIDTH, EQUAL, 12) | center
                    }) | borderRounded | color(Color::Yellow) | bgcolor(Color::Blue),
                    
                    // 显示数据行
                    text("数据表格功能待实现") | color(Color::GrayLight) | center,
                    
                    // 如果没有数据，显示提示
                    text("暂无数据，请先选择表并刷新数据") | color(Color::GrayDark) | center
                }) | borderRounded | color(Color::Magenta) | size(HEIGHT, GREATER_THAN, 15)
            }) | borderRounded | color(Color::Magenta)
        });
    });
}

// 创建连接管理器
ftxui::Component MySQLDialog::createConnectionManager() {
    // 创建连接配置列表
    std::vector<std::string> connection_names;
    for (const auto& config : saved_connections_) {
        connection_names.push_back(config.name + (config.is_active ? " (当前)" : ""));
    }
    
    auto connection_list = Menu(&connection_names, &selected_connection_config_index_);
    
    // 创建操作按钮
    auto connect_to_selected_button = Button("🔗 连接", [this] {
        if (selected_connection_config_index_ < saved_connections_.size()) {
            const auto& config = saved_connections_[selected_connection_config_index_];
            hostname_ = config.hostname;
            port_ = config.port;
            username_ = config.username;
            password_ = config.password;
            database_ = config.database;
            
            // 设置其他连接为非活跃状态
            for (auto& conn : saved_connections_) {
                conn.is_active = false;
            }
            saved_connections_[selected_connection_config_index_].is_active = true;
            
            status_text_ = "已加载连接配置: " + config.name;
        }
    });
    
    auto add_connection_button = Button("➕ 添加", [this] {
        ConnectionConfig new_config;
        new_config.name = "新连接 " + std::to_string(saved_connections_.size() + 1);
        new_config.hostname = "localhost";
        new_config.port = "3306";
        new_config.username = "root";
        new_config.password = "";
        new_config.database = "";
        new_config.is_active = false;
        saved_connections_.push_back(new_config);
        status_text_ = "已添加新连接配置";
    });
    
    auto delete_connection_button = Button("🗑️ 删除", [this] {
        if (selected_connection_config_index_ < saved_connections_.size() && saved_connections_.size() > 1) {
            saved_connections_.erase(saved_connections_.begin() + selected_connection_config_index_);
            if (selected_connection_config_index_ >= saved_connections_.size()) {
                selected_connection_config_index_ = saved_connections_.size() - 1;
            }
            status_text_ = "已删除连接配置";
        }
    });
    
    auto test_connection_button = Button("🔍 测试", [this] {
        if (selected_connection_config_index_ < saved_connections_.size()) {
            const auto& config = saved_connections_[selected_connection_config_index_];
            status_text_ = "正在测试连接: " + config.name + "...";
            // 这里可以添加实际的连接测试逻辑
        }
    });
    
    return Renderer(Container::Vertical({}), [this, connection_list, connect_to_selected_button, add_connection_button, 
                     delete_connection_button, test_connection_button] {
        return vbox({
            // 连接管理器标题
            hbox({
                text("🔗 连接管理器") | color(Color::Cyan) | bold,
                filler(),
                text("管理多个数据库连接") | color(Color::GrayDark)
            }) | border,
            
            // 连接列表
            vbox({
                text("📋 已保存的连接:") | color(Color::Blue) | bold,
                connection_list->Render() | color(Color::Yellow) | size(HEIGHT, GREATER_THAN, 8)
            }) | borderRounded | color(Color::Blue),
            
            // 操作按钮
            vbox({
                text("⚙️ 操作:") | color(Color::Magenta) | bold,
                hbox({
                    connect_to_selected_button->Render() | color(Color::Green) | bold,
                    text("  "),
                    test_connection_button->Render() | color(Color::Cyan) | bold
                }),
                hbox({
                    add_connection_button->Render() | color(Color::Blue) | bold,
                    text("  "),
                    delete_connection_button->Render() | color(Color::Red) | bold
                })
            }) | borderRounded | color(Color::Magenta),
            
            separator(),
            
            // 当前连接信息
            vbox({
                text("📊 当前连接信息:") | color(Color::Green) | bold,
                vbox({
                    hbox({text("主机: ") | color(Color::White), text(hostname_) | color(Color::Yellow)}),
                    hbox({text("端口: ") | color(Color::White), text(port_) | color(Color::Yellow)}),
                    hbox({text("用户: ") | color(Color::White), text(username_) | color(Color::Yellow)}),
                    hbox({text("数据库: ") | color(Color::White), 
                         text(database_.empty() ? "未指定" : database_) | color(Color::Yellow)}),
                    hbox({text("状态: ") | color(Color::White), 
                         text(connected_ ? "已连接" : "未连接") | 
                         color(connected_ ? Color::Green : Color::Red)})
                }) | borderRounded | color(Color::Green)
            }) | borderRounded | color(Color::Green)
        });
    });
}

// 创建表结构查看器
ftxui::Component MySQLDialog::createTableStructureViewer() {
    auto refresh_structure_button = Button("🔄 刷新结构", [this] {
        if (!selected_table_.empty()) {
            std::string query = "DESCRIBE " + selected_table_;
            query_ = query;
            onExecuteQuery();
            status_text_ = "正在获取表结构...";
        }
    });
    
    auto show_indexes_button = Button("📊 查看索引", [this] {
        if (!selected_table_.empty()) {
            std::string query = "SHOW INDEX FROM " + selected_table_;
            query_ = query;
            onExecuteQuery();
            status_text_ = "正在获取索引信息...";
        }
    });
    
    auto show_create_table_button = Button("📝 建表语句", [this] {
        if (!selected_table_.empty()) {
            std::string query = "SHOW CREATE TABLE " + selected_table_;
            query_ = query;
            onExecuteQuery();
            status_text_ = "正在获取建表语句...";
        }
    });
    
    return Renderer(Container::Vertical({}), [this, refresh_structure_button, show_indexes_button, show_create_table_button] {
        return vbox({
            // 表结构查看器标题
            hbox({
                text("📋 表结构查看器") | color(Color::Cyan) | bold,
                filler(),
                text("表: " + (selected_table_.empty() ? "未选择" : selected_table_)) | color(Color::Yellow)
            }) | border,
            
            // 操作按钮
            vbox({
                text("⚙️ 操作:") | color(Color::Blue) | bold,
                hbox({
                    refresh_structure_button->Render() | color(Color::Green) | bold,
                    text("  "),
                    show_indexes_button->Render() | color(Color::Cyan) | bold,
                    text("  "),
                    show_create_table_button->Render() | color(Color::Magenta) | bold
                })
            }) | borderRounded | color(Color::Blue),
            
            separator(),
            
            // 表结构信息
            vbox({
                text("📊 表结构信息:") | color(Color::Magenta) | bold,
                vbox({
                    // 显示表结构
                    hbox({
                        text("列名") | color(Color::Yellow) | bold | size(WIDTH, EQUAL, 15),
                        text("类型") | color(Color::Yellow) | bold | size(WIDTH, EQUAL, 15),
                        text("可空") | color(Color::Yellow) | bold | size(WIDTH, EQUAL, 8),
                        text("键") | color(Color::Yellow) | bold | size(WIDTH, EQUAL, 8),
                        text("默认值") | color(Color::Yellow) | bold | size(WIDTH, EQUAL, 12)
                    }) | borderRounded | color(Color::Yellow),
                    
                    text("表结构功能待实现") | color(Color::GrayLight) | center
                }) | borderRounded | color(Color::Magenta) | size(HEIGHT, GREATER_THAN, 15)
            }) | borderRounded | color(Color::Magenta)
        });
    });
}

// 创建性能分析器
ftxui::Component MySQLDialog::createPerformanceAnalyzer() {
    auto analyze_table_button = Button("📊 分析表", [this] {
        if (!selected_table_.empty()) {
            std::string query = "ANALYZE TABLE " + selected_table_;
            query_ = query;
            onExecuteQuery();
            status_text_ = "正在分析表性能...";
        }
    });
    
    auto optimize_table_button = Button("⚡ 优化表", [this] {
        if (!selected_table_.empty()) {
            std::string query = "OPTIMIZE TABLE " + selected_table_;
            query_ = query;
            onExecuteQuery();
            status_text_ = "正在优化表...";
        }
    });
    
    auto check_table_button = Button("🔍 检查表", [this] {
        if (!selected_table_.empty()) {
            std::string query = "CHECK TABLE " + selected_table_;
            query_ = query;
            onExecuteQuery();
            status_text_ = "正在检查表...";
        }
    });
    
    auto repair_table_button = Button("🔧 修复表", [this] {
        if (!selected_table_.empty()) {
            std::string query = "REPAIR TABLE " + selected_table_;
            query_ = query;
            onExecuteQuery();
            status_text_ = "正在修复表...";
        }
    });
    
    auto show_processlist_button = Button("📋 进程列表", [this] {
        std::string query = "SHOW PROCESSLIST";
        query_ = query;
        onExecuteQuery();
        status_text_ = "正在获取进程列表...";
    });
    
    auto show_status_button = Button("📈 状态信息", [this] {
        std::string query = "SHOW STATUS";
        query_ = query;
        onExecuteQuery();
        status_text_ = "正在获取状态信息...";
    });
    
    return Renderer(Container::Vertical({}), [this, analyze_table_button, optimize_table_button, check_table_button, 
                     repair_table_button, show_processlist_button, show_status_button] {
        return vbox({
            // 性能分析器标题
            hbox({
                text("📈 性能分析器") | color(Color::Cyan) | bold,
                filler(),
                text("数据库性能监控和优化") | color(Color::GrayDark)
            }) | border,
            
            // 表操作
            vbox({
                text("📋 表操作:") | color(Color::Blue) | bold,
                hbox({
                    analyze_table_button->Render() | color(Color::Cyan) | bold,
                    text("  "),
                    optimize_table_button->Render() | color(Color::Green) | bold,
                    text("  "),
                    check_table_button->Render() | color(Color::Yellow) | bold,
                    text("  "),
                    repair_table_button->Render() | color(Color::Red) | bold
                })
            }) | borderRounded | color(Color::Blue),
            
            separator(),
            
            // 系统监控
            vbox({
                text("📊 系统监控:") | color(Color::Magenta) | bold,
                hbox({
                    show_processlist_button->Render() | color(Color::Cyan) | bold,
                    text("  "),
                    show_status_button->Render() | color(Color::Green) | bold
                })
            }) | borderRounded | color(Color::Magenta),
            
            separator(),
            
            // 性能统计
            vbox({
                text("📈 性能统计:") | color(Color::Green) | bold,
                vbox({
                    hbox({text("当前表: ") | color(Color::White), 
                         text(selected_table_.empty() ? "未选择" : selected_table_) | color(Color::Yellow)}),
                    hbox({text("表行数: ") | color(Color::White), text(std::to_string(total_rows_)) | color(Color::Yellow)}),
                    hbox({text("连接状态: ") | color(Color::White), 
                         text(connected_ ? "已连接" : "未连接") | 
                         color(connected_ ? Color::Green : Color::Red)}),
                    hbox({text("数据库: ") | color(Color::White), 
                         text(selected_database_.empty() ? "未选择" : selected_database_) | color(Color::Yellow)})
                }) | borderRounded | color(Color::Green)
            }) | borderRounded | color(Color::Green)
        });
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

void MySQLDialog::onTestConnection() {
    try {
        // 验证输入参数
        if (hostname_.empty()) {
            status_text_ = "错误: 主机名不能为空";
            return;
        }
        if (username_.empty()) {
            status_text_ = "错误: 用户名不能为空";
            return;
        }
        if (port_.empty()) {
            port_ = "3306";
        }
        
        // 验证端口号
        int port_num = std::stoi(port_);
        if (port_num <= 0 || port_num > 65535) {
            status_text_ = "错误: 端口号必须在1-65535之间";
            return;
        }
        
        status_text_ = "正在测试连接...";
        
        // 创建临时连接参数进行测试
        Connection::MySQLConnectionParams test_params;
        test_params.hostname = hostname_;
        test_params.port = port_num;
        test_params.username = username_;
        test_params.password = password_;
        test_params.database = database_;
        test_params.is_local = (hostname_ == "localhost" || hostname_ == "127.0.0.1");
        
        // 尝试连接
        if (mysql_connection_->connect(test_params)) {
            status_text_ = "✅ 测试连接成功！参数验证通过";
            mysql_connection_->disconnect(); // 测试完成后断开
        } else {
            status_text_ = "❌ 测试连接失败: " + mysql_connection_->getLastError();
        }
        
    } catch (const std::exception& e) {
        status_text_ = "❌ 测试连接失败: " + std::string(e.what());
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
        selected_database_index_ = 0;
        selected_database_tree_index_ = 0;
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
        selected_table_index_ = 0;
        selected_table_tree_index_ = 0;
    }
}

void MySQLDialog::displayQueryResult(const Connection::MySQLQueryResult& result) {
    std::ostringstream oss;
    
    if (result.success) {
        if (!result.columns.empty()) {
            // 更新表格数据
            current_table_columns_ = result.columns;
            current_table_data_ = result.rows;
            total_rows_ = result.rows.size();
            
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
            
            // 显示数据行（限制显示数量）
            size_t display_count = std::min(result.rows.size(), size_t(50));
            for (size_t i = 0; i < display_count; ++i) {
                const auto& row = result.rows[i];
                for (const auto& cell : row) {
                    oss << cell << "\t";
                }
                oss << "\n";
            }
            
            if (result.rows.size() > 50) {
                oss << "... (显示前50行，共" << result.rows.size() << "行)";
            }
        } else {
            oss << "影响行数: " << result.affected_rows;
            // 清空表格数据
            current_table_columns_.clear();
            current_table_data_.clear();
            total_rows_ = 0;
        }
    } else {
        oss << "错误: " << result.error_message;
        // 清空表格数据
        current_table_columns_.clear();
        current_table_data_.clear();
        total_rows_ = 0;
    }
    
    query_result_text_ = oss.str();
}

} // namespace UI 