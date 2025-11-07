# 跨平台构建指南

## 概述

ScreenCast 版本支持在 Windows、Linux 和 macOS 上进行跨平台构建。本文档详细说明了各种构建方式、代码结构和最佳实践。

## 代码结构

ScreenCast 版本采用模块化设计，代码结构清晰，便于维护和扩展。主要目录结构如下：

### 核心目录说明

#### app 目录
- **功能**: 包含 UI 界面相关的代码和资源
- **类型**: qmake 工程
- **主要内容**: 
  - 主窗口、设置窗口等界面组件
  - 界面资源（图标、翻译文件等）
  - 应用程序入口和生命周期管理

#### middleware 目录
- **功能**: 核心业务逻辑实现
- **主要内容**: 
  - 设备管理系统
  - 插件管理系统
  - 配置项持久化
  - 日志系统
  - 视频处理组件

#### plugins 目录
- **功能**: 设备插件集合
- **主要内容**: 
  - Android 设备插件
  - HarmonyOS 设备插件
  - 支持动态加载和卸载

#### server 目录
- **功能**: 设备端镜像服务源码
- **主要内容**: 
  - Android 设备服务
  - HarmonyOS 设备服务
  - 共享的通信协议实现

### 其他重要目录

#### sdk 目录
- **功能**: 提供给插件使用的软件开发工具包
- **主要内容**: 设备抽象接口、插件接口定义、通用工具类

#### pack 目录
- **功能**: 打包和发布相关工具
- **主要内容**: 安装包制作工具、多语言支持文件

#### docs 目录
- **功能**: 项目文档
- **主要内容**: 架构文档、构建指南、API 文档等

## 模块关系

- **app** 依赖于 **middleware** 提供的核心功能
- **middleware** 通过插件接口管理 **plugins** 目录中的设备插件
- **plugins** 使用 **sdk** 中定义的接口进行开发
- **server** 目录中的代码需要编译后部署到目标设备上

## 构建方式

### 1. 本地构建（推荐）

ScreenCast 版本使用统一的构建脚本进行跨平台构建。所有平台的构建都通过根目录下的 `build.py` 脚本完成，无需进入特定目录。

#### Windows 构建
```powershell
# 在 Windows 上构建
python build.py

# 构建并打包
python build.py --package
```

#### Linux 构建
```bash
# 在 Linux 上构建
python build.py

# 构建并打包
python build.py --package
```

#### macOS 构建
```bash
# 在 macOS 上构建
python build.py

# 构建并打包
python build.py --package
```

### 构建脚本选项

```bash
# 显示帮助信息
python build.py --help

# 指定构建配置（debug/release）
python build.py --configuration Release

# 清理构建目录
python build.py --clean
```

### 2. 自动化构建（CI/CD）

使用 GitHub Actions 进行自动化跨平台构建：

```bash
# 推送代码到 main 分支触发构建
git push origin main

# 或创建 Release 触发完整构建
git tag v1.0.0
git push origin v1.0.0
```

## 技术限制说明

### ❌ 无法跨平台构建的原因

1. **架构差异**
   - macOS 需要 ARM64 或 x86_64 架构的特定二进制格式
   - 需要 macOS 特定的系统库和框架（Cocoa、Core Foundation 等）

2. **代码签名要求**
   - macOS 应用需要 Apple 开发者证书进行代码签名
   - 无法在非 macOS 系统上生成有效的签名

3. **文件系统差异**
   - DMG 是 macOS 特有的磁盘镜像格式
   - App Bundle 结构是 macOS 特有的应用包格式

4. **依赖库差异**
   - 各平台使用不同的系统 API
   - Qt 在不同平台上的实现细节不同

### ✅ 可行的解决方案

#### 1. 使用 CI/CD 自动化构建

**GitHub Actions 工作流**：
- 在 `windows-latest` runner 上构建 Windows 版本
- 在 `ubuntu-latest` runner 上构建 Linux 版本  
- 在 `macos-latest` runner 上构建 macOS 版本

**优势**：
- 自动化程度高
- 无需本地环境配置
- 支持多平台并行构建

#### 2. 使用 Docker 容器

**Linux 容器构建**：
```dockerfile
FROM ubuntu:20.04
RUN apt-get update && apt-get install -y qt6-base-dev
# 构建 Linux 版本
```

**macOS 容器构建**（需要特殊设置）：
```dockerfile
FROM macos:latest
RUN brew install qt6
# 构建 macOS 版本
```

#### 3. 使用虚拟机

在 Windows/Linux 上安装 macOS 虚拟机：
- 使用 VMware 或 VirtualBox
- 在虚拟机内进行 macOS 构建
- 性能较低但可行

## 构建产物

使用统一的 `build.py` 脚本构建后，产物将生成在项目根目录下的 `output` 文件夹中，按平台和架构分类。

### Windows
- **构建输出目录**: `output/windows/x64/`
- **可执行文件**: `output/windows/x64/ScreenCast.exe`
- **安装包**: `output/windows/x64/installer/ScreenCast-v1.1.0-x86_64-Setup.exe`
- **压缩包**: `output/windows/x64/packages/ScreenCast-v1.1.0-win64.zip`

### Linux
- **构建输出目录**: `output/linux/x64/`
- **AppImage**: `output/linux/x64/packages/ScreenCast-v1.1.0-x86_64.AppImage`
- **DEB 包**: `output/linux/x64/packages/ScreenCast_v1.1.0_amd64.deb`
- **可执行文件**: `output/linux/x64/ScreenCast`

