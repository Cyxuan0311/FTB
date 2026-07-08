#include "config/CLIArgs.hpp"
#include <iostream>
#include <cstdlib>

namespace FTB {

const char* FTB_VERSION = "2.1.0";

void print_version() {
    std::cout << "FTB v" << FTB_VERSION << std::endl;
    std::cout << "A terminal file manager built by Frames" << std::endl;
}

void print_help(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [OPTIONS] [DIRECTORY]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help       Show this help message" << std::endl;
    std::cout << "  -v, --version    Show version information" << std::endl;
    std::cout << "  --config <PATH>  Specify config file path" << std::endl;
    std::cout << std::endl;
    std::cout << "Keybindings (inside FTB):" << std::endl;
    std::cout << "  j/k or Arrows    Navigate up/down" << std::endl;
    std::cout << "  h/l or Left/Right  Parent/Enter directory" << std::endl;
    std::cout << "  /                Search" << std::endl;
    std::cout << "  Ctrl+B           Command prefix mode (configurable via ftb.json)" << std::endl;
    std::cout << "  Escape           Clear search/close panel" << std::endl;
    std::cout << "  Ctrl+C           Quit" << std::endl;
}

CLIArgs parse_args(int argc, char* argv[]) {
    CLIArgs args;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            args.show_help = true;
        } else if (arg == "-v" || arg == "--version") {
            args.show_version = true;
        } else if (arg == "--config" && i + 1 < argc) {
            args.config_path = argv[++i];
        } else if (arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << std::endl;
            std::cerr << "Try '" << argv[0] << " --help' for more information." << std::endl;
            std::exit(1);
        } else {
            args.startup_dir = arg;
        }
    }
    return args;
}

} // namespace FTB
