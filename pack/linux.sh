#!/bin/bash
# Force UTF-8 encoding
export LANG="en_US.UTF-8"

set -e

# Directory definitions
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/../build"
RELEASE_DIR="$SCRIPT_DIR/release"
APPDIR="$SCRIPT_DIR/AppDir"
LINUXDEPLOY_TOOL="$SCRIPT_DIR/tools/linuxdeployqt-x86_64.AppImage"

# Get Qt path from environment variable or use default
if [ -n "$Qt6_DIR" ]; then
    QT_HOME="$Qt6_DIR"
elif [ -n "$QT_ROOT_DIR" ]; then
    QT_HOME="$QT_ROOT_DIR"
else
    # Fallback for local development
    QT_HOME="/opt/Qt/6.9.1/gcc_64"
fi

QMAKE="$QT_HOME/bin/qmake"

echo "Qt home: $QT_HOME"
echo "QMake path: $QMAKE"

APP_NAME="ScreenCast"
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
        ARCH_NAME="amd64"
        ;;
    "aarch64"|"arm64")
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
# for pkg in libavcodec-dev libavformat-dev libavutil-dev libswscale-dev imagemagick fuse libfuse2; do
#     if ! dpkg -s "$pkg" &> /dev/null; then
#         echo "Installing: $pkg"
#         sudo apt install -y $pkg
#     fi
# done

echo "Starting build process..."
echo "Using Qt home: $QT_HOME"
echo "Version: $VERSION"
echo "Architecture: $ARCH_NAME"

safe_remove "$BUILD_DIR"
safe_remove "$RELEASE_DIR"
safe_remove "$APPDIR"
safe_remove "$SCRIPT_DIR/deb"

mkdir -p "$BUILD_DIR"
mkdir -p "$RELEASE_DIR"

# Generate Release version
cd "$BUILD_DIR"
echo "Generating Makefiles..."
"$QMAKE" "$PRO_FILE" -spec linux-g++ "CONFIG+=release"

echo "Building with make..."
make -j$(nproc)

# Copy executable file
EXE_PATH="$BUILD_DIR/$APP_NAME"
if [ -f "$EXE_PATH" ]; then
    cp "$EXE_PATH" "$RELEASE_DIR/"
else
    echo "Executable not found: $EXE_PATH"
    exit 1
fi

# Deploy Qt dependencies and create AppDir
echo "Deploying Qt dependencies and creating AppDir..."
mkdir -p "$APPDIR/usr/bin"
cp "$RELEASE_DIR/$APP_NAME" "$APPDIR/usr/bin/"


# Copy desktop file and icon
mkdir -p "$APPDIR/usr/share/applications"
cp "$SCRIPT_DIR/app.desktop" "$APPDIR/usr/share/applications/ScreenCast.desktop"

# Create proper icon directory structure for AppImage
sizes=(256 128 64 48 32 16)
for size in "${sizes[@]}"; do
    mkdir -p "$APPDIR/usr/share/icons/hicolor/${size}x${size}/apps"
    convert "$SCRIPT_DIR/../res/design/icon256.png" -resize ${size}x${size} "$APPDIR/usr/share/icons/hicolor/${size}x${size}/apps/ScreenCast.png"
done

# Create index.theme file for icon theme
cat > "$APPDIR/usr/share/icons/hicolor/index.theme" << 'EOF'
[Icon Theme]
Name=Hicolor
Comment=Default icon theme
Directories=16x16/apps,32x32/apps,48x48/apps,64x64/apps,128x128/apps,256x256/apps

[16x16/apps]
Size=16
Type=Fixed

[32x32/apps]
Size=32
Type=Fixed

[48x48/apps]
Size=48
Type=Fixed

[64x64/apps]
Size=64
Type=Fixed

[128x128/apps]
Size=128
Type=Fixed

[256x256/apps]
Size=256
Type=Fixed
EOF

# Update desktop file to use correct icon path and ensure proper formatting
sed -i 's|Icon=ScreenCast|Icon=ScreenCast|g' "$APPDIR/usr/share/applications/ScreenCast.desktop"

