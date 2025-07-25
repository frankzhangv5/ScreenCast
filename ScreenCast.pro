QT       += core gui network svgwidgets concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

TARGET = ScreenCast

include(version.pri)
QAPPLICATION_ORGANIZATION = $${COMPANY_NAME}
QAPPLICATION_NAME = $${APP_NAME}
QAPPLICATION_VERSION = $${VERSION}

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


# QtAwesome
CONFIG+=fontAwesomeFree
include(thirdparty/QtAwesome/QtAwesome/QtAwesome.pri)


INCLUDEPATH += include

HEADERS += $$files(include/*.h, true)
HEADERS += $$files(include/**/*.h, true)

message($${HEADERS})

SOURCES += $$files(src/*.cpp, true)
SOURCES += $$files(src/**/*.cpp, true)

message($${SOURCES})

TRANSLATIONS += \
    res/translations/app_en_US.ts

lupdate_only { 
    SOURCES += $$files(src/*.cpp, true) $$files(src/**/*.cpp, true)
}

# Resource system
RESOURCES += resources.qrc

# Cross-platform macros
win32 {
    FFMPEG_DIR = $$PWD/external/ffmpeg
    INCLUDEPATH += $$FFMPEG_DIR/include
    LIBS += -L$$FFMPEG_DIR/lib

    # Only link required libraries
    LIBS += -lavcodec \
            -lavformat \
            -lswscale \
            -lavutil

    # Automatically copy DLLs to build directory
    FFMPEG_DLL_DIR = $$FFMPEG_DIR/bin

    COPY_DLLS.target = dllcopy
    COPY_DLLS.commands =  $$QMAKE_COPY_DIR $$replace(FFMPEG_DLL_DIR, /, \\) $$replace($$OUT_PWD, /, \\)
    PRE_TARGETDEPS += dllcopy
    QMAKE_EXTRA_TARGETS += COPY_DLLS
}

unix:!macx {
    # Linux configuration
    CONFIG += static link_pkgconfig
    PKGCONFIG += libavcodec libavformat libswscale libavutil
}

macx {
    # Try to use FFMPEG_DIR environment variable first
    !isEmpty(FFMPEG_DIR) {
        INCLUDEPATH += $$FFMPEG_DIR/include
        LIBS += -L$$FFMPEG_DIR/lib
    } else {
        # Fallback to common Homebrew paths
        exists(/opt/homebrew/opt/ffmpeg@7/include) {
            INCLUDEPATH += /opt/homebrew/opt/ffmpeg@7/include
            LIBS += -L/opt/homebrew/opt/ffmpeg@7/lib
        } else: exists(/usr/local/opt/ffmpeg@7/include) {
            INCLUDEPATH += /usr/local/opt/ffmpeg@7/include
            LIBS += -L/usr/local/opt/ffmpeg@7/lib
        } else {
            # System default paths
            INCLUDEPATH += /usr/local/include
            LIBS += -L/usr/local/lib
        }
    }
    
    LIBS += -lavcodec \
            -lavformat \
            -lswscale \
            -lavutil

    # Disable symlink generation (reduce deployment issues)
    CONFIG += absolute_library_soname
}


CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
