# Common version variables (available for all platforms)
VER_MAJOR = 1
VER_MINOR = 1
VER_PATCH = 0
VERSION = $${VER_MAJOR}.$${VER_MINOR}.$${VER_PATCH}
message($${VERSION})

BUILD_ID = $$system(git rev-parse --short HEAD)

# Application information (common for all platforms)
APP_NAME = ScreenCast
AUTHOR = ZhangFeng

HOME_PAGE_URL = https://github.com/frankzhangv5/ScreenCast
DRIVER_REPO_URL = https://github.com/frankzhangv5/DeviceDriver

DEFINES += APP_NAME=\\\"$${APP_NAME}\\\"
DEFINES += APP_BUILD_ID=\\\"$${BUILD_ID}\\\"
DEFINES += APP_AUTHOR=\\\"$${AUTHOR}\\\"

DEFINES += HOME_PAGE_URL=\\\"$${HOME_PAGE_URL}\\\"
DEFINES += DRIVER_REPO_URL=\\\"$${DRIVER_REPO_URL}\\\"

QAPPLICATION_ORGANIZATION = $${APP_ORGANIZATION}
QAPPLICATION_NAME = $${APP_NAME}
QAPPLICATION_VERSION = $${VERSION}