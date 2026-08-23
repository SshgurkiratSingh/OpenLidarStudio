# Changelog

All notable changes to OpenLidarStudio will be documented in this file.

Format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Versions follow [Semantic Versioning](https://semver.org/).

---

## [Unreleased]

### Planned
- macOS support (Metal/MoltenVK backend)
- Multi-sensor simultaneous view
- SLAM overlay mode

---

## [1.0.0] — 2026-08-23

### Added
- **Live 2D LiDAR visualization** with OpenGL 3.3+ point-cloud renderer
- **Turbo colormap** shader — points colored by range or intensity
- **Motion history trails** — configurable length (2–30 sweeps), decay rate, toggle on/off
- **Adaptive auto-ranger** — exponential smoothing with configurable α and k-sensitivity
- **Signal filter / culling** — radius band + angular sweep mask
- **Concentric distance rings** — labeled in metres, auto-scaled to current range
- **Hover tooltip** — live distance (m) and angle (°) at mouse cursor
- **Async binary logger** — lock-free ring buffer; writes `.ols_meta` + `.ols_data`
- **Replay system** — load any recording, scrub frame-by-frame, play at 1–60 FPS
- **Data export** — one-click CSV and JSON conversion from recordings
- **Telemetry panel** — scrolling 30-second plots for range, alpha, and point-count
- **3-column dockable layout** — Control (left), Viewport+Replay tabs (center), Telemetry (right)
- **Single-Channel mode** support for YDLIDAR X2 and S2
- **Reset Defaults** button for all tuning parameters
- **GitHub Actions release pipeline** — cross-platform Linux + Windows binaries with SHA256 checksums

### Hardware
- Validated on YDLIDAR X2 at 115200 baud on Linux (Fedora 39)

### Dependencies
- YDLIDAR SDK v1.4.7 (embedded submodule)
- Dear ImGui v1.90.4-docking
- ImPlot v0.16
- GLFW 3.3.8
- GLM 0.9.9.8
- glad 0.1.36
