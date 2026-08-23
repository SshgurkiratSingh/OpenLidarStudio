# UI Guide — OpenLidarStudio

This document provides a detailed walkthrough of every panel in the OpenLidarStudio interface.

---

## Layout Overview

The window is divided into three persistent columns:

```
┌─────────────────┬──────────────────────────┬──────────────────┐
│  Control &      │                          │  Telemetry &     │
│  Toolbar        │  Viewport  |  Replay     │  Analytics       │
│  ─────────────  │  (tab bar)               │                  │
│  Logging &      │                          │                  │
│  Playback       │                          │                  │
└─────────────────┴──────────────────────────┴──────────────────┘
```

All panels are dockable — you can drag and rearrange them freely. The layout above is the default.

---

## Left Sidebar

### Control & Toolbar

#### Connection
| Control | Description |
|---|---|
| **Refresh Ports** | Rescans `/dev/*` (Linux) or COM ports (Windows) |
| **Port selector** | Dropdown of available serial ports |
| **Baudrate** | Common baudrates + a custom input field |
| **Single Channel (X2/S2)** | Must be checked for YDLIDAR X2 / S2 models |
| **Connect / Disconnect** | Opens or closes the serial connection |
| **Start Scan / Stop Scan** | Sends `turnOn()` / `turnOff()` to the hardware motor |

#### LiDAR Parameters
| Control | Description |
|---|---|
| **Reset Defaults** | Restores all sliders to factory values |
| **Target RPM** | Sets motor speed (300–900 RPM = 5–15 Hz) |

#### Visualization & History
| Control | Description |
|---|---|
| **Color Mode** | Range (Turbo colormap) or Intensity |
| **Enable Motion History** | Toggles the phosphor-decay trail system |
| **History Length** | Number of past sweeps to retain (2–30) |
| **Decay Rate** | How fast old points fade to blue and disappear (GPU uniform) |

#### Auto-Ranger & Display
| Control | Description |
|---|---|
| **Auto-Ranging** | When checked, display range adapts automatically |
| **Alpha Min / Alpha Max** | EMA smoothing bounds |
| **k Sensitivity** | Multiplier for range expansion on large jumps |
| **Manual Range** | Overrides auto range when Auto-Ranging is off |

#### Filters (Culling)
| Control | Description |
|---|---|
| **Min / Max Radius** | Discard points outside this distance band |
| **Min / Max Angle** | Discard points outside this angular sweep (0–360°) |

---

### Logging & Playback

| Control | Description |
|---|---|
| **Filename** | Base name for output files (no extension) |
| **Start Recording** | Begins writing `.ols_meta` + `.ols_data` to disk |
| **Stop Recording** | Flushes and closes the output files |
| **Duration** | Live elapsed time displayed during recording |
| **Export to CSV** | Converts the binary recording to a flat CSV |
| **Export to JSON** | Converts the binary recording to a JSON array of frames |

**Output files:**
- `<filename>.ols_meta` — binary stream of `TelemetryHeader` structs (per frame)
- `<filename>.ols_data` — binary stream of `SerializedPoint` arrays (per frame)
- `<filename>.csv` — flat table, one row per point per frame
- `<filename>.json` — array of frame objects, each with a header and points array

---

## Center Panel

### Viewport Tab

The main live scan visualization.

- **Point cloud** rendered in OpenGL; colored by range (Turbo colormap) or intensity
- **Distance rings** labeled in metres, automatically scaled to `r_final`
- **Motion trails** — if enabled, past sweeps layer behind the live scan, fading in size, color, and alpha based on age
- **Crosshair** at the sensor origin
- **Hover tooltip** — move the mouse anywhere in the viewport to see:
  - Exact distance (m) from the origin
  - Bearing angle (°)
  - A yellow aiming line drawn from the origin to the cursor

### Replay Tab

Frame-accurate playback of any `.ols_meta` / `.ols_data` recording.

| Control | Description |
|---|---|
| **Filename input** | Base name (same as used during recording) |
| **Load** | Decodes the entire recording into memory |
| **Clear** | Unloads the recording |
| **Frame N / Total** | Current frame indicator |
| **\|<** | Jump to first frame |
| **<** | Step back one frame |
| **Play / Pause** | Automatic playback at the set FPS |
| **>** | Step forward one frame |
| **>\|** | Jump to last frame |
| **FPS slider** | Playback speed (1–60 FPS) |
| **Scrub bar** | Drag to any frame in the recording |

The same OpenGL viewport and distance rings are rendered in replay mode, giving you an identical spatial view to the live scan.

---

## Right Sidebar

### Telemetry & Analytics

Three live scrolling plots covering the last **30 seconds**:

| Plot | Y-axis | Signals |
|---|---|---|
| **Range Dynamics** | Range (m) | Inst Range (blue), Smooth Range (orange) |
| **Alpha Adaptive** | Alpha coefficient | α (green) |
| **Point Count** | Points per sweep | Points (purple) |

All plots share a synchronized scrolling X-axis. The stats bar at the top shows:
- **Points / Sweep** — last sweep's point count
- **RPM** — motor frequency reported by the SDK
- **Range** — current `r_final` value from the AutoRanger

---

## Keyboard & Mouse

| Action | Effect |
|---|---|
| Hover in Viewport | Shows distance/angle tooltip and aiming line |
| Drag panel title bar | Reposition panel (docking system) |
| Close panel (✕) | Remove from layout; reopen via View menu |

---

## File Format Reference

### `.ols_meta`

Binary stream of `TelemetryHeader` records (one per captured sweep):

```cpp
struct TelemetryHeader {
    uint64_t timestamp_ns;   // UNIX nanosecond timestamp
    uint32_t frame_index;    // Sequential frame number
    float    motor_rpm;      // Motor frequency
    float    r_inst;         // Instantaneous auto-range estimate
    float    r_smooth;       // Exponentially smoothed range
    float    delta_r;        // Range delta used for adaptation
    float    alpha_adaptive; // EMA coefficient at this frame
    uint32_t point_count;    // Number of points in the paired data block
};
```

### `.ols_data`

Binary stream of `SerializedPoint` arrays. Each frame's block is `point_count × sizeof(SerializedPoint)` bytes, in the same order as the corresponding header in `.ols_meta`.

```cpp
struct SerializedPoint {
    float x;         // Cartesian X (metres)
    float y;         // Cartesian Y (metres)
    float range;     // Polar range (metres)
    float angle;     // Polar angle (radians)
    float intensity; // Reflectance (SDK-dependent scale)
};
```

### CSV columns

`timestamp_ns, frame_index, motor_rpm, r_inst, r_smooth, delta_r, alpha_adaptive, x, y, range, angle, intensity`
