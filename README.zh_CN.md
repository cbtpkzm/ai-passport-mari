<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# AI Passport Mari

运行在 FoloToy AI Passport 上的离线电子宠物固件。角色使用 SANABI 风格动画，
支持中文对白、好感度成长、分时主题和三键互动。

> 本项目面向个人原型与学习用途。角色及动画相关权利归原权利人所有。

## 功能

- 7 组 RGB565 动画，按动作分别设置帧率并逐帧压缩存储
- 清晨、白天、黄昏、夜晚自动切换背景和角色色调
- 低好感、熟悉、高好感三阶段对白
- 好感度存档和 5 颗像素爱心显示
- 吃饭、玩耍、休息三种互动
- 中文完整对白、深度思考和关怀台词
- NVS 持久化，断电后保留好感度和宠物状态

## 支持的硬件

- FoloToy AI Passport
- ESP32-C3
- 8 MB Flash
- 240 x 320 ST7789P3 RGB565 屏幕
- 无 PSRAM

其他 ESP32-C3 开发板不会直接兼容本项目的屏幕、按键、电池计和引脚配置。

## 操作方式

| 按键 | 操作 |
| --- | --- |
| 上 / 下短按 | 切换吃饭、玩耍、休息 |
| 确定短按 | 执行当前互动 |
| 上键长按 | 手动将时钟调快一小时 |
| 下键长按 | 手动将时钟调慢一小时 |

设备没有独立 RTC。断电后会从固件编译时间开始计时，可通过长按上下键校准小时。

## 编译环境

必须使用 **ESP-IDF v5.5.3**。不要混用 Arduino、PlatformIO 或其他 ESP-IDF
版本生成的配置。

首次构建会通过 ESP-IDF Component Manager 下载 LVGL、`esp_lvgl_port` 和按键等
依赖。仓库已经包含固件所需的压缩动画，无需安装 FFmpeg 即可编译。

详细环境安装说明：

- [开发环境配置](docs/development/environment-setup.zh_CN.md)
- [硬件开发指南](docs/hardware-design/AI_HARDWARE_DEVELOPMENT_GUIDE.zh_CN.md)

## 快速开始

```bash
git clone https://github.com/cbtpkzm/ai-passport-mari.git
cd ai-passport-mari

source <ESP-IDF-v5.5.3路径>/export.sh
idf.py --version
idf.py set-target esp32c3
idf.py build
```

`idf.py --version` 应输出 `ESP-IDF v5.5.3`。

## 连接设备

使用支持数据传输的 USB 线连接工牌，然后查找串口。

macOS：

```bash
ls /dev/cu.usbmodem*
```

Linux：

```bash
ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null
```

Windows 可在设备管理器中查看对应的 `COM` 端口。

## 安全刷入现有工牌

对于已有系统、设备身份和 Recovery 的 FoloToy AI Passport，使用：

```bash
idf.py -p <串口> app-flash
idf.py -p <串口> monitor
```

示例：

```bash
# macOS
idf.py -p /dev/cu.usbmodem2101 app-flash

# Linux
idf.py -p /dev/ttyACM0 app-flash

# Windows
idf.py -p COM5 app-flash
```

`app-flash` 只更新位于 `0x10000` 的应用分区，不写入设备身份和 Recovery。
该方式假设设备仍使用本项目兼容的原厂分区布局。

### 重要安全说明

- **不要执行 `idf.py erase-flash`**，否则会删除设备身份和 Recovery。
- 不要把 `build/FoloToy-AI-Passport.bin` 写入 `0x0`，它只是应用镜像。
- 不要修改或覆盖 `cardid` 分区。
- 空白开发板、分区损坏或需要完整恢复时，请先阅读
  [BLE 与 Recovery 兼容说明](docs/development/ble-recovery-compatibility.md)。

退出串口监视器使用 `Ctrl+]`。

## 验证

运行主机测试：

```bash
./tools/validate.sh --static
```

构建并验证完整固件：

```bash
./tools/validate.sh --firmware
```

普通增量开发只需：

```bash
idf.py build
```

## 修改动画

原始 GIF 位于 `assets/images/pet/`，生成后的压缩帧位于
`main/assets/compressed/`。只有修改动画素材时才需要 Python、FFmpeg 和
`ffprobe`：

```bash
python3 tools/generate_pet_frames.py
idf.py build
```

各动画采样率可在
[`tools/generate_pet_frames.py`](tools/generate_pet_frames.py) 的
`FRAME_STEPS` 中单独设置。

## 主要代码

| 路径 | 内容 |
| --- | --- |
| `main/pet_app.c` | UI、动画、对白、好感度交互和时段主题 |
| `main/pet_state.c` | 宠物数值与好感度状态逻辑 |
| `main/ui_pixel.c` | 像素风界面和分时配色 |
| `main/font_pet_zh_14.c` | 中文子集字体 |
| `tools/generate_pet_frames.py` | GIF 转压缩 RGB565 帧工具 |

## 故障排查

| 问题 | 处理 |
| --- | --- |
| `idf.py: command not found` | 重新执行 ESP-IDF v5.5.3 的 `export.sh` |
| 找不到串口 | 检查 USB 数据线、设备供电和系统 USB 枚举 |
| 串口被占用 | 关闭其他 monitor、WebSerial 或串口工具 |
| 构建环境版本错误 | 激活 v5.5.3 后执行 `idf.py fullclean`，再重新构建 |
| 刷入后 USB 暂时消失 | 等待重新枚举，必要时重新插拔设备 |
