# WHT Factory Tool

[![Build and Release Qt App (Windows)](https://github.com/ylong/wht-factory-tool/actions/workflows/release.yml/badge.svg)](https://github.com/ylong/wht-factory-tool/actions/workflows/release.yml)

一个基于Qt6开发的WHT（Wireless Hardware Testing）工厂测试工具，用于无线硬件设备的配置、调试和数据采集。

## 功能特性

### 🔧 核心功能
- **UDP通信调试**: 支持UDP协议的双向通信调试
- **设备管理**: 自动发现和管理网络中的WHT设备
- **从机配置**: 可视化配置从机设备参数
- **数据采集**: 实时采集和显示传导数据、阻抗数据和夹具数据
- **协议处理**: 完整的WHT协议栈实现，支持分片传输

### 🎨 界面特性
- **现代化UI**: 采用QDarkStyle深色主题
- **多标签页**: 分离不同功能模块的用户界面
- **实时日志**: 完整的操作日志记录和文件保存
- **数据可视化**: 表格形式展示设备状态和数据

### 📡 通信协议
- **Master-Slave架构**: 支持主从设备通信
- **Backend-Master通信**: 后端与主设备的控制通信
- **多种消息类型**: 
  - 设备配置消息
  - 控制消息
  - 数据传输消息
  - 心跳和状态消息

## 系统架构

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Backend       │    │     Master      │    │     Slave       │
│  (Factory Tool) │◄──►│    Device       │◄──►│    Device       │
└─────────────────┘    └─────────────────┘    └─────────────────┘
        │                       │                       │
        │                       │                       │
        ▼                       ▼                       ▼
   UDP Protocol            RF/UWB Protocol         Sensor Data
```

## 技术栈

- **开发语言**: C++17
- **GUI框架**: Qt6 (Core, Gui, Widgets, SerialPort, Network)
- **构建系统**: CMake 3.16+
- **编译器**: MSVC 2022 / MinGW GCC
- **主题**: QDarkStyle
- **协议**: 自定义二进制协议 + UDP传输

## 快速开始

### 环境要求

- Windows 10/11 (x64)
- Qt 6.9.1 或更高版本
- CMake 3.16+
- Visual Studio 2022 或 MinGW GCC 13.1.0+

### 编译安装

1. **克隆仓库**
   ```bash
   git clone https://github.com/ylong/wht-factory-tool.git
   cd wht-factory-tool
   ```

2. **配置构建环境**
   ```bash
   # 使用CMake预设配置
   cmake --preset=release
   ```

3. **编译项目**
   ```bash
   cmake --build build/release --config Release
   ```

4. **运行程序**
   ```bash
   ./build/release/wht-factory-tool.exe
   ```

### 使用预编译版本

从 [Releases](https://github.com/ylong/wht-factory-tool/releases) 页面下载最新的预编译版本，解压后直接运行 `wht-factory-tool.exe`。

## 使用指南

### 1. UDP通信配置

1. 在"UDP调试"标签页中配置网络参数：
   - 本地IP: 默认 192.168.0.3
   - 本地端口: 默认 8080
   - 远程IP: 目标设备IP
   - 远程端口: 目标设备端口

2. 点击"连接"建立UDP连接

### 2. 设备管理

1. 在"设备管理"标签页中：
   - 点击"查询设备"扫描网络中的设备
   - 查看设备列表，包括设备ID、电池电量、信号强度等
   - 管理设备连接状态

### 3. 从机配置

1. 在"从机配置"标签页中：
   - 添加新的从机配置
   - 设置从机ID、传导通道数、阻抗通道数等参数
   - 配置夹具模式和状态
   - 发送配置到目标设备

### 4. 数据查看

1. 在"数据查看"标签页中：
   - 启动数据采集
   - 实时查看传导数据、阻抗数据
   - 监控设备状态变化

## 协议说明

### 消息类型

#### Backend ↔ Master 消息
- `SLAVE_CFG_MSG`: 从机配置消息
- `MODE_CFG_MSG`: 模式配置消息
- `CTRL_MSG`: 控制消息
- `DEVICE_LIST_REQ_MSG`: 设备列表请求
- `PING_CTRL_MSG`: 心跳控制消息

#### Master ↔ Slave 消息
- `SYNC_MSG`: 同步消息
- `PING_REQ_MSG`: Ping请求
- `SHORT_ID_ASSIGN_MSG`: 短ID分配消息

#### Slave → Backend 数据消息
- `CONDUCTION_DATA_MSG`: 传导数据
- `RESISTANCE_DATA_MSG`: 阻抗数据
- `CLIP_DATA_MSG`: 夹具数据

### 协议特性

- **自动分片**: 支持大数据包的自动分片和重组
- **CRC校验**: 数据完整性校验
- **超时重传**: 可靠的数据传输机制
- **设备状态**: 实时设备状态监控

## 项目结构

```
wht-factory-tool/
├── main.cpp                    # 程序入口
├── mainwindow.{h,cpp,ui}      # 主窗口
├── slaveconfigdialog.{h,cpp}  # 从机配置对话框
├── protocol/                   # 协议实现
│   ├── WhtsProtocol.h         # 协议总头文件
│   ├── Common.h               # 协议常量定义
│   ├── Frame.{h,cpp}          # 帧结构
│   ├── DeviceStatus.{h,cpp}   # 设备状态
│   ├── ProtocolProcessor.{h,cpp} # 协议处理器
│   ├── messages/              # 消息定义
│   │   ├── Message.h          # 消息基类
│   │   ├── Backend2Master.{h,cpp}
│   │   ├── Master2Backend.{h,cpp}
│   │   ├── Master2Slave.{h,cpp}
│   │   ├── Slave2Master.{h,cpp}
│   │   └── Slave2Backend.{h,cpp}
│   └── utils/                 # 工具类
│       └── ByteUtils.{h,cpp}  # 字节处理工具
├── external/                  # 外部依赖
│   └── qdarkstyle/           # 深色主题
├── build/                    # 构建输出
└── CMakeLists.txt           # 构建配置
```

## 开发指南

### 代码规范

本项目遵循微软C++命名规范：
- 类名使用PascalCase: `ProtocolProcessor`
- 函数名使用PascalCase: `ProcessMessage()`
- 变量名使用camelCase，成员变量加前缀: `m_pUdpSocket`
- 常量使用UPPER_CASE: `FRAME_DELIMITER_1`

### 添加新消息类型

1. 在 `protocol/Common.h` 中添加消息ID枚举
2. 在对应的消息文件中实现消息类
3. 在 `ProtocolProcessor` 中添加处理逻辑
4. 更新UI界面支持新消息

### 调试功能

- 启用详细日志输出
- UDP调试日志自动保存到文件
- 支持十六进制数据查看
- 实时协议解析显示

## 许可证

本项目采用 [MIT License](LICENSE) 开源许可证。

## 贡献

欢迎提交Issue和Pull Request来改进这个项目。

### 贡献指南

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 创建 Pull Request

## 联系方式

如有问题或建议，请通过以下方式联系：

- 创建 [GitHub Issue](https://github.com/ylong/wht-factory-tool/issues)
- 发送邮件到项目维护者

---

**注意**: 本工具专为WHT设备工厂测试设计，请确保在正确的网络环境中使用。