# Ensure desktop file has proper permissions
chmod 644 "$APPDIR/usr/share/applications/ScreenCast.desktop"

cat > "$APPDIR/AppRun" << 'EOF'
#!/bin/bash

HERE="$(dirname "$(readlink -f "$0")")"
export QT_QPA_PLATFORM_PLUGIN_PATH="${HERE}/usr/plugins"
export LD_LIBRARY_PATH="${HERE}/usr/lib:${LD_LIBRARY_PATH}"

# Set XDG_DATA_DIRS to include our icons
export XDG_DATA_DIRS="${HERE}/usr/share:${XDG_DATA_DIRS:-/usr/local/share:/usr/share}"

# Set application name for better desktop integration
export DESKTOP_FILE="${HERE}/usr/share/applications/ScreenCast.desktop"

# Set application name for Qt
export QAPPLICATION_NAME="ScreenCast"
export QAPPLICATION_ORGANIZATION="ZhangFeng"

# For debugging: uncomment to see window class
# export QT_DEBUG_PLUGINS=1

exec "${HERE}/usr/bin/ScreenCast" "$@"
EOF
chmod +x "$APPDIR/AppRun"

cd $RELEASE_DIR
export APPIMAGE_EXTRACT_AND_RUN=1
"$LINUXDEPLOY_TOOL" "$APPDIR/usr/share/applications/ScreenCast.desktop" \
    -bundle-non-qt-libs \
    -qmake=$QMAKE \
    -no-translations

# Strip binaries to reduce size
echo "Stripping binaries..."
find "$APPDIR" -type f -name "*.so*" -exec strip --strip-unneeded {} + || true
strip --strip-unneeded "$APPDIR/usr/bin/$APP_NAME" || true

# use appimagetool to create AppImage
APPIMAGE_NAME="ScreenCast-v${VERSION}-linux-${ARCH_NAME}.AppImage"
"$SCRIPT_DIR/tools/appimagetool-x86_64.AppImage" -squashfs "$APPDIR" "$RELEASE_DIR/$APPIMAGE_NAME" --runtime-file="$SCRIPT_DIR/tools/runtime-x86_64"

# Find the generated AppImage
APPIMAGE_PATH=$(find "$RELEASE_DIR" -maxdepth 1 -name "${APPIMAGE_NAME}" | head -n 1)
if [ -f "$APPIMAGE_PATH" ]; then
    echo "AppImage build completed successfully!"
    echo "Output file: $APPIMAGE_PATH"
    echo "File size: $(du -h "$APPIMAGE_PATH" | cut -f1)"
else
    echo "Failed to create AppImage!"
    exit 1
fi

# Create DEB package
echo "Creating DEB package..."

DEB_DIR="$SCRIPT_DIR/deb"
DEB_PKG_NAME="${APP_NAME}_v${VERSION}_linux_${ARCH_NAME}"
DEB_PKG_DIR="$DEB_DIR/$DEB_PKG_NAME"
DEB_CONTROL_DIR="$DEB_PKG_DIR/DEBIAN"
DEB_BIN_DIR="$DEB_PKG_DIR/usr/bin"
DEB_SHARE_DIR="$DEB_PKG_DIR/usr/share"
DEB_APP_DIR="$DEB_SHARE_DIR/applications"
DEB_ICON_DIR="$DEB_SHARE_DIR/icons/hicolor/256x256/apps"
DEB_LIB_DIR="$DEB_PKG_DIR/usr/lib"
DEB_PLUGINS_DIR="$DEB_PKG_DIR/usr/plugins"

# Clean and create DEB directory structure
rm -rf "$DEB_DIR"
mkdir -p "$DEB_CONTROL_DIR" "$DEB_BIN_DIR" "$DEB_APP_DIR" "$DEB_ICON_DIR" "$DEB_LIB_DIR" "$DEB_PLUGINS_DIR"

# Copy executable and all dependencies from AppDir
echo "Copying files to DEB structure..."
cp "$APPDIR/usr/bin/$APP_NAME" "$DEB_BIN_DIR/"

