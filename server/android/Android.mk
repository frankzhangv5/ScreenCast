LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# Application name
LOCAL_MODULE := mirror_server

# Source files
LOCAL_SRC_FILES := $(LOCAL_PATH)/../common/src/main.cpp \
                   $(LOCAL_PATH)/src/AndroidDevice.cpp

# Include directories
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include \
                    $(LOCAL_PATH)/../common/include

# Compiler flags
LOCAL_CFLAGS := -DPORT=12345 -DANDROID
LOCAL_CPPFLAGS := -std=c++17 -frtti -fexceptions -D_GLIBCXX_USE_CXX11_ABI=1

# Linker flags
LOCAL_LDFLAGS := -llog -static-libstdc++

# Build as executable
include $(BUILD_EXECUTABLE)