### macOS
- **构建输出目录**: `output/macos/`
- **App Bundle**: `output/macos/ScreenCast.app`
- **DMG 镜像**: `output/macos/packages/ScreenCast-v1.1.0-macOS.dmg`

### 插件构建产物
- **Android 插件**: `output/plugins/android/libandroidplugin.so` (Linux/macOS) 或 `output/plugins/android/androidplugin.dll` (Windows)
- **HarmonyOS 插件**: `output/plugins/ohos/libohosplugin.so` (Linux/macOS) 或 `output/plugins/ohos/ohosplugin.dll` (Windows)

### 服务器构建产物
- **Android 服务器**: `output/server/android/mirror_server.apk`
- **HarmonyOS 服务器**: `output/server/ohos/mirror_server.hap`

## 环境要求

### 通用要求
- Python 3.8+（用于运行构建脚本）
- Git（用于版本控制和获取依赖）

### Windows
- Windows 10/11 (64位)
- Qt 6.5.0+（推荐使用 Qt 6.9.1）
- MinGW-w64 12.2+ 或 Visual Studio 2022
- Inno Setup 6.2.1+（用于创建安装包）
- Python 3.8+（用于运行构建脚本）

### Linux
- Ubuntu 20.04+ 或兼容的发行版
- Qt 6.5.0+（推荐使用 Qt 6.9.1）
- GCC 9.4+ 或 Clang 12+
- FFmpeg 开发库 (libavcodec-dev, libavformat-dev, libswscale-dev)
- libgl1-mesa-dev
- libx11-dev
- patchelf (用于 AppImage 打包)

### macOS
- macOS 12.0+ (Monterey)
- Qt 6.5.0+（推荐使用 Qt 6.9.1）
- Xcode Command Line Tools (Xcode 14.0+)
- FFmpeg（通过 Homebrew 安装）
- create-dmg（通过 npm 安装，用于创建 DMG 安装包）

## 依赖管理

### 模块间依赖
- **app** 依赖于 **middleware** 和 **sdk**
- **middleware** 依赖于 **sdk** 和 FFmpeg
- **plugins** 依赖于 **sdk**
- **server** 目录中的代码使用独立的构建系统

### 第三方库
- Qt 框架：用于跨平台 UI 开发
- FFmpeg：用于视频处理和编解码
- OpenSSL：用于安全通信（可选）

### 依赖安装

#### Windows
```powershell
# 使用 vcpkg 安装 FFmpeg（推荐）
vcpkg install ffmpeg[core]:x64-windows

# 或通过 chocolatey 安装
choco install ffmpeg
```

#### Linux
```bash
# Ubuntu/Debian
apt-get update
apt-get install -y libavcodec-dev libavformat-dev libswscale-dev libavutil-dev

# CentOS/RHEL
dnf install -y ffmpeg-devel
```

#### macOS
```bash
# 使用 Homebrew 安装
brew install ffmpeg

## 最佳实践

### 1. 模块化开发
- 遵循当前的模块结构（app、middleware、plugins、server）进行开发
- 保持模块间的低耦合，通过接口进行通信
- 新增功能时考虑应该放在哪个模块中，避免跨模块职责不清晰

### 2. 插件开发
- 遵循 SDK 中定义的插件接口规范
- 将平台特定的代码封装在对应插件中
- 确保插件可以独立编译和测试

### 3. 构建优化
- 使用 `--config release` 参数进行正式版本构建，以获得更好的性能
- 定期执行 `python build.py --clean` 清理构建缓存，避免潜在的构建问题
- 在 CI/CD 环境中使用多线程构建加速构建过程

### 4. 版本管理
- 使用 `app/version.pri` 统一管理版本号
- 构建脚本会自动读取版本信息并应用到所有构建产物
- 发布新版本时同步更新所有相关的版本信息

### 5. 测试验证
- 构建完成后在目标平台上进行功能测试
- 验证插件加载和设备连接功能
- 检查构建产物的完整性和文件权限

## 常见问题

### Q: 构建脚本找不到 Qt 环境怎么办？
A: 确保已正确安装 Qt 并设置了 `QT_PATH` 环境变量，或者在运行构建脚本时使用 `--qt-path` 参数指定 Qt 安装路径。

### Q: 如何构建特定平台的插件？
A: 使用 `python build.py --platform [platform] --plugins-only` 命令只构建插件部分。

### Q: 构建过程中出现 FFmpeg 相关错误怎么办？
A: 确保已正确安装 FFmpeg 开发库，并设置了相应的环境变量或在构建脚本中指定了 FFmpeg 路径。

### Q: 如何自定义构建输出路径？
A: 使用 `--output-dir` 参数可以自定义构建产物的输出目录。

### Q: 为什么插件加载失败？
A: 检查插件是否与主程序使用相同的 Qt 版本构建，确保插件路径正确，以及插件所需的依赖库是否可用。

## 总结

ScreenCast 版本采用了模块化的架构设计，通过统一的构建脚本实现了高效的跨平台开发和构建流程。通过遵循本文档中的指南，开发者可以轻松地在不同平台上构建和打包应用程序。

主要优势包括：

- 统一的构建入口：使用根目录下的 `build.py` 脚本处理所有平台的构建和打包
- 清晰的模块化结构：UI、核心逻辑、插件和服务器代码分离，便于维护和扩展
- 统一的输出目录：所有构建产物按平台和类型组织在 `output` 目录中
- 灵活的插件系统：支持动态加载不同设备类型的插件

通过结合 CI/CD 系统，可以实现完全自动化的跨平台构建和发布流程，大大提高开发效率。