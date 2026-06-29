#!/bin/bash

set -e

PROJECT_NAME="FTB"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
DIM='\033[2m'
NC='\033[0m'

# ====================================================================
# Helper functions
# ====================================================================
print_banner() {
    echo ""
    echo -e "${CYAN}${BOLD}  ╔═══════════════════════════════╗${NC}"
    echo -e "${CYAN}${BOLD}  ║       FTB Build System        ║${NC}"
    echo -e "${CYAN}${BOLD}  ╚═══════════════════════════════╝${NC}"
    echo ""
}

print_step() { echo -e "\n${GREEN}${BOLD}►${NC} ${BOLD}$1${NC}"; }
print_info() { echo -e "  ${DIM}$1${NC}"; }
print_error() { echo -e "  ${RED}$1${NC}"; }

ask_yes_no() {
    local prompt="$1"
    local default="${2:-n}"
    local suffix
    [[ "$default" == "y" ]] && suffix="[Y/n]" || suffix="[y/N]"
    while true; do
        read -p "$(echo -e "  ${CYAN}?${NC} ${prompt} ${suffix}: ")" answer
        answer="${answer:-$default}"
        case "$answer" in
            [Yy]*) return 0 ;;
            [Nn]*) return 1 ;;
            *) echo -e "  ${DIM}Please answer y or n${NC}" ;;
        esac
    done
}

ask_option() {
    local prompt="$1"
    shift
    local options=("$@")
    echo -e "  ${CYAN}?${NC} ${prompt}"
    local i=1
    for opt in "${options[@]}"; do
        echo -e "    ${BOLD}${i}${NC}) ${opt}"
        ((i++))
    done
    while true; do
        read -p "$(echo -e "  ${CYAN}►${NC} Select [1-$((${#options[@]}))]: ")" choice
        if [[ "$choice" =~ ^[0-9]+$ ]] && [ "$choice" -ge 1 ] && [ "$choice" -le "${#options[@]}" ]; then
            SELECTED_OPTION="$choice"
            return 0
        fi
        echo -e "  ${DIM}Invalid choice, please try again${NC}"
    done
}

# ====================================================================
# Defaults
# ====================================================================
OPT_ICONS=ON
OPT_SSH=OFF
OPT_TREE_SITTER=OFF
OPT_LIBCHAFA=OFF
OPT_PLUGINS=ON
OPT_AI=OFF
OPT_BUILD_TYPE=Debug
OPT_CLEAN=false
OPT_INSTALL=false
OPT_RUN=false
INTERACTIVE=true

# ====================================================================
# Parse command-line args
# ====================================================================
print_usage() {
    echo "Usage: ./build.sh [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --ai                  Enable AI assistant (Ollama/OpenAI)"
    echo "  --ssh                 Enable SSH support (requires libssh2)"
    echo "  --tree-sitter         Enable tree-sitter (requires grammars)"
    echo "  --libchafa            Enable libchafa image preview"
    echo "  --no-icons            Disable Nerd Font icons"
    echo "  --no-plugins          Disable plugin system"
    echo "  --release             Build in Release mode"
    echo "  --relwithdebinfo      Build in RelWithDebInfo mode"
    echo "  --minsizerel          Build in MinSizeRel mode"
    echo "  --clean               Clean build (remove build dir)"
    echo "  --install             Install to system after build"
    echo "  --run                 Run after build"
    echo "  --all                 Enable all features"
    echo "  --help, -h            Show this help"
    echo ""
    echo "Examples:"
    echo "  ./build.sh                     # interactive mode"
    echo "  ./build.sh --ai                # build with AI (default opts)"
    echo "  ./build.sh --ai --release      # AI + release build"
    echo "  ./build.sh --all               # everything enabled"
    echo "  ./build.sh --ai --clean --run  # quick dev cycle"
}

for arg in "$@"; do
    case "$arg" in
        --ai)           OPT_AI=ON ;;
        --ssh)          OPT_SSH=ON ;;
        --tree-sitter)  OPT_TREE_SITTER=ON ;;
        --libchafa)     OPT_LIBCHAFA=ON ;;
        --no-icons)     OPT_ICONS=OFF ;;
        --no-plugins)   OPT_PLUGINS=OFF ;;
        --release)      OPT_BUILD_TYPE=Release ;;
        --relwithdebinfo) OPT_BUILD_TYPE=RelWithDebInfo ;;
        --minsizerel)   OPT_BUILD_TYPE=MinSizeRel ;;
        --clean)        OPT_CLEAN=true ;;
        --install)      OPT_INSTALL=true ;;
        --run)          OPT_RUN=true ;;
        --all)
            OPT_AI=ON; OPT_SSH=ON; OPT_TREE_SITTER=ON
            OPT_LIBCHAFA=ON; OPT_PLUGINS=ON; OPT_ICONS=ON
            ;;
        --help|-h)      print_usage; exit 0 ;;
        *)              echo -e "${RED}Unknown option: $arg${NC}"; print_usage; exit 1 ;;
    esac
    INTERACTIVE=false
