<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# AI Passport Mari

An offline virtual pet firmware for the FoloToy AI Passport. It features
SANABI-style animations, Chinese dialogue, affection progression, time-based
themes, and three-button interaction.

> This project is intended for personal prototyping and learning. Character and
> animation rights remain with their respective owners.

## Features

- Seven RGB565 animations with per-action frame rates and compressed frames
- Dawn, daytime, dusk, and night background and character tinting
- Low, familiar, and high affection dialogue stages
- Persistent affection state with a five-heart pixel display
- Feed, play, and rest interactions
- Independent 10-minute reward cooldowns for each interaction
- Handwritten Chinese dialogue, reflective lines, and caring responses
- NVS persistence across power cycles

## Supported Hardware

- FoloToy AI Passport
- ESP32-C3
- 8 MB Flash
- 240 x 320 ST7789P3 RGB565 display
- No PSRAM

Other ESP32-C3 boards are not directly compatible with this project's display,
buttons, battery gauge, or pin assignments.

## Controls

| Button | Action |
| --- | --- |
| Short press Up / Down | Select Feed, Play, or Rest |
| Short press OK | Run the selected interaction |
| Long press Up | Advance the clock by one hour |
| Long press Down | Move the clock back by one hour |

The device has no independent RTC. After power loss, time starts from the
firmware build time. Use the long-press controls to correct the hour.

## Build Requirements

Use **ESP-IDF v5.5.3**. Do not generate the project configuration with Arduino,
PlatformIO, or another ESP-IDF version.

The first build downloads LVGL, `esp_lvgl_port`, button, and other dependencies
through ESP-IDF Component Manager. Compressed animation assets are committed, so
FFmpeg is not required for a normal build.

Detailed setup documentation:

- [Development environment](docs/development/environment-setup.md)
- [Hardware development guide](docs/hardware-design/AI_HARDWARE_DEVELOPMENT_GUIDE.md)

## Quick Start

```bash
git clone https://github.com/cbtpkzm/ai-passport-mari.git
cd ai-passport-mari

source <path-to-ESP-IDF-v5.5.3>/export.sh
idf.py --version
idf.py set-target esp32c3
idf.py build
```

`idf.py --version` must report `ESP-IDF v5.5.3`.

## Connect the Device

Connect the badge with a USB data cable and locate its serial port.

macOS:

```bash
ls /dev/cu.usbmodem*
```

Linux:

```bash
ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null
```

On Windows, locate the `COM` port in Device Manager.

## Safely Flash an Existing Badge

For a provisioned FoloToy AI Passport that still has its device identity and
Recovery partition, use:

```bash
idf.py -p <port> app-flash
idf.py -p <port> monitor
```

Examples:

```bash
# macOS
idf.py -p /dev/cu.usbmodem2101 app-flash

# Linux
idf.py -p /dev/ttyACM0 app-flash

# Windows
idf.py -p COM5 app-flash
```

`app-flash` updates only the application at `0x10000`. It does not write the
device identity or Recovery partitions. This assumes the badge still uses a
compatible factory partition layout.

### Important Safety Notes

- **Never run `idf.py erase-flash`** on a provisioned badge. It removes the
  device identity and Recovery.
- Never write `build/FoloToy-AI-Passport.bin` at `0x0`; it is app-only.
- Do not modify or overwrite the `cardid` partition.
- For blank boards, damaged partition tables, or full recovery, read
  [BLE and Recovery compatibility](docs/development/ble-recovery-compatibility.md)
  first.

Exit the serial monitor with `Ctrl+]`.

## Validation

Run host-side checks:

```bash
./tools/validate.sh --static
```

Build and validate the complete firmware:

```bash
./tools/validate.sh --firmware
```

For a normal incremental build:

```bash
idf.py build
```

## Replacing Animations

Source GIFs are stored under `assets/images/pet/`. Generated compressed frames
are stored under `main/assets/compressed/`. Python, FFmpeg, and `ffprobe` are
needed only when changing animation sources:

```bash
python3 tools/generate_pet_frames.py
idf.py build
```

Set per-animation sampling rates in `FRAME_STEPS` inside
[`tools/generate_pet_frames.py`](tools/generate_pet_frames.py).

## Main Source Files

| Path | Purpose |
| --- | --- |
| `main/pet_app.c` | UI, animation, dialogue, affection interactions, and themes |
| `main/pet_state.c` | Pet stats and affection state logic |
| `main/ui_pixel.c` | Pixel UI and time-based palettes |
| `main/font_pet_zh_14.c` | Embedded Chinese glyph subset |
| `tools/generate_pet_frames.py` | GIF to compressed RGB565 frame converter |

## Troubleshooting

| Problem | Resolution |
| --- | --- |
| `idf.py: command not found` | Source the ESP-IDF v5.5.3 `export.sh` |
| Serial port is missing | Check the USB data cable, power, and USB enumeration |
| Serial port is busy | Close other monitors, WebSerial, or serial tools |
| Build uses the wrong IDF | Activate v5.5.3, run `idf.py fullclean`, and rebuild |
| USB disappears after flashing | Wait for re-enumeration or reconnect the badge |
