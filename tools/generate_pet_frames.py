#!/usr/bin/env python3
"""Convert pet GIFs into independently compressed RGB565 frames."""

import json
import struct
import subprocess
import zlib
from pathlib import Path


WIDTH = 96
HEIGHT = 128
FRAME_BYTES = WIDTH * HEIGHT * 2
DEFAULT_FRAME_STEP = 2
FRAME_STEPS = {
    "happy": 1,
}
MAGIC = b"PZF1"


def run(*args: str) -> bytes:
    return subprocess.check_output(args)


def convert(source: Path, destination: Path) -> tuple[int, int]:
    probe = json.loads(
        run(
            "ffprobe",
            "-v",
            "error",
            "-select_streams",
            "v:0",
            "-show_frames",
            "-show_entries",
            "frame=duration_time,pkt_duration_time",
            "-of",
            "json",
            str(source),
        )
    )
    durations = [
        max(
            20,
            round(
                float(
                    frame.get("duration_time")
                    or frame.get("pkt_duration_time")
                    or 0.125
                )
                * 1000
            ),
        )
        for frame in probe["frames"]
    ]
    raw = run(
        "ffmpeg",
        "-v",
        "error",
        "-i",
        str(source),
        "-vf",
        f"scale={WIDTH}:{HEIGHT}:flags=neighbor,format=rgb565le",
        "-vsync",
        "0",
        "-f",
        "rawvideo",
        "pipe:1",
    )
    if len(raw) % FRAME_BYTES:
        raise ValueError(f"{source}: invalid decoded size {len(raw)}")
    frames = [
        raw[offset : offset + FRAME_BYTES]
        for offset in range(0, len(raw), FRAME_BYTES)
    ]
    if len(frames) != len(durations):
        raise ValueError(
            f"{source}: decoded {len(frames)} frames but probed {len(durations)}"
        )

    frame_step = FRAME_STEPS.get(source.stem, DEFAULT_FRAME_STEP)
    sampled_frames = frames[::frame_step]
    sampled_durations = [
        sum(durations[index : index + frame_step])
        for index in range(0, len(durations), frame_step)
    ]
    frames = sampled_frames
    durations = sampled_durations

    compressed = [zlib.compress(frame, level=9) for frame in frames]
    header_size = 12 + 4 * (len(frames) + 1) + 2 * len(frames)
    offsets = [header_size]
    for frame in compressed:
        offsets.append(offsets[-1] + len(frame))

    output = bytearray()
    output += struct.pack("<4sHHHH", MAGIC, WIDTH, HEIGHT, len(frames), 0)
    output += struct.pack(f"<{len(offsets)}I", *offsets)
    output += struct.pack(f"<{len(durations)}H", *durations)
    output += b"".join(compressed)
    destination.write_bytes(output)
    return len(frames), sum(durations)


def main() -> None:
    root = Path(__file__).resolve().parents[1]
    source_dir = root / "assets" / "images" / "pet"
    output_dir = root / "main" / "assets" / "compressed"
    output_dir.mkdir(parents=True, exist_ok=True)

    for source in sorted(source_dir.glob("*.gif")):
        destination = output_dir / f"{source.stem}.zframes"
        frame_count, duration_ms = convert(source, destination)
        print(
            f"{source.name}: {frame_count} frames, {duration_ms} ms, "
            f"{destination.stat().st_size / 1024:.1f} KiB"
        )


if __name__ == "__main__":
    main()
