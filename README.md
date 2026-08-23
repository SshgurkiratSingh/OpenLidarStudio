# OpenLidarStudio

<div align="center">

![OpenLidarStudio](docs/Images/Full%20Software%20View.png)

**A high-performance, open-source desktop application for real-time 2D LiDAR visualization, sensor diagnostics, hardware control, and telemetry logging.**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Windows-green.svg)](#)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-orange.svg)](#)
[![OpenGL](https://img.shields.io/badge/OpenGL-3.3%2B-red.svg)](#)
[![Release](https://img.shields.io/github/v/release/SshgurkiratSingh/OpenLidarStudio?include_prereleases)](https://github.com/SshgurkiratSingh/OpenLidarStudio/releases)

</div>

---

## Overview

OpenLidarStudio is a cross-platform desktop tool built with **modern C++17**, **Dear ImGui**, **ImPlot**, and **OpenGL 3.3+** for interfacing with YDLIDAR sensors. It provides real-time polar point-cloud rendering, adaptive auto-ranging, motion history trails, session recording, and frame-accurate replay — all in a single, self-contained binary.

> **Validated hardware:** YDLIDAR X2 (Single-Channel, 115200 baud)

---

## Screenshots

| Feature | Preview |
|---|---|
| **Live Viewport** — Distance rings, hover tooltips, motion trails | ![Viewport](docs/Images/Viewport%20.png) |
| **Telemetry & Analytics** — Scrolling range/alpha/point-count plots | ![Telemetry](docs/Images/Telemetry%20%26%20Analysis.png) |
| **History Playback** — Frame-accurate replay with transport controls | ![Replay](docs/Images/History%20Playback.png) |

---

## Features

### Core Visualization & Aesthetics
- **OpenGL 3.3+ point-cloud renderer** with a custom GLSL shader pipeline
- **Anti-aliased Circular Points** — toggleable soft-edged round points for a premium radar aesthetic
- **Turbo colormap** — points colored by range (close = blue, far = red) or intensity
- **Concentric distance rings & Cardinal markers** — labeled in metres with N/S/E/W headings
- **Cartesian Grid Overlay** — helper grid with configurable spacing in meters (e.g. 0.5m)
- **Point Size Adjuster** — scale point width dynamically from 1px to 10px

### Interactive Viewport Tools
- **Precision Zoom & Pan** — zoom using the mouse wheel (0.2x to 20x) and drag using the middle mouse button to inspect specific clusters. Double-click MMB resets the view.
- **Ruler Measurement Tool** — measure physical distance in meters by placing marker A and marker B with left-click. Right-click to clear.
- **Frame Screenshots** — export pixel-perfect high-resolution PNGs of the viewport directly from the OpenGL framebuffer.
- **Hover tooltips** — displays real-time distance and angle at the mouse cursor with an aiming guide line.

### Guard Zone Alerts
- Set up to **4 independent alarm zones** defined by customizable distance boundaries and angular ranges (e.g. 0° to 90°, 0.5m to 2m).
- Real-time alarm trigger evaluation — zones flash red instantly if a point falls within their bounds.

### Motion History Trails
- **Toggleable trail system** — enable/disable in the UI without reconnecting
- **History Length** — keep 2–30 past sweeps layered under the live scan
- **Decay Rate** — control how fast old points fade and shift to blue; fully driven by a GPU uniform

### Adaptive Auto-Ranging
- **Exponential smoothing** with configurable α-min/α-max and k-sensitivity
- Automatically expands the display range on large jumps and contracts it smoothly during quiet scenes
- Manual range override when auto-ranging is disabled

### Signal Filter / Culling
- Radius band filter (Min / Max Radius)
- Angular sweep cull region (Min / Max Angle)
- All filters applied in-pipeline before rendering

### Telemetry & Analytics
- Real-time **30-second scrolling plots** for:
- Instantaneous range, smoothed range
- Adaptive alpha coefficient
- Points per sweep
- Inline stats bar (Points, RPM, Range)

### Recording & Replay
- **Binary recording** — captures every frame at full resolution into `.ols_meta` + `.ols_data` files
- **Async logger** — lock-free ring buffer ensures the scan thread is never blocked by I/O
- **Replay tab** — load any recording, scrub frame-by-frame, play at 1–60 FPS
- **Data export** — one-click CSV or JSON conversion from any recording

### Hardware Control
- Serial port auto-discovery and baudrate selection
- **Single-Channel mode** toggle (required for YDLIDAR X2/S2)
- Target RPM slider (300–900 RPM → 5–15 Hz)
- Reset Defaults button

---

## Getting Started

### Requirements

| Dependency | Notes |
|---|---|
| CMake ≥ 3.20 | Build system |
| C++17 compiler | GCC 10+, Clang 12+, or MSVC 2019+ |
| OpenGL 3.3+ driver | Mesa / NVIDIA / AMD |
| YDLIDAR hardware | X2, X4, G4, S2 etc. |

> On **Linux**, the following system libraries are needed:
> ```bash
> sudo apt-get install libgl1-mesa-dev libglu1-mesa-dev \
>     xorg-dev libxrandr-dev libxinerama-dev \
>     libxcursor-dev libxi-dev libudev-dev
> ```
> On **Fedora**:
> ```bash
> sudo dnf install mesa-libGL-devel mesa-libGLU-devel \
>     libXrandr-devel libXinerama-devel libXcursor-devel \
>     libXi-devel systemd-devel
> ```

### Build from Source

```bash
# 1. Clone the repository with the embedded SDK
git clone --recurse-submodules https://github.com/SshgurkiratSingh/OpenLidarStudio.git
cd OpenLidarStudio

# 2. Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 3. Compile (parallel)
cmake --build build -j$(nproc)

# 4. Run
./build/OpenLidarStudio
```

### Linux Serial Port Permissions

If the app cannot open `/dev/ttyUSB0`, add yourself to the `dialout` group:
```bash
sudo usermod -aG dialout $USER
# Log out and back in, then verify:
ls -l /dev/ttyUSB0
```

### Pre-built Binaries

Download the latest release for your platform from the [**Releases page**](https://github.com/SshgurkiratSingh/OpenLidarStudio/releases):

| Platform | File |
|---|---|
| Linux (x86_64) | `OpenLidarStudio-Linux.tar.gz` |
| Windows (x64) | `OpenLidarStudio-Windows.zip` |

---

## Connecting Your LiDAR

1. Plug in your YDLIDAR via USB
2. Click **Refresh Ports** and select the port (e.g. `/dev/ttyUSB0` or `COM3`)
3. Set **Baudrate** to match your model (115200 for X2)
4. If using an X2 or S2, ensure **Single Channel (X2/S2)** is checked
5. Click **Connect** → then **Start Scan**

Points should appear immediately in the Viewport.

---

## User Interface Guide

See [docs/UI_GUIDE.md](docs/UI_GUIDE.md) for a full panel-by-panel walkthrough.

---

## Recording & Replay Workflow

1. **Record**: Enter a filename in **Logging & Playback** → click **Start Recording**. A `.ols_meta` and `.ols_data` file are written to the working directory.
2. **Stop**: Click **Stop Recording**.
3. **Export** (optional): Click **Export to CSV** or **Export to JSON** for use in Python, MATLAB, etc.
4. **Replay**: Switch to the **Replay** tab in the center panel → enter the same filename base → click **Load** → use transport controls to play/pause/scrub.

---

## Project Architecture

```
OpenLidarStudio/
├── src/
│   ├── hardware/       # LidarController — serial connection, worker thread
│   ├── pipeline/       # SignalFilter, AutoRanger, AsyncLogger (ring-buffer I/O)
│   ├── rendering/      # GLRenderer, Shader — OpenGL point-cloud pipeline
│   └── ui/
│       └── panels/     # ViewportPanel, ControlPanel, TelemetryPanel,
│                       # LoggingPanel, ReplayPanel
├── include/            # All C++ headers (mirrors src/)
├── assets/shaders/     # pointcloud.vert / pointcloud.frag (GLSL 330)
├── 3rdparty/
│   └── ydlidar_sdk/    # Git submodule — YDLIDAR C++ SDK v1.4.7
├── docs/               # Documentation and screenshots
└── .github/workflows/  # CI/CD — cross-platform release pipeline
```

---

## Supported LiDAR Models

| Model | Single-Channel | Baudrate | Tested |
|---|---|---|---|
| YDLIDAR X2 | ✅ Yes | 115200 | ✅ |
| YDLIDAR X4 | ❌ No | 128000 | — |
| YDLIDAR G4 | ❌ No | 230400 | — |
| YDLIDAR S2 | ✅ Yes | 115200 | — |

---

## Contributing

Pull requests are welcome. Please open an issue first to discuss significant changes.

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/my-feature`)
3. Commit your changes
4. Push and open a Pull Request

---

## License

MIT License — see [LICENSE](LICENSE) for details.

---

## Acknowledgements

- [YDLIDAR SDK](https://github.com/YDLIDAR/sdk) — hardware driver
- [Dear ImGui](https://github.com/ocornut/imgui) — immediate-mode UI
- [ImPlot](https://github.com/epezent/implot) — real-time plotting
- [GLFW](https://www.glfw.org/) — window & input
- [GLM](https://github.com/g-truc/glm) — math library
- [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake) — dependency management
