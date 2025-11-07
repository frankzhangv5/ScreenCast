TEMPLATE = lib
CONFIG += plugin
QT += core widgets

TARGET = android_device_plugin

# 包含SDK配置
include($$PWD/../../sdk/config.pri)

# 源文件
SOURCES += \
    AndroidDevicePlugin.cpp \
    AndroidDevice.cpp \

# 头文件
HEADERS += \
    AndroidDevice.h \
    AndroidDevicePlugin.h# 资源文件
RESOURCES += \
    android.qrc

# 定义
DEFINES += ANDROID_DEVICE_PLUGIN_LIBRARY

# 平台特定设置
win32 {
    LIBS += -luser32 -lshell32
}

unix:!macx {
    LIBS += -ldl
}

macx {
    LIBS += -framework Foundation
}