<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Images

Store reusable source images and generated display assets here.

- Use descriptive names and document dimensions, pixel format, conversion steps, and destination.
- Prefer formats suitable for the 240 × 320 RGB565 display and account for Flash and internal RAM.
- Preserve editable sources where licensing permits, and record the source and license.
- Never commit device QR secrets, credentials, or personal data in images.

## SANABI pet animations

`pet/*.gif` contains seven user-provided character animations optimized for
the badge. Firmware frames are sampled offline into independently compressed
96 x 128 RGB565 frames under `main/assets/compressed/`. Per-animation sampling
keeps expressive animations smooth while limiting Flash use. This avoids
runtime GIF decoding and keeps RAM use deterministic. Character rights remain
with their respective owner; these assets are for personal prototype use.
