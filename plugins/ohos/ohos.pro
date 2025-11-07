TEMPLATE = lib
CONFIG += plugin
QT += core widgets

TARGET = ohos_device_plugin

# 包含SDK配置
include($$PWD/../../sdk/config.pri)

# 源文件
SOURCES += \
    OHOSDevicePlugin.cpp \
    OHOSDevice.cpp \

# 头文件
HEADERS += \
    OHOSDevice.h \
    OHOSDevicePlugin.h# 资源文件
RESOURCES += \
    ohos.qrc

# 定义
DEFINES += OHOS_DEVICE_PLUGIN_LIBRARY

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