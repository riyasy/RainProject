import CoreGraphics

private let kMaxSnowFraction: CGFloat = 0.35  // snow pile capped at 35% of screen height

private let kSnowColumnSpacing: CGFloat = 12   // ~12 pt per pile column (downsampled)
private let kSnowMaxColumns = 160

struct SnowSystem {
    var flakes: [SnowFlake] = []
    var heightMap: [CGFloat]    // settled snow height per pile column (pt above floor)

    private let screenBounds: CGRect
    private let maxHeight: CGFloat

    init(screenBounds: CGRect, count: Int) {
        self.screenBounds = screenBounds
        self.maxHeight    = screenBounds.height * kMaxSnowFraction
        let cols = max(2, min(kSnowMaxColumns, Int(screenBounds.width / kSnowColumnSpacing)))
        heightMap = [CGFloat](repeating: 0, count: cols)
        flakes    = (0..<count).map { _ in SnowFlake(screenBounds: screenBounds, stagger: true) }
    }

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

    mutating func clearSnow() {
        heightMap = [CGFloat](repeating: 0, count: heightMap.count)
    }

    // Map a screen x-coordinate to a pile column index (clamped).
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
