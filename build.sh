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

print_banner() {
    echo ""
    echo -e "${CYAN}${BOLD}  ╔═══════════════════════════════╗${NC}"
    echo -e "${CYAN}${BOLD}  ║       FTB Build System        ║${NC}"
    echo -e "${CYAN}${BOLD}  ╚═══════════════════════════════╝${NC}"
    echo ""
}

print_step() {
    echo -e "\n${GREEN}${BOLD}►${NC} ${BOLD}$1${NC}"
}

print_info() {
    echo -e "  ${DIM}$1${NC}"
}

print_warn() {
    echo -e "  ${YELLOW}$1${NC}"
}

print_error() {
    echo -e "  ${RED}$1${NC}"
}

ask_yes_no() {
    local prompt="$1"
    local default="${2:-n}"
    local suffix
    if [[ "$default" == "y" ]]; then
        suffix="[Y/n]"
    else
        suffix="[y/N]"
    fi
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

# ─── Main ──────────────────────────────────────────────────────────────

print_banner

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR" || exit 1

# ─── CMake Options Configuration ──────────────────────────────────────

CMAKE_OPTS=()

print_step "Configure Build Options"

# Icons
if ask_yes_no "Enable Nerd Font icons?" "y"; then
    CMAKE_OPTS+=("-DFTB_ENABLE_ICONS=ON")
    print_info "Icons: ON (Nerd Font)"
else
    CMAKE_OPTS+=("-DFTB_ENABLE_ICONS=OFF")
    print_info "Icons: OFF (ASCII fallback)"
fi

# SSH
if ask_yes_no "Enable SSH support (requires libssh2)?" "n"; then
    CMAKE_OPTS+=("-DFTB_ENABLE_SSH=ON")
    print_info "SSH: ON"
else
    CMAKE_OPTS+=("-DFTB_ENABLE_SSH=OFF")
    print_info "SSH: OFF"
fi

# Tree-sitter
if ask_yes_no "Enable tree-sitter syntax highlighting (requires tree-sitter libs)?" "n"; then
    CMAKE_OPTS+=("-DFTB_ENABLE_TREE_SITTER=ON")
    print_info "Tree-sitter: ON"
else
    CMAKE_OPTS+=("-DFTB_ENABLE_TREE_SITTER=OFF")
    print_info "Tree-sitter: OFF (built-in keyword matching)"
fi

# libchafa
if ask_yes_no "Enable libchafa image preview (requires libchafa)?" "n"; then
    CMAKE_OPTS+=("-DFTB_ENABLE_LIBCHAFA=ON")
    print_info "libchafa: ON"
else
    CMAKE_OPTS+=("-DFTB_ENABLE_LIBCHAFA=OFF")
    print_info "libchafa: OFF (stb_image + Unicode blocks)"
fi

# Plugin system
if ask_yes_no "Enable plugin system (TypeScript/JavaScript, requires QuickJS)?" "y"; then
    CMAKE_OPTS+=("-DFTB_ENABLE_PLUGINS=ON")
    print_info "Plugins: ON"
else
    CMAKE_OPTS+=("-DFTB_ENABLE_PLUGINS=OFF")
    print_info "Plugins: OFF"
fi

# Build type
ask_option "Select build type:" "Debug" "Release" "RelWithDebInfo" "MinSizeRel"
case "$SELECTED_OPTION" in
    1) CMAKE_OPTS+=("-DCMAKE_BUILD_TYPE=Debug") ;;
    2) CMAKE_OPTS+=("-DCMAKE_BUILD_TYPE=Release") ;;
    3) CMAKE_OPTS+=("-DCMAKE_BUILD_TYPE=RelWithDebInfo") ;;
    4) CMAKE_OPTS+=("-DCMAKE_BUILD_TYPE=MinSizeRel") ;;
esac

# Clean build
CLEAN_BUILD=false
if ask_yes_no "Clean build (remove old build directory)?" "y"; then
    CLEAN_BUILD=true
fi

# ─── Summary ──────────────────────────────────────────────────────────

