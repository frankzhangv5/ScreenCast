QT       += core gui network svgwidgets concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

TARGET = ScreenCast

include(version.pri)

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# QtAwesome
CONFIG+=fontAwesomeFree
include(lib/QtAwesome/QtAwesome/QtAwesome.pri)
include($$PWD/../middleware/config.pri)

INCLUDEPATH += include
HEADERS += $$files($$PWD/include/*.h, true)
HEADERS += $$files($$PWD/include/**/*.h, true)

SOURCES += $$files($$PWD/src/*.cpp, true)
SOURCES += $$files($$PWD/src/**/*.cpp, true)

# Resource system
RESOURCES += res/resource.qrc

win32 {
    RC_ICONS = $$PWD/res/app_icons/windows_icon.ico
}

macx {
    ICON = $$PWD/res/app_icons/macos_icon.icns
}


lupdate_only { 
    SOURCES += $$files(src/*.cpp, true) $$files(src/**/*.cpp, true)
}

TRANSLATIONS += \
    res/translations/de_DE.ts \
    res/translations/en_GB.ts \
    res/translations/en_US.ts \
    res/translations/fr_FR.ts \
    res/translations/hi_IN.ts \
    res/translations/ja_JP.ts \
    res/translations/ko_KR.ts \
    res/translations/ru_RU.ts \
    res/translations/th_TH.ts \
    res/translations/vi_VN.ts \
    res/translations/zh_CN.ts \
    res/translations/zh_HK.ts \
    res/translations/zh_TW.ts

CONFIG += lrelease
CONFIG += embed_translations