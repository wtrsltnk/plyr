# 🎵 plyr

A sleek, minimalist MP3 player built with modern C++ that brings back the joy of simple, focused music playback.

## ✨ Features

### 🎧 Core Playback
- **Fast MP3 decoding** powered by minimp3
- **Gapless playback** with automatic track advancement
- **Smooth seeking** - scrub through tracks with precision
- **Real-time spectrum analyzer** - 32-band frequency visualization with smooth decay

### 🎨 Modern Interface
- **Borderless design** - Clean, minimal window with custom title bar
- **Draggable** - Move the window by grabbing the title bar
- **Collapsible playlist** - Expand or minimize to save screen space
- **Marquee scrolling** - Long track names scroll smoothly across the display
- **Responsive controls** - Play, pause, stop, skip forward/back

### 📁 Playlist Management
- **Custom music folders** - Point to your music library via command-line
- **File browser** - Navigate directories and add songs on the fly
- **Playlist editing** - Reorder, duplicate, and remove tracks
- **Persistent playback** - Auto-advance through your queue

### ⌨️ Keyboard Shortcuts
- **Space** - Play/Pause toggle
- **Left/Right Arrow** - Seek backward/forward 10 seconds
- **Ctrl + Left/Right Arrow** - Seek backward/forward 60 seconds
- **Up/Down Arrow** - Navigate playlist or file browser
- **Enter** - Play selected track / Open selected item
- **Ctrl+O** - Open file browser
- **Ctrl+P** - Toggle playlist view
- **Mouse drag** - Seek through tracks via timeline

### 🎼 Visual Feedback
- **Live spectrum display** - Watch frequencies dance to your music
- **Smooth decay** - Bars gracefully fall when music pauses
- **Progress timeline** - Visual track position indicator
- **Time display** - Current time / Total duration

## 🚀 Getting Started

### Prerequisites
- CMake 3.14+
- C++23 compatible compiler (MSVC 2022, GCC 12+, Clang 15+)
- Windows, Linux, or macOS

### Building

```bash
# Clone the repository
git clone <your-repo-url>
cd plyr

# Configure and build
cmake -B build
cmake --build build

# Run
./build/plyr.exe [path/to/music/folder]
```

### Running

**Default mode:**
```bash
plyr.exe
```
Starts with current directory as music root.

**Custom music folder:**
```bash
plyr.exe "C:\Users\YourName\Music"
```
Opens file browser in specified directory.

## 🎮 Usage

### Adding Music
1. Click the **folder icon** to open the file browser
2. Navigate to your music files
3. Double-click or select and press "Open" to add to playlist

### Playback Controls
- **▶ Play** - Start playback of selected track
- **⏸ Pause** - Pause current track (spectrum gradually fades)
- **⏹ Stop** - Stop and reset to beginning
- **⏮ Previous** - Jump to previous track in playlist
- **⏭ Next** - Jump to next track in playlist
- **📋 Playlist Toggle** - Show/hide playlist view

### Playlist Management
- **Drag to reorder** - Click and drag tracks
- **⬆⬇ Arrows** - Move selected track up/down
- **🗑️ Delete** - Remove track from playlist
- **📋 Duplicate** - Copy selected track
- **💾 Save** - Save playlist (coming soon)

## 🛠️ Technical Details

### Technologies
- **SDL3** - Cross-platform window/audio system
- **ImGui** - Immediate mode GUI
- **minimp3** - Ultra-fast MP3 decoding
- **GLM** - Math library
- **OpenGL** - Graphics rendering
- **C++23** - Modern C++ features

### Architecture
- **Single-threaded** - Simple, predictable execution
- **Push-based audio** - SDL3 audio streams for low latency
- **MDCT spectrum** - Direct frequency data from MP3 decoder
- **Real-time processing** - 512-sample DFT for visualization

### Performance
- **Low CPU usage** - Efficient decoding and rendering
- **Minimal memory** - Streaming playback, no full file buffering
- **60 FPS UI** - Smooth, responsive interface
- **Fast seeking** - Index-based sample-accurate positioning

## 📝 File Format Support

Currently supports:
- ✅ **MP3** (MPEG-1/2/2.5 Layer III)

Not supported (will display error):
- ❌ MP4/M4A/AAC containers
- ❌ WAV, FLAC, OGG Vorbis
- ❌ Other formats

## 🎨 Customization

### Window Size
The window starts at 1024x768 and can collapse to a compact 1024x180 mini-player mode using the playlist toggle button.

### Spectrum Visualization
Adjust the spectrum sensitivity in `src/app.cpp`:
```cpp
const float scale = 0.4f; // Lower = less sensitive, Higher = more reactive
```

Adjust decay speed in `src/decode.c`:
```cpp
const float decay_rate = 0.98f; // Higher = slower fall, Lower = faster fall
```

## 🐛 Known Issues

- MP4/M4A files with .mp3 extension will fail to load (use actual MP3s)
- Playlist not saved between sessions (manual management required)
- No shuffle or repeat modes yet

## 🤝 Contributing

This is a personal project, but feel free to fork and experiment!

## 📜 License

See LICENSE file for details.

## 🙏 Acknowledgments

- **minimp3** by lieff - Incredible single-header MP3 decoder
- **Dear ImGui** by ocornut - Best immediate mode GUI library
- **SDL3** team - Solid foundation for cross-platform apps
- **Icon fonts** - Lucide and Fontaudio icon sets

---

Built with ❤️ for music lovers who appreciate simplicity.
