# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

"Let It Rain" is a Windows desktop screensaver/wallpaper application written in C++ that creates animated rain and snow effects across multiple monitors. The application uses Direct2D for hardware-accelerated graphics rendering and Windows Composition Engine for transparent overlay windows.

## Build Commands

### Visual Studio Project
- **Build Debug (x64)**: Open `let-it-rain.vcxproj` in Visual Studio and build using the x64 Debug configuration
- **Build Release (x64)**: Build using the x64 Release configuration  
- **MSBuild Command Line**: `msbuild let-it-rain.vcxproj /p:Configuration=Release /p:Platform=x64`

### Build Configurations
- **Platform Toolset**: v143 (Visual Studio 2022)
- **C++ Standard**: C++17
- **Target Platforms**: Win32, x64 (x64 recommended)
- **Build Outputs**: 
  - Debug: `x64/Debug/let-it-rain.exe`
  - Release: `x64/Release/let-it-rain.exe`

## Architecture Overview

### Core Components

1. **DisplayWindow** (`DisplayWindow.h/.cpp`)
   - Main window class that manages the transparent overlay window for each monitor
   - Handles Direct2D rendering context and device initialization
   - Manages the animation loop and particle system updates

2. **Particle System**
   - **RainDrop** (`RainDrop.h/.cpp`): Individual rain drop physics and rendering
   - **SnowFlake** (`SnowFlake.h/.cpp`): Snow particle behavior and rendering
   - **Splatter** (`Splatter.h/.cpp`): Ground impact effects for rain drops

3. **Settings Management**
   - **SettingsManager** (`SettingsManager.h/.cpp`): Persists user preferences
   - **OptionDialog** (`OptionDialog.h/.cpp`): Configuration UI dialog
   - **Setting** class: Particle count, wind speed, colors, particle type

4. **System Integration**
   - **CPUUsageTracker** (`CPUUsageTracker.h`): Monitors CPU usage to hide animation when system is busy
   - **CallBackWindow** (`CallBackWindow.h`): Notification area (system tray) integration
   - Multi-monitor support via `MonitorEnumProc` callback

### Key Design Patterns

- **Per-Monitor Architecture**: Creates separate `DisplayWindow` instances for each detected monitor
- **Performance Optimization**: Automatically hides animation when CPU usage exceeds 80%
- **Direct2D Integration**: Uses hardware-accelerated 2D graphics with Windows Composition Engine
- **Settings Persistence**: User preferences saved/loaded via `SettingsManager`

### Graphics Pipeline

1. **Initialization**: Direct2D device context and composition target setup per monitor
2. **Animation Loop**: 60 FPS animation updates in main message loop
3. **Particle Physics**: Terminal velocity simulation with wind effects
4. **Rendering**: Hardware-accelerated Direct2D drawing with transparency

### Development Notes

- Uses Windows Composition Engine for layered window effects
- Requires Windows 10+ for DPI awareness and composition features  
- FastNoiseLite integration for procedural effects
- All warnings treated as errors in Debug builds
- Resource files (.ico) automatically copied to output directory

### File Structure

- **Core Engine**: `Main.cpp`, `DisplayWindow.*`, `Global.h`
- **Particle System**: `RainDrop.*`, `SnowFlake.*`, `Splatter.*`
- **Utilities**: `Vector2.*`, `MathUtil.h`, `RandomGenerator.h`
- **UI/Settings**: `OptionDialog.*`, `SettingsManager.*`
- **System**: `CPUUsageTracker.h`, `CallBackWindow.h`
- **Resources**: `let-it-rain.rc`, `*.ico` files