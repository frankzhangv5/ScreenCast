# 🚀 快速开始

## 系统要求

### 操作系统支持
- **Windows**: Windows 7 及以上版本
- **macOS**: macOS 10.15 (Catalina) 及以上版本
- **Linux**: Ubuntu 18.04 及以上版本

### 硬件要求
- **处理器**: Intel/AMD 双核处理器或更高
- **内存**: 4GB RAM 或更高
- **存储**: 100MB 可用磁盘空间
- **网络**: 支持 USB 连接或网络连接

### 软件依赖
- **ADB**: Android Debug Bridge（Android 设备）
- **HDC**: Harmony Device Connector（OpenHarmony 设备）

- **从此处下载**: https://frankzhangv5/ScreenCast/DeviceDriver

## 下载安装

### 方式一：预编译版本（推荐）

1. **下载最新版本**
   - 访问 [GitHub Releases](https://github.com/frankzhangv5/ScreenCast/releases)
   - 选择适合您操作系统的版本
   - 下载对应的安装包

2. **安装步骤**

   **Windows**:
   ```bash
   # 下载 ScreenCast-Setup-v1.0.0.exe
   # 双击运行安装程序
   # 按照向导完成安装
   ```

   **macOS**:
   ```bash
   # 下载 ScreenCast-v1.0.0.dmg
   # 双击挂载 DMG 文件
   # 将 ScreenCast 拖拽到 Applications 文件夹
   ```

   **Linux**:
   ```bash
   # 下载 ScreenCast-v1.0.0.AppImage
   chmod +x ScreenCast-v1.0.0.AppImage
   ./ScreenCast-v1.0.0.AppImage
   ```

### 方式二：从源码编译

1. **克隆仓库**
   ```bash
   git clone https://github.com/frankzhangv5/ScreenCast.git
   cd ScreenCast
   ```

2. **安装依赖**
   ```bash
   # Ubuntu/Debian
   sudo apt update
   sudo apt install qt6-base-dev qt6-tools-dev-tools cmake build-essential
   
   # macOS
   brew install qt6 cmake
   
   # Windows
   # 安装 Qt 6 和 Visual Studio
   ```

3. **编译项目**
   ```bash
   mkdir build && cd build
   cmake ..
   make -j$(nproc)
   ```

4. **运行程序**
   ```bash
   ./ScreenCast
   ```

## 设备准备

### Android 设备设置

1. **启用开发者选项**
   - 进入 `设置` → `关于手机`
   - 连续点击 `版本号` 7次
   - 返回设置，找到 `开发者选项`

2. **启用 USB 调试**
   - 进入 `开发者选项`
   - 开启 `USB 调试`
   - 开启 `USB 调试（安全设置）`

3. **安装 ADB 驱动**
   ```bash
   # Windows 用户需要安装 ADB 驱动
   # 下载地址：https://developer.android.com/studio/run/win-usb
   ```

### OpenHarmony 设备设置

1. **启用开发者选项**
   - 进入 `设置` → `关于手机`
   - 连续点击 `版本号` 7次
   - 返回设置，找到 `开发者选项`

2. **启用 USB 调试**
   - 进入 `开发者选项`
   - 开启 `USB 调试`
   - 开启 `USB 调试（安全设置）`

3. **安装 HDC 工具**
   ```bash
   # 下载 HDC 工具
   # 地址：https://gitee.com/openharmony/developtools_hdc_standard
   ```

## 首次使用

### 启动程序

1. **运行 ScreenCast**
   - Windows: 从开始菜单或桌面快捷方式启动
   - macOS: 从 Applications 文件夹启动
   - Linux: 从应用程序菜单启动

2. **查看主界面**

### 连接设备

1. **连接设备**
   - 使用 USB 线连接 Android 或 OpenHarmony 设备
   - 确保设备已启用 USB 调试

2. **授权连接**
   - 在设备上弹出授权对话框时，选择 `允许 USB 调试`
   - 勾选 `始终允许来自此计算机`

3. **查看设备列表**
   - 设备连接成功后，会在主界面显示设备信息


### 开始投屏

1. **选择设备**
   - 在设备列表中点击要投屏的设备右边的播放按钮

2. **基本操作**
   - **鼠标点击**: 模拟触摸操作
   - **鼠标滑动**: 模拟滑动操作
   - **鼠标滚轮滑动**: 模拟细粒度滑动操作
   - **右键菜单**: 提供额外功能选项

## 基本功能

### 设备管理

1. **设备重命名**
   - 右键设备 → `重命名`
   - 输入自定义名称
   - 点击确定保存

2. **设备置顶**
   - 右键设备 → `置顶设备`
   - 置顶的设备会显示在列表顶部


### 投屏控制

1. **窗口控制**
   - **移动**: 拖拽窗口标题栏

2. **截图功能**
   - 右键投屏窗口 → `截图`
   - 选择保存位置
   - 图片自动保存

### 设置配置

1. **打开设置**
   - 主界面 → 设置按钮
   - 或系统托盘 → 右键 → 设置

2. **常用设置**
   - **日志设置**: 配置日志输出
   - **驱动设置**: 管理 ADB/HDC 驱动
   - **媒体设置**: 配置截图和录制路径
   - **语言设置**: 切换界面语言

## 常见问题

### 设备无法识别

**问题**: 设备连接后未在列表中显示

**解决方案**:
1. 检查 USB 调试是否启用
2. 确认 ADB/HDC 驱动是否正确安装
3. 尝试重新插拔 USB 线
4. 重启 ScreenCast 程序

### 投屏画面卡顿

**问题**: 投屏画面不流畅，有卡顿现象

**解决方案**:
1. 检查 USB 连接质量
2. 降低投屏分辨率
3. 关闭其他占用资源的程序
4. 更新显卡驱动

### 权限问题

**问题**: 程序无法访问设备或文件

**解决方案**:
1. 以管理员身份运行程序
2. 检查防火墙设置
3. 确认设备授权状态
4. 检查文件权限设置

### 程序崩溃

**问题**: 程序运行过程中崩溃

**解决方案**:
1. 查看错误日志
2. 更新到最新版本
3. 重新安装程序
4. 联系技术支持


## 技术支持

我们非常重视用户的反馈和建议，欢迎通过以下方式联系我们：

- 🐛 [报告 Bug](https://github.com/frankzhangv5/ScreenCast/issues)
- 💡 [功能建议](https://github.com/frankzhangv5/ScreenCast/discussions)
- 📧 [邮件联系](mailto:frankzhang2010@foxmail.com)
- 💬 [社区讨论](https://github.com/frankzhangv5/ScreenCast/discussions)
