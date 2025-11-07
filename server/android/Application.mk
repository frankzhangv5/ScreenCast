# Application configuration for Android NDK build

# Target ABI
APP_ABI := arm64-v8a

# Android API level
APP_PLATFORM := android-21

# C++ standard library
APP_STL := c++_static

# Enable RTTI and exceptions
APP_CPPFLAGS := -std=c++17 -frtti -fexceptions -D_GLIBCXX_USE_CXX11_ABI=1

# Optimization level
APP_OPTIM := release