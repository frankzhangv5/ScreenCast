# 在主项目中包含此文件以使用 SDK
INCLUDEPATH += $$PWD/include
HEADERS += $$files($$PWD/include/*.h, true)
HEADERS += $$files($$PWD/include/**/*.h, true)

SOURCES += $$files($$PWD/src/*.cpp, true)
SOURCES += $$files($$PWD/src/**/*.cpp, true)

# 可选：定义 SDK 版本
DEFINES += SDK_VERSION=\"1.0.0\"

# 建议开发阶段启用可以查看QDebug打印
# CONFIG += console