#!/bin/bash

# Extract current version from CMakeCache if present
CURRENT_VER=$(grep "APP_VERSION:" build/CMakeCache.txt 2>/dev/null | cut -d'=' -f2)
CURRENT_VER=${CURRENT_VER:-v0.1}

# Prompt user for version
read -p "Enter App Version [Current: $CURRENT_VER] (Press Enter to keep): " INPUT_VER

# Use input or default to current version
VERSION=${INPUT_VER:-$CURRENT_VER}

echo "Building Replay with version: $VERSION..."

# Configure and compile
cmake -B build -DAPP_VERSION="$VERSION"
cmake --build build
