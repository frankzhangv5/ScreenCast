# 跨平台构建指南

## 概述

ScreenCast 支持在 Windows、Linux 和 macOS 上进行跨平台构建。本文档详细说明了各种构建方式和最佳实践。

## 构建方式

### 1. 本地构建（推荐）

#### Windows 构建
```powershell
# 在 Windows 上构建
cd pack
powershell -ExecutionPolicy Bypass -File windows.ps1
```

#### Linux 构建
```bash
# 在 Linux 上构建
cd pack
chmod +x linux.sh
./linux.sh
```

#### macOS 构建
```bash
# 在 macOS 上构建
cd pack
chmod +x macos.sh
./macos.sh
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

### Windows
- **可执行文件**: `ScreenCast.exe`
- **安装包**: `ScreenCast-v1.0.0-x86_64-Setup.exe`
- **压缩包**: `ScreenCast-v1.0.0-win64.zip`

### Linux
- **AppImage**: `ScreenCast-v1.0.0-x86_64.AppImage`
- **DEB 包**: `ScreenCast_v1.0.0_amd64.deb`
- **可执行文件**: `ScreenCast`

### macOS
- **App Bundle**: `ScreenCast.app`
- **DMG 镜像**: `ScreenCast-v1.0.0-macOS.dmg`

## 环境要求

### Windows
- Windows 10/11
- Qt 6.9.1
- MinGW-w64 或 Visual Studio
- Inno Setup（可选，用于创建安装包）

### Linux
- Ubuntu 18.04+ 或 CentOS 7+
- Qt 6.9.1
- GCC 7+ 或 Clang
- FFmpeg 开发库
- ImageMagick

### macOS
- macOS 10.15+
- Qt 6.9.1
- Xcode Command Line Tools
- FFmpeg（通过 Homebrew）

## 最佳实践

### 1. 版本管理
- 使用 `version.pri` 统一管理版本号
- 在构建脚本中自动读取版本信息
- 确保所有平台的版本号一致

### 2. 依赖管理
- 使用 Qt 的跨平台特性
- 在 `.pro` 文件中使用平台条件编译
- 确保第三方库在各平台上的兼容性

### 3. 资源文件
- 使用 Qt 资源系统（`.qrc` 文件）
- 为不同平台准备相应的图标格式
- 统一资源文件的管理

### 4. 测试验证
- 在每个平台上测试构建产物
- 验证功能完整性和兼容性
- 使用自动化测试确保质量

## 常见问题

### Q: 为什么不能在 Windows 上构建 macOS DMG？
A: macOS DMG 需要 macOS 特有的工具链和系统库，无法在 Windows 上直接构建。

### Q: 如何实现真正的跨平台构建？
A: 使用 CI/CD 系统（如 GitHub Actions）在不同平台的 runner 上并行构建。

### Q: 本地开发时如何测试多平台？
A: 使用虚拟机或 Docker 容器创建不同平台的开发环境。

### Q: 如何处理平台特定的代码？
A: 使用 Qt 的预处理器宏（如 `Q_OS_WIN`、`Q_OS_MAC`、`Q_OS_LINUX`）进行条件编译。

## 总结

虽然无法在单一平台上构建所有平台的安装包，但通过合理的工具链配置和自动化构建系统，可以实现高效的跨平台开发和发布流程。推荐使用 GitHub Actions 等 CI/CD 工具来实现真正的跨平台构建自动化。 