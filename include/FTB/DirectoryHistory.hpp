#ifndef DIRECTORY_HISTORY_HPP
#define DIRECTORY_HISTORY_HPP

#include <string>
#include <stack>

class DirectoryHistory {
public:
    DirectoryHistory() = default;
    
    // 入栈
    void push(const std::string &path) {
        history.push(path);
    }
    
    // 出栈并返回历史目录，若栈空则返回空字符串
    std::string pop() {
        if (history.empty())
            return "";
        std::string topPath = history.top();
        history.pop();
        return topPath;
    }
    
    // 获取栈顶目录，不改变栈
    std::string top() const {
        return history.empty() ? "" : history.top();
    }
    
    bool empty() const {
        return history.empty();
    }
    
private:
    std::stack<std::string> history;
};

#endif // DIRECTORY_HISTORY_HPP
