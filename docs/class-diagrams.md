# PlantUML 类图文档

本文档包含 ScreenCast 项目的详细类图，使用 PlantUML 格式描述系统架构和模块关系。

## 类图文件列表

### 1. 设备扫描和管理模块
**文件**: `device-management-class-diagram.puml`

展示设备发现、管理和控制的核心类结构：
- **DeviceManager**: 单例模式的设备管理中心
- **DeviceProxy**: 设备代理抽象基类，定义通用接口
- **AndroidDevice**: Android设备代理实现
- **OHOSDevice**: OpenHarmony设备代理实现
- **DeviceConnector**: 网络连接管理器
- **DeviceInfo/DeviceInfoFull**: 设备信息数据结构

### 2. 屏幕录制数据流模块
**文件**: `screen-recording-dataflow-class-diagram.puml`

展示从设备到显示的完整数据流：
- **StreamReader**: 屏幕数据读取器
- **StreamDecoder**: H264视频流解码器（基于FFmpeg）
- **DeviceConnector**: TCP网络连接管理
- **数据流序列**: 详细的数据处理流程说明

### 3. 界面显示模块
**文件**: `ui-display-class-diagram.puml`

展示用户界面组件结构：
- **FramelessWindow**: 无边框窗口基类
- **MainWindow**: 主程序窗口
- **DeviceListPage**: 设备列表页面
- **ScreenWindow**: 屏幕显示窗口
- **SettingsWindow**: 设置窗口
- **DeviceItem**: 设备列表项组件

### 4. 系统架构总览
**文件**: `system-architecture-overview.puml`

展示整个系统的分层架构：
- **设备管理层**: 设备发现和管理
- **数据流层**: 屏幕数据传输和解码
- **UI表现层**: 用户界面展示
- **外部依赖**: FFmpeg、Qt、平台工具

## 类图设计原则

### 模块化设计
- 每个类图专注于特定功能模块
- 清晰的包边界和依赖关系
- 支持关注点分离

### 可扩展性
- 使用抽象基类定义接口
- 支持新设备类型的添加
- 插件式架构设计

### 跨平台支持
- 统一的设备代理接口
- 平台特定实现隔离
- 支持Android和OpenHarmony

## 关键设计模式

### 1. 单例模式
- `DeviceManager`: 全局设备管理中心
- `Settings`: 全局配置管理
- `TrayManager`: 系统托盘管理

### 2. 观察者模式
- 设备状态变化的信号通知
- UI组件的自动更新
- 异步事件处理

### 3. 策略模式
- `DeviceProxy`: 不同平台的设备控制策略
- 可插拔的设备支持扩展

### 4. 工厂模式
- 设备代理的创建和管理
- 根据设备类型选择合适的代理

## 数据流架构

```
设备发现 → 连接建立 → 数据传输 → 解码处理 → UI显示
    ↓         ↓         ↓         ↓         ↓
DeviceManager → DeviceProxy → DeviceConnector → StreamDecoder → ScreenWindow
```

## 技术栈映射

| 功能模块 | 技术组件 | 类图文件 |
|---------|----------|----------|
| 设备管理 | Qt + 平台工具 | device-management-class-diagram.puml |
| 视频解码 | FFmpeg | screen-recording-dataflow-class-diagram.puml |
| 用户界面 | Qt Widgets | ui-display-class-diagram.puml |
| 网络通信 | Qt Network | 包含在数据流类图中 |

## 扩展指南

### 添加新设备类型
1. 继承 `DeviceProxy` 创建新设备代理类
2. 在 `DeviceManager` 中注册新代理
3. 更新设备类型枚举

### 添加新功能
1. 在相关类图中添加新类和关系
2. 遵循现有设计模式
3. 保持模块边界清晰

### 性能优化
1. 使用异步处理避免UI阻塞
2. 合理的数据缓存策略
3. 资源清理和内存管理

## 维护建议

1. **版本控制**: 类图与代码保持同步更新
2. **文档注释**: 重要变更需要更新类图
3. **代码审查**: 新功能开发前更新相关类图
4. **定期回顾**: 每季度检查类图的准确性