#!/bin/bash
set -e # Exit immediately if any command fails

# Color output helpers
GREEN='\033[0;32m'
CYAN='\033[0;36m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 1. Detect or fallback version
CURRENT_VER=$(grep "APP_VERSION:" build/CMakeCache.txt 2>/dev/null | cut -d'=' -f2)
CURRENT_VER=${CURRENT_VER:-v0.1}

echo -e "${CYAN}==========================================${NC}"
echo -e "${CYAN}          Replay Build System             ${NC}"
echo -e "${CYAN}==========================================${NC}"

# 2. Prompt for Version
read -p "Enter version [Current: $CURRENT_VER]: " INPUT_VER
VERSION=${INPUT_VER:-$CURRENT_VER}

# 3. Prompt for Build Mode
echo ""
echo "Select build mode:"
echo "  [1] Executable Only  (Fast dev/test build)"
echo "  [2] Full Installer   (Build + deploy dist/ + Inno Setup)"
read -p "Choice [1 or 2, default: 1]: " BUILD_CHOICE
BUILD_CHOICE=${BUILD_CHOICE:-1}

echo ""
echo -e "${GREEN}==> Compiling Replay $VERSION...${NC}"
cmake -B build -DAPP_VERSION="$VERSION"
cmake --build build

# 4. Handle Full Installer Pipeline if Option 2 selected
if [ "$BUILD_CHOICE" == "2" ]; then
    echo -e "\n${GREEN}==> Updating dist/ directory...${NC}"
    mkdir -p dist
    cp build/ReplayQt.exe dist/
    cp app.ico dist/

    echo -e "\n${GREEN}==> Deploying Qt DLLs with windeployqt...${NC}"
    windeployqt --no-translations dist/ReplayQt.exe

    echo -e "\n${GREEN}==> Compiling Inno Setup Installer...${NC}"

    # Auto-detect ISCC location (Checking PATH, Inno Setup 7, 6, and 5)
    ISCC_PATH=""
    if command -v ISCC.exe &> /dev/null; then
        ISCC_PATH="ISCC.exe"
    elif [ -f "/c/Program Files/Inno Setup 7/ISCC.exe" ]; then
        ISCC_PATH="/c/Program Files/Inno Setup 7/ISCC.exe"
    elif [ -f "/c/Program Files (x86)/Inno Setup 7/ISCC.exe" ]; then
        ISCC_PATH="/c/Program Files (x86)/Inno Setup 7/ISCC.exe"
    elif [ -f "/c/Program Files/Inno Setup 6/ISCC.exe" ]; then
        ISCC_PATH="/c/Program Files/Inno Setup 6/ISCC.exe"
    elif [ -f "/c/Program Files (x86)/Inno Setup 6/ISCC.exe" ]; then
        ISCC_PATH="/c/Program Files (x86)/Inno Setup 6/ISCC.exe"
    elif [ -f "/c/Program Files (x86)/Inno Setup 5/ISCC.exe" ]; then
        ISCC_PATH="/c/Program Files (x86)/Inno Setup 5/ISCC.exe"
    fi

    if [ -n "$ISCC_PATH" ]; then
        "$ISCC_PATH" //DMyAppVersion="$VERSION" installer.iss
        echo -e "\n${GREEN}==========================================${NC}"
        echo -e "${GREEN} SUCCESS! Created Replay_Setup_$VERSION.exe${NC}"
        echo -e "${GREEN}==========================================${NC}"
    else
        echo -e "\n${YELLOW}Warning: ISCC.exe could not be found.${NC}"
        echo -e "${YELLOW}Please add ISCC.exe to your PATH or check your Inno Setup installation path.${NC}"
    fi
else
    echo -e "\n${GREEN}==========================================${NC}"
    echo -e "${GREEN} SUCCESS! Built build/ReplayQt.exe ($VERSION)${NC}"
    echo -e "${GREEN}==========================================${NC}"
fi
