# Simulation Visualizer with Video Export

The `visualize_sim.py` script visualizes hex-world simulation logs with support for both interactive display and MP4 video export.

## Features

- **Interactive Mode**: Real-time animation display in a matplotlib window
- **Video Export**: Save simulations as MP4 files for sharing and archiving  
- **Automatic Filename**: Default output filename matches input (e.g., `sim.json` → `sim.mp4`)
- **Customizable FPS**: Frame rate derived from `--delay` parameter (FPS = 1/delay)
- **Dual Writer Support**: Uses FFmpeg (preferred) or Pillow (fallback) for video encoding

## Installation

### Requirements
- Python 3.7+
- matplotlib
- numpy

### Optional (for video export)
- **FFmpeg** (recommended for better quality/smaller files):
  ```bash
  # Linux
  sudo apt install ffmpeg
  
  # macOS
  brew install ffmpeg
  ```

- **Pillow** (automatic fallback if FFmpeg unavailable):
  - Included with matplotlib, no additional installation needed
  - Produces larger file sizes than FFmpeg

## Usage

### Interactive Display (Default)

```bash
python3 scripts/visualize_sim.py --input sim.json
python3 scripts/visualize_sim.py --input sim.json --delay 0.5
python3 scripts/visualize_sim.py --input sim.json --start-step 100
```

### Video Export

**Default filename** (input.stem + .mp4):
```bash
python3 scripts/visualize_sim.py --input sim.json --save-video
# Creates: sim.mp4
```

**Custom filename**:
```bash
python3 scripts/visualize_sim.py --input sim.json --save-video --output my_animation.mp4
# Creates: my_animation.mp4
```

**Different playback speed** (affects FPS):
```bash
# Slower video (0.5 FPS)
python3 scripts/visualize_sim.py --input sim.json --save-video --delay 2.0

# Faster video (10 FPS)
python3 scripts/visualize_sim.py --input sim.json --save-video --delay 0.1
```

## Command-Line Arguments

| Argument | Type | Default | Description |
|----------|------|---------|-------------|
| `--input` | str | `sim.json` | Path to simulation JSON log file |
| `--delay` | float | `0.35` | Delay in seconds between frames (FPS = 1/delay) |
| `--hex-size` | float | `1.0` | Hex size scaling factor |
| `--start-step` | int | `0` | Start visualization from this step |
| `--save-video` | flag | `False` | Save animation as MP4 instead of displaying |
| `--output` | str | `<input>.mp4` | Output video filename (only with --save-video) |

## Examples

### Quick video export
```bash
python3 scripts/visualize_sim.py --input results/training_run.json --save-video
# Creates: results/training_run.mp4
```

### High-quality slow-motion
```bash
python3 scripts/visualize_sim.py --input sim.json --save-video --delay 1.0 --output slow_motion.mp4
# Creates: slow_motion.mp4 at 1 FPS (1 second per frame)
```

### Fast playback
```bash
python3 scripts/visualize_sim.py --input sim.json --save-video --delay 0.05 --output fast.mp4
# Creates: fast.mp4 at 20 FPS
```

### Skip warmup period
```bash
python3 scripts/visualize_sim.py --input sim.json --save-video --start-step 500
# Skip first 500 steps
```

## Output

### Video Files
- **Format**: MP4 (H.264 codec with FFmpeg, or GIF-based with Pillow)
- **File size**: Varies by simulation length and writer (FFmpeg typically 10-50 MB)
- **Metadata**: Includes artist tag "Perimeter-MG"

### Playback
Videos can be played with:
- VLC Media Player
- ffplay (command line): `ffplay sim.mp4`
- Browser video players
- Any standard MP4-compatible player

## Troubleshooting

### "No video writer available"
**Problem**: Neither FFmpeg nor Pillow is available.

**Solution**: Install FFmpeg:
```bash
# Linux
sudo apt install ffmpeg

# macOS
brew install ffmpeg

# Windows
# Download from https://ffmpeg.org/download.html
```

### "Using Pillow (may produce larger files)"
**Problem**: FFmpeg not found, falling back to Pillow.

**Solution**: This is just a warning. Video will still be created, but files will be larger. Install FFmpeg for better compression.

### Video playback speed wrong
**Problem**: Video plays too fast or too slow.

**Solution**: Adjust `--delay` parameter. FPS = 1 / delay.
- `--delay 0.5` → 2 FPS (slow)
- `--delay 0.1` → 10 FPS (fast)
- `--delay 0.35` → ~2.86 FPS (default)

### Out of memory during video export
**Problem**: Large simulations (>5000 frames) may consume significant RAM.

**Solution**:
- Reduce simulation length or use `--start-step` to skip early frames
- Use a system with more RAM
- Export in segments (run script multiple times with different --start-step)

## Notes

- Video export can be slow for long simulations (1-2 minutes for 1000 frames)
- Interactive mode is unchanged and works as before
- The FPS in saved videos exactly matches the specified delay
- All visualization features (hex overlay, agent colors, reward plots) are preserved in videos

## Implementation Details

The script uses `matplotlib.animation.FuncAnimation` for video export:
- Frames are rendered identically to interactive mode
- Writer is auto-selected: FFmpeg (preferred) or Pillow (fallback)
- No temporary files are created during export
- Progress is shown during video rendering
