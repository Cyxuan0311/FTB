#pragma once

#include <string>

namespace FTB {

extern const char* FTB_VERSION;

struct CLIArgs {
    bool show_help = false;
    bool show_version = false;
    bool log_enabled = false;
    std::string startup_dir;
    std::string config_path;
};

CLIArgs parse_args(int argc, char* argv[]);
void print_version();
void print_help(const char* prog_name);

} // namespace FTB
