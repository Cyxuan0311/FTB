#include "ai/ContextBuilder.hpp"
#include <sstream>

#ifdef _WIN32
#define OS_TYPE "windows"
#elif __APPLE__
#define OS_TYPE "macos"
#else
#define OS_TYPE "linux"
#endif

namespace FTB {

std::string ContextBuilder::detectOS() {
    return OS_TYPE;
}

std::string ContextBuilder::buildSystemPrompt(
    const Environment& env,
    const std::vector<Message>& history,
    const std::string& tool_schemas_json,
    const std::string& custom_prompt)
{
    (void)history;
    if (!custom_prompt.empty()) {
        return custom_prompt;
    }

    std::ostringstream ss;
    ss << "You are an AI assistant for the FTB terminal file browser.\n";
    ss << "You help users manage files, directories, and answer questions.\n\n";
    ss << "Environment:\n";
    ss << "  OS: " << env.os_type << "\n";
    ss << "  Working directory: " << env.cwd << "\n";
    ss << "  Terminal: " << env.terminal_width << "x" << env.terminal_height << "\n\n";
    ss << "CRITICAL: You MUST plan ALL necessary steps in a single response.\n";
    ss << "Do NOT split work across multiple turns. If listing then copying,\n";
    ss << "include BOTH steps in the same \"steps\" array.\n\n";
    ss << "You MUST respond with valid JSON in one of two formats:\n\n";
    ss << "For questions (no file ops needed):\n";
    ss << "  {\"thought\": \"your answer\", \"steps\": []}\n\n";
    ss << "For file operations:\n";
    ss << "  {\"thought\": \"your reasoning\", \"steps\": [{\"action\": \"action_name\", \"params\": {\"key\": \"value\"}, \"description\": \"what this step does\"}]}\n\n";
    ss << "Available actions:\n\n";
    ss << "--- File Operations ---\n";
    ss << "- create_file: {\"path\": \"...\", \"content\": \"...\"}\n";
    ss << "- write_file: {\"path\": \"...\", \"content\": \"...\"}\n";
    ss << "- read_file: {\"path\": \"...\", \"start_line\": N, \"end_line\": N}\n";
    ss << "- delete_file: {\"path\": \"...\"}\n";
    ss << "- rename: {\"path\": \"...\", \"new_name\": \"...\"}\n";
    ss << "- move: {\"path\": \"...\", \"destination\": \"...\"}  (removes the source)\n";
    ss << "- copy: {\"path\": \"...\", \"destination\": \"...\"}  (keeps the source intact)\n";
    ss << "- get_file_info: {\"path\": \"...\"}  (returns type, size, permissions, mtime)\n";
    ss << "- search_files: {\"pattern\": \"glob\", \"path\": \"...\", \"max_results\": N}\n";
    ss << "- grep_content: {\"pattern\": \"regex\", \"path\": \"...\", \"include\": \"*.ext\"}\n";
    ss << "- diff_files: {\"file1\": \"...\", \"file2\": \"...\"}\n\n";
    ss << "--- Directory Operations ---\n";
    ss << "- create_directory: {\"path\": \"...\"}\n";
    ss << "- delete_directory: {\"path\": \"...\"}\n";
    ss << "- list_directory: {\"path\": \"...\", \"show_metadata\": true, \"max_items\": 50}\n";
    ss << "- navigate: {\"path\": \"...\"}\n";
    ss << "- go_back: {}  (navigate to previous directory)\n";
    ss << "- go_forward: {}  (navigate to next directory)\n";
    ss << "- go_home: {}  (navigate to home directory)\n\n";
    ss << "--- Clipboard & Selection ---\n";
    ss << "- copy_to_clipboard: {\"all\": true} or {\"pattern\": \"*.cpp\"} (glob or regex)\n";
    ss << "- cut_to_clipboard: {\"all\": true} or {\"pattern\": \"*.cpp\"} (glob or regex)\n";
    ss << "- paste_from_clipboard: {\"force_overwrite\": false}\n";
    ss << "- select_files: {\"all\": true} or {\"pattern\": \"*.cpp\"} (glob or regex)\n\n";
    ss << "--- Batch Operations ---\n";
    ss << "- batch_rename: {\"pattern\": \"foo.*\", \"replacement\": \"bar$&\"} (glob or regex)\n";
    ss << "- batch_delete: {\"paths\": [\"path1\", \"path2\", ...]}\n\n";
    ss << "--- Advanced ---\n";
    ss << "- create_symlink: {\"target\": \"...\", \"link_path\": \"...\", \"relative\": true}\n";
    ss << "- compress: {\"paths\": [...], \"archive_name\": \"...\", \"format\": \"tar.gz\"|\"zip\"}\n";
    ss << "- extract: {\"archive\": \"...\", \"destination\": \"...\"}\n";
    ss << "- open_file: {\"path\": \"...\"}\n";
    ss << "- execute: {\"command\": \"...\"}\n";
    ss << "\nTool schemas (use these exact function names and parameter keys):\n";
    ss << tool_schemas_json << "\n";

    return ss.str();
}

std::vector<Message> ContextBuilder::buildMessages(
    const std::string& system_prompt,
    const std::vector<Message>& history,
    const std::string& user_input)
{
    std::vector<Message> messages;
    messages.push_back({Message::System, system_prompt, ""});

    for (const auto& m : history) {
        messages.push_back(m);
    }

    messages.push_back({Message::User, user_input, ""});
    return messages;
}

} // namespace FTB
