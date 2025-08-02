#ifndef DIRECTORY_HISTORY_HPP
#define DIRECTORY_HISTORY_HPP

#include <string>
#include <stack>

/**
 * @class DirectoryHistory
 * @brief 目录浏览历史管理类，使用栈结构保存用户浏览过的目录路径
 * 
 * 该类用于实现类似浏览器前进/后退功能的目录导航历史记录，
 * 支持入栈、出栈、查看栈顶等基本操作
 */
class DirectoryHistory {
public:
    DirectoryHistory() = default;  ///< 默认构造函数
    
    /**
     * @brief 将目录路径压入历史栈
     * @param path 要保存的目录路径
     */
    void push(const std::string &path) {
        history.push(path);
    }
    
    /**
     * @brief 从历史栈中弹出并返回最近访问的目录
     * @return 最近访问的目录路径，如果栈为空则返回空字符串
     */
    std::string pop() {
        if (history.empty())
            return "";
        std::string topPath = history.top();
        history.pop();
        return topPath;
    }
    
    /**
     * @brief 获取最近访问的目录但不改变栈
     * @return 最近访问的目录路径，如果栈为空则返回空字符串
     */
    std::string top() const {
        return history.empty() ? "" : history.top();
    }
    
    /**
     * @brief 检查历史栈是否为空
     * @return true表示历史栈为空，false表示不为空
     */
    bool empty() const {
        return history.empty();
    }
    
private:
    std::stack<std::string> history;  ///< 使用STL栈保存目录历史记录
};

#endif // DIRECTORY_HISTORY_HPP
