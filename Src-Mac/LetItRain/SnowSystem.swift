import CoreGraphics

// Settled-pile tuning knobs. Each note says what raising (↑) / lowering (↓) it does.
private let kMaxSnowFraction: CGFloat = 0.35  // pile height cap, as a fraction of screen height.
                                              // ↑ lets drifts grow taller before they stop accumulating; ↓ keeps the pile shallow.
private let kSnowColumnSpacing: CGFloat = 12  // pt per pile column (the heightMap is downsampled to this).
                                              // ↑ coarser, chunkier mounds (fewer columns); ↓ finer, smoother slopes (more columns, more CPU).
private let kSnowMaxColumns = 160             // hard cap on column count, bounding pile cost on very wide screens.

/// Owns the live snowflakes and the settled-snow pile. The pile is stored as a
/// downsampled `heightMap` (one height per ~12 pt column); flakes that settle
/// add to their column, and `smoothHeightMap()` relaxes the columns into organic
/// slopes. Stepped once per frame by `update(dt:time:targetCount:)`.
struct SnowSystem {
    var flakes: [SnowFlake] = []
    var heightMap: [CGFloat]    // settled snow height per pile column (pt above floor)

    private let screenBounds: CGRect
    private let maxHeight: CGFloat   // per-column height cap (kMaxSnowFraction of screen height)

    /// Build the pile columns (sized to the screen width) and the initial,
    /// staggered flake population.
    init(screenBounds: CGRect, count: Int) {
        self.screenBounds = screenBounds
        self.maxHeight    = screenBounds.height * kMaxSnowFraction
        let cols = max(2, min(kSnowMaxColumns, Int(screenBounds.width / kSnowColumnSpacing)))
        heightMap = [CGFloat](repeating: 0, count: cols)
        flakes    = (0..<count).map { _ in SnowFlake(screenBounds: screenBounds, stagger: true) }
    }

    /// Advance one frame: reconcile the flake count to `targetCount`, move every
    /// flake, fold settled flakes into the pile (respawning them), respawn ones
    /// that drift off-screen, and relax the pile.
    mutating func update(dt: CGFloat, time: Double, targetCount: Int) {
        // Reconcile flake count
        if flakes.count < targetCount {
            flakes += (flakes.count..<targetCount).map { _ in SnowFlake(screenBounds: screenBounds) }
        } else if flakes.count > targetCount {
            flakes.removeLast(flakes.count - targetCount)
        }

        for i in flakes.indices {
            switch flakes[i].update(dt: dt, time: time, heightMap: heightMap, screenBounds: screenBounds) {
            case .settled:
                let col = SnowSystem.column(forX: flakes[i].pos.x,
                                            screenBounds: screenBounds, count: heightMap.count)
                heightMap[col] = min(heightMap[col] + flakes[i].radius * 0.5, maxHeight)
                flakes[i] = SnowFlake(screenBounds: screenBounds)
            case .offScreen:
                flakes[i] = SnowFlake(screenBounds: screenBounds)
            case .falling:
                break
            }
        }

        smoothHeightMap()
    }

    /// Flatten the settled pile back to zero (keeps the column count).
    mutating func clearSnow() {
        heightMap = [CGFloat](repeating: 0, count: heightMap.count)
    }

    /// Map a screen x-coordinate to a pile column index (clamped to `0..<count`).
    /// Shared by `SnowSystem` (accumulating) and `SnowFlake` (settling test).
    static func column(forX x: CGFloat, screenBounds: CGRect, count: Int) -> Int {
        let frac = (x - screenBounds.minX) / max(screenBounds.width, 1)
        let raw  = frac * CGFloat(count)
        return Int(min(max(raw, 0), CGFloat(count - 1)))
    }

    // MARK: Private

    // Gradually flow excess height sideways so the pile has organic rounded slopes.
    private mutating func smoothHeightMap() {
        let n = heightMap.count
        guard n > 2 else { return }
        // Forward pass
        for x in 1..<(n - 1) {
            let diff = heightMap[x] - heightMap[x - 1]
            if diff > 2 { let f = diff * 0.08; heightMap[x] -= f; heightMap[x - 1] += f }
        }
        // Backward pass
        for x in stride(from: n - 2, through: 1, by: -1) {
            let diff = heightMap[x] - heightMap[x + 1]
            if diff > 2 { let f = diff * 0.08; heightMap[x] -= f; heightMap[x + 1] += f }
        }
    }
}
