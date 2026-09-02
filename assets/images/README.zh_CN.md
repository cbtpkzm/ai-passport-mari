<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 图片资源（Images）

本目录存放项目可复用的图片资源，如 UI 图标、背景、RGB565 资源等。

## 如何使用

- 图片文件复制到本目录，并在本项目 `README.md` 记录分辨率、格式、用途与来源。
- 与固件集成时，参考 [`components/bsp/include/bsp_display.h`](../../components/bsp/include/bsp_display.h) 与相关示例分支的图片资源管线，转换为固件所需格式（如 RGB565 数组）。
- 图片资源占用 Flash 与内存，集成前请评估 ESP32-C3 无 PSRAM 的限制。

## 目录说明

新增资源时请同步更新本 `README.md` 的索引。

## SANABI 宠物动画

`pet/*.gif` 包含用户提供并针对工牌优化的七组角色动画。固件使用的动画会离线抽帧，
生成独立压缩的 96 × 128 RGB565 帧并保存到 `main/assets/compressed/`。不同动画
可以采用不同采样率，在保留动作表现的同时控制 Flash 占用；设备端无需解码 GIF，
RAM 占用也保持稳定。角色版权归原权利人所有，这些素材仅用于个人原型。
