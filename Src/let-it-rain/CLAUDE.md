# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Let It Rain is a Windows desktop rain and snow simulation application written in C++20. It creates an ASMR-inspired visual experience with realistic raindrops and snowflakes falling across the desktop using DirectX/Direct2D rendering.

## Build System

**Primary Build Method:**
- Open `../let-it-rain.sln` in Visual Studio 2022
- Build using MSBuild: `msbuild ../let-it-rain.sln /p:Configuration=Release /p:Platform=x64`
- Project uses C++20 standard (`<LanguageStandard>stdcpp20</LanguageStandard>`)

**Supported Configurations:**
- Debug/Release for x86 and x64
- Primary development target: x64
- Built executable: `x64/Release/let-it-rain.exe`

**Dependencies:**
- Windows 10/11 required
- DirectX 11.2, Direct2D, DXGI 1.3
- Windows Composition API (DirectComposition)

## Architecture

**Core Components:**
- `Main.cpp` - Entry point with multi-monitor support and main message loop
- `DisplayWindow` - Per-monitor window management and DirectX initialization
- `DisplayData` - Modern RAII-based graphics resource management
- `ParticleSystem<T>` - C++20 concept-based template for particle management
- `RainDrop`/`SnowFlake` - Particle implementations with physics simulation
- `Vector2` - Constexpr mathematics with complete operator overloading
- `SettingsManager` - Configuration persistence and UI integration

**Key Patterns:**
- Multi-monitor architecture with one DisplayWindow per screen
- Modern C++20 features: concepts, constexpr, smart pointers, RAII
- Template-based particle system with parallel execution support
- Error handling using custom Result<T>/Expected<T> system (no exceptions)
- Namespace organization under `RainEngine::`

**Graphics Pipeline:**
- DirectComposition-based layered windows for desktop integration
- Direct2D device context rendering with hardware acceleration
- Per-frame particle updates with physics simulation
- Collision detection against taskbar and window boundaries

## Development Workflow

**Code Style:**
- C++20 modern practices with constexpr, concepts, and smart pointers
- RAII memory management throughout
- Const correctness and noexcept specifications
- Namespace organization: `RainEngine::` for new code, global aliases for compatibility

**Modernization Status:**
- Phase 1 (Completed): Core infrastructure, error handling, Vector2, DisplayData
- Phase 2 (In Progress): Particle system templates, RainDrop/SnowFlake modernization
- See `MODERNIZATION_GUIDE.md` for detailed migration status

**Testing:**
- No automated test framework - manual testing required
- Test on multiple monitors with different DPI settings
- Verify particle performance under high load scenarios

**Performance Considerations:**
- Particle systems use parallel execution for large counts
- Memory-efficient container operations with std::span
- SIMD-friendly data layouts where possible
- CPU usage monitoring via `CPUUsageTracker`

## Common Tasks

**Adding New Particle Types:**
1. Implement the `Particle` concept (UpdatePosition, Draw, IsAlive methods)
2. Create specialization of `ParticleSystem<YourParticle>`
3. Add to DisplayWindow's particle management

**Modifying Graphics:**
- Direct2D resources managed in `DisplayData` class
- Use RAII wrappers for all DirectX objects
- Color management through `DisplayData::SetRainColor/SetSnowColor`

**Settings Integration:**
- UI components in `OptionDialog.cpp/h`
- Persistence through `SettingsManager`
- Registry-based configuration storage

**Multi-Monitor Support:**
- Handled automatically in `Main.cpp` via `EnumDisplayMonitors`
- Each monitor gets independent `DisplayWindow` instance
- Global settings shared across all displays