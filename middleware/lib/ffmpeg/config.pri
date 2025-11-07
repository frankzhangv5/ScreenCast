win32 {
    INCLUDEPATH += $$PWD/include
    
    # 设置FFmpeg库文件路径
    LIBS += -L$$PWD/lib
    LIBS += -lavcodec -lavformat -lavutil -lswscale

    # Automatically copy DLLs to build directory
    FFMPEG_DLL_DIR = $$PWD/bin

    FFMPEG_COPY_DLLS.target = ffmepegdllcopy
    FFMPEG_COPY_DLLS.commands =  $$QMAKE_COPY_DIR $$replace(FFMPEG_DLL_DIR, /, \\) $$replace($$OUT_PWD, /, \\)
    PRE_TARGETDEPS += ffmepegdllcopy
    QMAKE_EXTRA_TARGETS += FFMPEG_COPY_DLLS
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