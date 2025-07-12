# 🔧 开发贡献

## 项目概述

ScreenCast 是一个基于 Qt 6 的跨平台投屏工具，支持 Android 和 OpenHarmony 设备。

### 技术栈
- **框架**: Qt 6.x
- **构建工具**: qmake + nmake (Windows) / make (Linux/macOS)
- **语言**: C++17
- **第三方库**: FFmpeg

## 环境搭建

### 系统要求
- **Windows**: Windows 10/11 + Visual Studio 2019+
- **Linux**: Ubuntu 18.04+ / CentOS 7+
- **macOS**: macOS 10.15+

### 依赖安装

#### Windows
```bash
# 安装 Qt 6.x
# 下载并安装 Qt 6.x 开发环境

# 安装 Visual Studio 2019+
# 确保包含 C++ 开发工具

# 安装 FFmpeg
# 将 FFmpeg 库文件放置在 external/ffmpeg/ 目录下
```

#### Linux
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install qt6-base-dev qt6-tools-dev-tools
sudo apt install build-essential cmake

# CentOS/RHEL
sudo yum install qt6-qtbase-devel qt6-qttools-devel
sudo yum install gcc-c++ make cmake
```

#### macOS
```bash
# 使用 Homebrew
brew install qt6
brew install cmake
```

## 项目结构

```
ScreenCast/
├── src/                   # 源代码目录
│   ├── main.cpp          # 程序入口
│   ├── core/             # 核心功能模块
│   ├── device/           # 设备管理模块
│   ├── ui/               # 用户界面模块
│   └── util/             # 工具类模块
├── include/              # 头文件目录
├── external/             # 第三方库目录
│   └── ffmpeg/           # FFmpeg 库
├── res/                  # 资源文件目录
├── docs/                 # 文档目录
├── ScreenCast.pro        # qmake 项目文件
└── README.md             # 项目说明
```

## 编译构建

### 使用 qmake 构建

```bash
# 生成 Makefile
qmake ScreenCast.pro

# Windows 使用 nmake
nmake

# Linux/macOS 使用 make
make

# 清理构建文件
nmake clean  # Windows
make clean   # Linux/macOS
```

### 构建配置

项目支持以下构建配置：

```bash
# Debug 版本
qmake CONFIG+=debug

# Release 版本
qmake CONFIG+=release

# 静态链接
qmake CONFIG+=static

# 动态链接
qmake CONFIG+=shared
```

## 开发指南

### 代码规范

#### 命名规范
- **类名**: 使用 PascalCase，如 `DeviceManager`
- **函数名**: 使用 camelCase，如 `connectDevice`
- **变量名**: 使用 camelCase，如 `deviceList`
- **常量名**: 使用 UPPER_CASE，如 `MAX_DEVICES`
- **文件名**: 使用 PascalCase，如 `DeviceManager.cpp`

#### 代码风格
```cpp
class DeviceManager : public QObject
{
    Q_OBJECT

public:
    explicit DeviceManager(QObject *parent = nullptr);
    ~DeviceManager();

    // 公共接口
    QVector<DeviceInfo> devices() const;
    bool connectDevice(const QString &deviceId);

private slots:
    // 私有槽函数
    void onDeviceDiscovered(const DeviceInfo &device);
    void onDeviceConnected(const QString &deviceId);

private:
    // 私有成员
    void initializeDevices();
    void loadDeviceCache();

    QVector<DeviceInfo> m_deviceList;
    QHash<QString, Device*> m_connectedDevices;
};
```

### 模块开发

#### 核心模块 (core/)
- **Settings**: 配置管理
- **StreamDecoder**: 流解码器
- **StreamReader**: 流读取器

#### 设备模块 (device/)
- **DeviceManager**: 设备管理器
- **AndroidDevice**: Android 设备支持
- **OHOSDevice**: OpenHarmony 设备支持
- **DeviceConnector**: 设备连接器

#### 界面模块 (ui/)
- **MainWindow**: 主窗口
- **DeviceListPage**: 设备列表页面
- **ScreenWindow**: 投屏窗口
- **SettingsWindow**: 设置窗口

### 调试技巧

#### 日志输出
```cpp
#include "util/Log.h"