done

# ====================================================================
# Interactive mode (only when no flags given)
# ====================================================================
if $INTERACTIVE; then
    print_banner

    echo -e "\n${GREEN}${BOLD}►${NC} ${BOLD}Configure Build Options${NC}"

    ask_yes_no "Enable Nerd Font icons?" "y"         && OPT_ICONS=ON || OPT_ICONS=OFF
    ask_yes_no "Enable AI assistant (Ollama/OpenAI)?" "y" && OPT_AI=ON || OPT_AI=OFF
    ask_yes_no "Enable SSH support (requires libssh2)?" "n" && OPT_SSH=ON || OPT_SSH=OFF
    ask_yes_no "Enable tree-sitter (requires grammars)?" "n" && OPT_TREE_SITTER=ON || OPT_TREE_SITTER=OFF
    ask_yes_no "Enable libchafa image preview?" "n"   && OPT_LIBCHAFA=ON || OPT_LIBCHAFA=OFF
    ask_yes_no "Enable plugin system?" "y"            && OPT_PLUGINS=ON || OPT_PLUGINS=OFF

    ask_option "Select build type:" "Debug" "Release" "RelWithDebInfo" "MinSizeRel"
    case "$SELECTED_OPTION" in
        1) OPT_BUILD_TYPE=Debug ;;
        2) OPT_BUILD_TYPE=Release ;;
        3) OPT_BUILD_TYPE=RelWithDebInfo ;;
        4) OPT_BUILD_TYPE=MinSizeRel ;;
    esac

    ask_yes_no "Clean build (remove old build directory)?" "y" && OPT_CLEAN=true
fi

# ====================================================================
# Summary
# ====================================================================
print_banner