# Extract option values
ICONS_VAL=$(echo "${CMAKE_OPTS[@]}" | grep -oP 'ICONS=\K[^ ]+')
SSH_VAL=$(echo "${CMAKE_OPTS[@]}" | grep -oP 'SSH=\K[^ ]+')
TREE_SITTER_VAL=$(echo "${CMAKE_OPTS[@]}" | grep -oP 'TREE_SITTER=\K[^ ]+')
CHAFA_VAL=$(echo "${CMAKE_OPTS[@]}" | grep -oP 'LIBCHAFA=\K[^ ]+')
PLUGINS_VAL=$(echo "${CMAKE_OPTS[@]}" | grep -oP 'PLUGINS=\K[^ ]+')
BUILD_VAL=$(echo "${CMAKE_OPTS[@]}" | grep -oP 'BUILD_TYPE=\K[^ ]+')
CLEAN_VAL=$([ "$CLEAN_BUILD" = true ] && echo "Yes" || echo "No")

# Calculate column widths for alignment
LABEL_W=14
VALUE_W=16
INNER_W=$((LABEL_W + VALUE_W + 1))  # label + space + value

# Build horizontal rule
HR=$(printf '─%.0s' $(seq 1 $INNER_W))

# Title centered
TITLE="Build Summary"
PAD_L=$(((INNER_W - ${#TITLE}) / 2))
PAD_R=$((INNER_W - ${#TITLE} - PAD_L))

echo ""
echo -e "${BOLD}  ┌${HR}┐${NC}"
printf "  ${BOLD}│${NC}%*s%s%*s${BOLD}│${NC}\n" $PAD_L "" "$TITLE" $PAD_R ""
echo -e "${BOLD}  ├${HR}┤${NC}"
printf "  ${BOLD}│${NC} %-${LABEL_W}s %-${VALUE_W}s ${BOLD}│${NC}\n" "Icons:" "$ICONS_VAL"
printf "  ${BOLD}│${NC} %-${LABEL_W}s %-${VALUE_W}s ${BOLD}│${NC}\n" "SSH:" "$SSH_VAL"
printf "  ${BOLD}│${NC} %-${LABEL_W}s %-${VALUE_W}s ${BOLD}│${NC}\n" "Tree-sitter:" "$TREE_SITTER_VAL"
printf "  ${BOLD}│${NC} %-${LABEL_W}s %-${VALUE_W}s ${BOLD}│${NC}\n" "libchafa:" "$CHAFA_VAL"
printf "  ${BOLD}│${NC} %-${LABEL_W}s %-${VALUE_W}s ${BOLD}│${NC}\n" "Plugins:" "$PLUGINS_VAL"
printf "  ${BOLD}│${NC} %-${LABEL_W}s %-${VALUE_W}s ${BOLD}│${NC}\n" "Build type:" "$BUILD_VAL"
printf "  ${BOLD}│${NC} %-${LABEL_W}s %-${VALUE_W}s ${BOLD}│${NC}\n" "Clean:" "$CLEAN_VAL"
echo -e "${BOLD}  └${HR}┘${NC}"

if ! ask_yes_no "Proceed with build?" "y"; then
    echo -e "\n  Build cancelled."
    exit 0
fi

# ─── Build ────────────────────────────────────────────────────────────

if [ "$CLEAN_BUILD" = true ]; then
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

# ─── Post-build ───────────────────────────────────────────────────────

if ask_yes_no "Install to system?" "n"; then
    print_step "Installing..."
    sudo make install || { print_error "Installation failed!"; exit 1; }
    echo -e "  ${GREEN}Installed. Run '${PROJECT_NAME}' to launch.${NC}"
fi

# Install example plugins
if [ -d "$SCRIPT_DIR/plugins" ]; then
    PLUGIN_DIR="${HOME}/.config/ftb/plugins"
    mkdir -p "$PLUGIN_DIR"
    for plugin_dir in "$SCRIPT_DIR/plugins/"*.ftb; do
        if [ -d "$plugin_dir" ]; then
            plugin_name=$(basename "$plugin_dir")
            if [ ! -d "$PLUGIN_DIR/$plugin_name" ]; then
                cp -r "$plugin_dir" "$PLUGIN_DIR/"
                print_info "Installed plugin: $plugin_name"
            else
                print_info "Plugin already exists: $plugin_name (skipped)"
            fi
        fi
    done
fi

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

echo ""
echo -e "${DIM}  Done.${NC}"