// 使用日志宏
LOG_INFO("设备连接成功: %s", deviceId.toUtf8().constData());
LOG_ERROR("连接失败: %s", errorMessage.toUtf8().constData());
LOG_DEBUG("调试信息: %d", value);
```

#### 调试模式
```bash
# 启用调试输出
qmake CONFIG+=debug
nmake
```

## 贡献指南

### 贡献流程

#### 1. Fork 项目
1. 访问 [GitHub 项目页面](https://github.com/frankzhangv5/ScreenCast)
2. 点击 "Fork" 按钮创建自己的分支

#### 2. 创建功能分支
```bash
git clone https://github.com/frankzhangv5/ScreenCast.git
cd ScreenCast
git checkout -b feature/your-feature-name
```

#### 3. 开发功能
- 遵循代码规范
- 添加必要的测试
- 更新相关文档

#### 4. 提交代码
```bash
git add .
git commit -m "feat: 添加新功能描述"
git push origin feature/your-feature-name
```

#### 5. 创建 Pull Request
1. 在 GitHub 上创建 Pull Request
2. 填写详细的描述信息
3. 等待代码审查

### 提交规范

使用 [Conventional Commits](https://www.conventionalcommits.org/) 规范：

```
<type>[optional scope]: <description>

[optional body]

[optional footer(s)]
```

#### 提交类型
- `feat`: 新功能
- `fix`: Bug 修复
- `docs`: 文档更新
- `style`: 代码格式调整
- `refactor`: 代码重构
- `test`: 测试相关
- `chore`: 构建过程或辅助工具的变动

#### 示例
```bash
feat(device): 添加设备自动重连功能
fix(ui): 修复设备列表显示异常
docs: 更新快速开始指南
style: 统一代码格式
```

### 问题反馈

#### Bug 报告
在 [GitHub Issues](https://github.com/frankzhangv5/ScreenCast/issues) 中报告问题：

**标题格式**: `[Bug] 简短描述`

**内容模板**:
```markdown
## 问题描述
详细描述遇到的问题

## 复现步骤
1. 步骤1
2. 步骤2
3. 步骤3

## 期望行为
描述期望的正确行为

## 实际行为
描述实际发生的行为

## 环境信息
- 操作系统: Windows 10
- Qt 版本: 6.5.0
- 项目版本: v1.0.0

## 附加信息
截图、日志等额外信息
```

#### 功能建议
**标题格式**: `[Feature] 功能名称`

**内容模板**:
```markdown
## 功能描述
详细描述建议的功能

## 使用场景
描述功能的使用场景

## 实现建议
可选的实现建议

## 优先级
高/中/低
```

### 代码审查

#### 审查标准
- **功能正确性**: 功能实现是否正确
- **代码质量**: 代码是否清晰、可维护
- **性能影响**: 是否影响系统性能
- **安全性**: 是否存在安全隐患
- **兼容性**: 是否影响其他功能

#### 审查流程
1. **自动检查**: CI/CD 自动运行测试
2. **代码审查**: 维护者进行代码审查
3. **功能测试**: 测试新功能
4. **合并代码**: 审查通过后合并

### 社区参与

#### 讨论交流
- **GitHub Discussions**: 功能讨论和问题解答
- **Issues**: Bug 报告和功能建议
- **Pull Requests**: 代码贡献

#### 帮助他人
- 回答其他用户的问题
- 审查他人的代码
- 改进项目文档
- 分享使用经验

## 发布流程

### 版本号规范
使用 [Semantic Versioning](https://semver.org/) 规范：

```
MAJOR.MINOR.PATCH
```

- **MAJOR**: 不兼容的 API 修改
- **MINOR**: 向下兼容的功能性新增
- **PATCH**: 向下兼容的问题修正

### 发布步骤
1. **功能开发**: 完成新功能开发
2. **测试验证**: 全面的测试验证
3. **版本号更新**: 更新版本号
4. **文档更新**: 更新相关文档
5. **构建发布**: 构建各平台安装包
6. **正式发布**: 发布到 GitHub Releases

---

**感谢您的贡献！让我们一起让 ScreenCast 变得更好！** 