#!/bin/bash
# Force UTF-8 encoding
export LANG="en_US.UTF-8"

set -e

# Directory definitions
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/../build"
RELEASE_DIR="$SCRIPT_DIR/release"
# Get Qt path from environment variable or use default
if [ -n "$QT_ROOT_DIR" ]; then
    QT_HOME="$QT_ROOT_DIR"
else
    # Fallback for local development
    QT_HOME="/usr/local/Qt-6.9.1"
fi

QMAKE="$QT_HOME/bin/qmake"
MACDEPLOY_TOOL="$QT_HOME/bin/macdeployqt6"

echo "Qt home: $QT_HOME"
echo "QMake path: $QMAKE"
echo "MacDeployQt path: $MACDEPLOY_TOOL"

PRO_FILE="$SCRIPT_DIR/../ScreenCast.pro"

# Read version from version.pri
VERSION_MAJOR=$(grep "VER_MAJOR = " "$SCRIPT_DIR/../version.pri" | cut -d'=' -f2 | tr -d ' \r')
VERSION_MINOR=$(grep "VER_MINOR = " "$SCRIPT_DIR/../version.pri" | cut -d'=' -f2 | tr -d ' \r')
VERSION_PATCH=$(grep "VER_PATCH = " "$SCRIPT_DIR/../version.pri" | cut -d'=' -f2 | tr -d ' \r')
VERSION="${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"

# Get CPU architecture
ARCH=$(uname -m)
case $ARCH in
    "x86_64")
        ARCH_NAME="x64"
        ;;
    "arm64")
        ARCH_NAME="arm64"
        ;;
    *)
        ARCH_NAME="$ARCH"
        ;;
esac

# Clean build directory
function safe_remove() {
    local path="$1"
    if [ -d "$path" ]; then
        rm -rf "$path"
        echo "[SUCCESS] Build directory cleaned: $path"
    else
        echo "Build directory not found: $path"
    fi
}

# Install required packages
echo "Checking and installing dependencies..."

if ! brew list ffmpeg &>/dev/null; then
    echo "Installing FFmpeg..."
    brew install ffmpeg@7
fi

echo "Starting build process..."
echo "Using Qt home: $QT_HOME"
echo "Version: $VERSION"
echo "Architecture: $ARCH_NAME"

safe_remove "$BUILD_DIR"
safe_remove "$RELEASE_DIR"

mkdir -p "$BUILD_DIR"
mkdir -p "$RELEASE_DIR"

# Read additional variables from version.pri
APP_NAME=$(grep "APP_NAME = " "$SCRIPT_DIR/../version.pri" | cut -d'=' -f2 | tr -d ' \r')
APP_ID=$(grep "APP_ID = " "$SCRIPT_DIR/../version.pri" | cut -d'=' -f2 | tr -d ' \r')
BUILD_DATE=$(date +%Y%m%d)
BUILD_VERSION="${VERSION}.${BUILD_DATE}"

# Replace variables in Info.plist using sed
echo "Replacing variables in Info.plist..."
sed -i.bak \
    -e "s/APP_EXECUTABLE/${APP_NAME}/g" \
    -e "s/BUNDLE_IDENTIFIER/${APP_ID}/g" \
    -e "s/APP_NAME/${APP_NAME}/g" \
    -e "s/DISPLAY_NAME/${APP_NAME}/g" \
    -e "s/BUILD_VERSION/${BUILD_VERSION}/g" \
    -e "s/VERSION/${VERSION}/g" \
    -e "s/AppIcon/macos_icon.icns/g" \
    "$SCRIPT_DIR/Info.plist"

plutil -p "$SCRIPT_DIR/Info.plist"

# Generate Release version
cd "$BUILD_DIR"
echo "Generating Makefiles..."
"$QMAKE" "$PRO_FILE" -spec macx-clang "CONFIG+=release"

echo "Building with make..."
make -j$(sysctl -n hw.ncpu)

# Check if the app bundle was created
EXE_PATH="$BUILD_DIR/${APP_NAME}.app"
if [ ! -d "$EXE_PATH" ]; then
    echo "App bundle not found: $EXE_PATH"
    exit 1
fi

echo "App bundle created successfully: $EXE_PATH"

echo "Info.plist content:"
plutil -p ${EXE_PATH}/Contents/Info.plist

# Copy app bundle to release directory with version and architecture in name
RELEASE_APP_NAME="ScreenCast-v${VERSION}-${ARCH_NAME}.app"
RELEASE_APP_PATH="$RELEASE_DIR/$RELEASE_APP_NAME"

echo "Copying app bundle to release directory..."
cp -rvf "$EXE_PATH" "$RELEASE_APP_PATH"

# Deploy Qt dependencies and create DMG automatically
echo "Deploying Qt dependencies and creating DMG..."
"$MACDEPLOY_TOOL" "$RELEASE_APP_PATH" -dmg -verbose=1

# The DMG will be created in the same directory as the app bundle
# macdeployqt6 automatically names it based on the app bundle name
DMG_NAME="ScreenCast-v${VERSION}-${ARCH_NAME}.dmg"
DMG_PATH="$RELEASE_DIR/$DMG_NAME"

# Verify the DMG was created
if [ -f "$DMG_PATH" ]; then
    echo "DMG build completed successfully!"
    echo "Output file: $DMG_PATH"
    echo "File size: $(du -h "$DMG_PATH" | cut -f1)"
else
    echo "Failed to create DMG!"
    echo "Checking for alternative DMG names..."
    ls -la "$RELEASE_DIR"/*.dmg 2>/dev/null || echo "No DMG files found"
    exit 1
fi

# Show final results
echo ""
echo "Build completed successfully!"
echo "App Bundle: $RELEASE_APP_PATH"
echo "DMG package: $DMG_PATH ($(du -h "$DMG_PATH" | cut -f1))"

# Optionally: automatically open the folder
open "$(dirname "$DMG_PATH")" 