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