LABEL_W=16
VALUE_W=16
INNER_W=$((LABEL_W + VALUE_W + 1))
HR=$(printf '─%.0s' $(seq 1 $INNER_W))
TITLE="Build Summary"
PAD_L=$(((INNER_W - ${#TITLE}) / 2))
PAD_R=$((INNER_W - ${#TITLE} - PAD_L))

echo ""
echo -e "${BOLD}  ┌${HR}┐${NC}"
printf "  ${BOLD}│${NC}%*s%s%*s${BOLD}│${NC}\n" $PAD_L "" "$TITLE" $PAD_R ""
echo -e "${BOLD}  ├${HR}┤${NC}"
printf "  ${BOLD}│${NC} %-${LABEL_W}s %-${VALUE_W}s ${BOLD}│${NC}\n" "AI:" "$OPT_AI"
printf "  ${BOLD}│${NC} %-${LABEL_W}s %-${VALUE_W}s ${BOLD}│${NC}\n" "Icons:" "$OPT_ICONS"
printf "  ${BOLD}│${NC} %-${LABEL_W}s %-${VALUE_W}s ${BOLD}│${NC}\n" "SSH:" "$OPT_SSH"
printf "  ${BOLD}│${NC} %-${LABEL_W}s %-${VALUE_W}s ${BOLD}│${NC}\n" "Tree-sitter:" "$OPT_TREE_SITTER"
printf "  ${BOLD}│${NC} %-${LABEL_W}s %-${VALUE_W}s ${BOLD}│${NC}\n" "libchafa:" "$OPT_LIBCHAFA"
printf "  ${BOLD}│${NC} %-${LABEL_W}s %-${VALUE_W}s ${BOLD}│${NC}\n" "Plugins:" "$OPT_PLUGINS"
printf "  ${BOLD}│${NC} %-${LABEL_W}s %-${VALUE_W}s ${BOLD}│${NC}\n" "Build type:" "$OPT_BUILD_TYPE"
printf "  ${BOLD}│${NC} %-${LABEL_W}s %-${VALUE_W}s ${BOLD}│${NC}\n" "Clean:" "$OPT_CLEAN"
echo -e "${BOLD}  └${HR}┘${NC}"

if $INTERACTIVE; then
    if ! ask_yes_no "Proceed with build?" "y"; then
        echo -e "\n  Build cancelled."
        exit 0
    fi
fi

# ====================================================================
# Build
# ====================================================================
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR" || exit 1

CMAKE_OPTS=(
    "-DFTB_ENABLE_AI=$OPT_AI"
    "-DFTB_ENABLE_ICONS=$OPT_ICONS"
    "-DFTB_ENABLE_SSH=$OPT_SSH"
    "-DFTB_ENABLE_TREE_SITTER=$OPT_TREE_SITTER"
    "-DFTB_ENABLE_LIBCHAFA=$OPT_LIBCHAFA"
    "-DFTB_ENABLE_PLUGINS=$OPT_PLUGINS"
    "-DCMAKE_BUILD_TYPE=$OPT_BUILD_TYPE"
)

if [ "$OPT_CLEAN" = true ]; then
    print_step "Cleaning old build files..."
    rm -rf build
fi

print_step "Creating build directory..."
mkdir -p build && cd build || exit 1

print_step "Running CMake..."
print_info "cmake .. ${CMAKE_OPTS[*]}"
cmake .. "${CMAKE_OPTS[@]}" || { print_error "CMake configuration failed!"; exit 1; }

print_step "Building project..."
make -j$(nproc) || { print_error "Build failed!"; exit 1; }

echo ""
echo -e "${GREEN}${BOLD}  Build successful!${NC} Executable: build/${PROJECT_NAME}"

# ====================================================================
# Post-build
# ====================================================================
if [ "$OPT_INSTALL" = true ]; then
    print_step "Installing..."
    sudo make install || { print_error "Installation failed!"; exit 1; }
    echo -e "  ${GREEN}Installed. Run '${PROJECT_NAME}' to launch.${NC}"
fi

if [ -d "$SCRIPT_DIR/plugins" ]; then
    PLUGIN_DIR="${HOME}/.config/ftb/plugins"
    mkdir -p "$PLUGIN_DIR"
    for plugin_dir in "$SCRIPT_DIR/plugins/"*.ftb; do
        if [ -d "$plugin_dir" ]; then
            plugin_name=$(basename "$plugin_dir")
            if [ ! -d "$PLUGIN_DIR/$plugin_name" ]; then
                cp -r "$plugin_dir" "$PLUGIN_DIR/"
                print_info "Installed plugin: $plugin_name"
            fi
        fi
    done
fi

if [ "$OPT_RUN" = true ]; then
    print_step "Launching ${PROJECT_NAME}..."
    cd "$SCRIPT_DIR/build" || exit 1
    "./${PROJECT_NAME}" || {
        print_error "Program exited with error."
        if $INTERACTIVE && ask_yes_no "Debug with GDB?" "n"; then
            gdb "./${PROJECT_NAME}"
        fi
    }
elif $INTERACTIVE; then
    if ask_yes_no "Run now?" "n"; then
        print_step "Launching ${PROJECT_NAME}..."
        cd "$SCRIPT_DIR/build" || exit 1
        "./${PROJECT_NAME}" || {
            print_error "Program exited with error."
            if ask_yes_no "Debug with GDB?" "n"; then
                gdb "./${PROJECT_NAME}"
            fi
        }
    fi
fi

echo ""
echo -e "${DIM}  Done.${NC}"
