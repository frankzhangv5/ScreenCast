# 在主项目中包含此文件以使用 SDK
QT       += network svgwidgets concurrent

include($$PWD/lib/ffmpeg/config.pri)

include($$PWD/../sdk/config.pri)

INCLUDEPATH += $$PWD/include
HEADERS += $$files($$PWD/include/*.h, true)
HEADERS += $$files($$PWD/include/**/*.h, true)

SOURCES += $$files($$PWD/src/*.cpp, true)
SOURCES += $$files($$PWD/src/**/*.cpp, true)

# 可选：定义 SDK 版本
DEFINES += MIDDLEWARE_VERSION=\"1.0.0\"