mkdir -p "$DEB_LIB_DIR"
for so in "$APPDIR/usr/lib/"*; do
    so_base=$(basename "$so")
    if [[ "$so_base" == libc.so* ]] || \
       [[ "$so_base" == libpthread.so* ]] || \
       [[ "$so_base" == libm.so* ]] || \
       [[ "$so_base" == libdl.so* ]] || \
       [[ "$so_base" == librt.so* ]] || \
       [[ "$so_base" == libstdc++.so* ]] || \
       [[ "$so_base" == libgcc_s.so* ]]; then
        continue
    fi
    cp -v "$so" "$DEB_LIB_DIR/"
done

# Copy Qt plugins
if [ -d "$APPDIR/usr/plugins" ]; then
    cp -r "$APPDIR/usr/plugins/"* "$DEB_PLUGINS_DIR/" 2>/dev/null || true
fi

# Copy desktop file and icon
cp "$APPDIR/usr/share/applications/ScreenCast.desktop" "$DEB_APP_DIR/"

# Copy icon to proper location with all sizes
sizes=(256 128 64 48 32 16)
for size in "${sizes[@]}"; do
    mkdir -p "$DEB_SHARE_DIR/icons/hicolor/${size}x${size}/apps"
    cp "$APPDIR/usr/share/icons/hicolor/${size}x${size}/apps/ScreenCast.png" "$DEB_SHARE_DIR/icons/hicolor/${size}x${size}/apps/"
done

# Only copy necessary so, plugins, desktop, and icon files
# Do not copy $APPDIR/usr/share/doc, $APPDIR/usr/share/man, etc.
if [ -d "$APPDIR/usr/share" ]; then
    for item in "$APPDIR/usr/share"/*; do
        base=$(basename "$item")
        if [ -d "$item" ] && [[ "$base" != "translations" && "$base" != "icons" && "$base" != "applications" && "$base" != "doc" && "$base" != "man" && "$base" != "info" && "$base" != "locale" ]]; then
            cp -r "$item" "$DEB_SHARE_DIR/" 2>/dev/null || true
        fi
    done
fi

# Create control file with proper dependencies
cat > "$DEB_CONTROL_DIR/control" << EOF
Package: $APP_NAME
Version: $VERSION
Architecture: $ARCH_NAME
Maintainer: ZhangFeng <frankzhang02010@gmail.com>
Depends: libc6, libstdc++6, libgomp1, fuse, libfuse2
Description: Screen recording and casting tool
 A powerful screen recording and casting application
 built with Qt framework.
 This package includes all necessary dependencies.
EOF

# Create postinst script for desktop integration
cat > "$DEB_CONTROL_DIR/postinst" << 'EOF'
#!/bin/bash
set -e
update-desktop-database
gtk-update-icon-cache -f -t /usr/share/icons/hicolor || true
ldconfig
EOF
chmod +x "$DEB_CONTROL_DIR/postinst"

# Create prerm script for cleanup
cat > "$DEB_CONTROL_DIR/prerm" << 'EOF'
#!/bin/bash
set -e
# Cleanup if needed
EOF
chmod +x "$DEB_CONTROL_DIR/prerm"

# Build DEB package
dpkg-deb --build "${DEB_PKG_DIR}"
DEB_PATH="$DEB_DIR/${DEB_PKG_NAME}.deb"

if [ -f "$DEB_PATH" ]; then
    echo "DEB package created successfully!"
    echo "Output file: $DEB_PATH"
    echo "File size: $(du -h "$DEB_PATH" | cut -f1)"
else
    echo "Failed to create DEB package!"
    exit 1
fi

# Show final results
echo ""
echo "Build completed successfully!"
echo "AppImage: $APPIMAGE_PATH ($(du -h "$APPIMAGE_PATH" | cut -f1))"
echo "DEB package: $DEB_PATH ($(du -h "$DEB_PATH" | cut -f1))"

# Optionally: automatically open the folder
# xdg-open "$(dirname "$APPIMAGE_PATH")"
