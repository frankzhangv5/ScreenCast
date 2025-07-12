# Common version variables (available for all platforms)
VER_MAJOR = 1
VER_MINOR = 0
VER_PATCH = 0
VERSION = $${VER_MAJOR}.$${VER_MINOR}.$${VER_PATCH}

message($${VERSION})

# Optional: Build number (usually used for CI)
win32 {
    BUILD_DATE = $$system(powershell -command "Get-Date -Format 'yyyyMMdd'")
} else {
    BUILD_DATE = $$system(date +%Y%m%d)
}
message($${BUILD_DATE})
BUILD_NUMBER = $$BUILD_DATE
VERSION_FULL = $${VERSION}.$${BUILD_NUMBER}
BUILD_ID = $$system(git rev-parse --short HEAD)
message($${VERSION_FULL})

# Application information (common for all platforms)
APP_NAME = ScreenCast
COMPANY_NAME = ZhangFeng
APP_ID = zhang.feng.screencast
COPYRIGHT = "Copyright @ 2025 $${COMPANY_NAME}"

DEFINES += APP_VERSION=\\\"$${VERSION}\\\"
DEFINES += APP_BUILD_ID=\\\"$${BUILD_ID}\\\"
DEFINES += APP_NAME=\\\"$${APP_NAME}\\\"
DEFINES += APP_ID=\\\"$${APP_ID}\\\"

win32 {
    DEFINES += APP_PLATFORM=\\\"WINDOWS\\\"
    message(Building for Windows platform)
    # ==== WINDOWS specific settings ====
    # Resource file variables (Windows only)
    QMAKE_TARGET_PRODUCT = "$${APP_NAME}"
    QMAKE_TARGET_COMPANY = "$${COMPANY_NAME}"
    QMAKE_TARGET_DESCRIPTION = "$${APP_NAME}"
    QMAKE_TARGET_COPYRIGHT = $${COPYRIGHT}

    RC_ICONS = $$PWD/res/app_icons/windows_icon.ico
}

unix:!macx {
    DEFINES += APP_PLATFORM=\\\"LINUX\\\"
    message(Building for Linux platform)
    # ==== LINUX specific settings ====
    # Desktop file settings
    DESKTOP_FILE = $$PWD/pack/app.desktop
}

macx {
    DEFINES += APP_PLATFORM=\\\"MACOS\\\"
    message(Building for macOS platform)

    # macOS specific Bundle settings
    ICON = $$PWD/res/app_icons/macos_icon.icns

    QMAKE_INFO_PLIST = $$PWD/pack/Info.plist
}
