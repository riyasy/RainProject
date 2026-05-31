import CoreGraphics

private let kSnowGravity: CGFloat        = 10
private let kSnowMaxSpeed: CGFloat       = 75
private let kNoiseIntensity: CGFloat     = 20
private let kNoiseScale: Double          = 0.01
private let kSnowMinRadius: CGFloat      = 0.8
private let kSnowMaxRadius: CGFloat      = 2.5

enum SnowUpdateResult { case settled, offScreen, falling }

enum SnowflakeShape { case simple, crystal, hexagon, star }

struct SnowFlake {
    var pos: CGPoint
    var vel: CGPoint
    let radius: CGFloat
    let shape: SnowflakeShape
    var rotation: Double
    let rotationSpeed: Double

    private let screenBounds: CGRect

    init(screenBounds: CGRect, stagger: Bool = false) {
        self.screenBounds = screenBounds
        radius       = CGFloat.random(in: kSnowMinRadius...kSnowMaxRadius)
        rotation     = Double.random(in: 0...(2 * .pi))
        rotationSpeed = Double.random(in: -2.0...2.0)
        vel          = CGPoint(x: 0, y: -CGFloat.random(in: 5...10))

        let r = Int.random(in: 0...99)
        shape = r < 40 ? .simple : r < 70 ? .crystal : r < 90 ? .hexagon : .star

        let xMargin = screenBounds.width * 0.5
        let x = CGFloat.random(in: (screenBounds.minX - xMargin)...(screenBounds.maxX + xMargin))
        let y = stagger ? CGFloat.random(in: screenBounds.minY...screenBounds.maxY)
                        : screenBounds.maxY + radius
        pos = CGPoint(x: x, y: y)
    }

    mutating func update(dt: CGFloat, time: Double,
                         heightMap: [CGFloat], screenBounds: CGRect) -> SnowUpdateResult {
        // 3D Perlin noise field drives an *acceleration* (matches the Windows build):
        // velocity accumulates and is only capped, so flakes carry momentum and the
        // noise modulates each flake's vertical speed by where it is — neighbours fall
        // at different rates instead of all converging on terminal velocity.
        let n = SnowNoise.value(Double(pos.x) * kNoiseScale,
                                Double(pos.y) * kNoiseScale,
                                time * kNoiseScale)
        let angle = n * (2 * .pi) + .pi / 2   // PI/2 bias → baseline push along fall axis

        // AppKit Y is up, so "down" is negative — sign-flipped vs the Windows version.
        vel.x += CGFloat(cos(angle)) * kNoiseIntensity * dt * 2
        vel.y -= CGFloat(sin(angle)) * kNoiseIntensity * dt
        vel.y -= kSnowGravity * dt

        let spd = (vel.x * vel.x + vel.y * vel.y).squareRoot()
        if spd > kSnowMaxSpeed { let s = kSnowMaxSpeed / spd; vel.x *= s; vel.y *= s }

        rotation += rotationSpeed * Double(dt)

        pos.x += vel.x * dt
        pos.y += vel.y * dt

        // Only settle within the visible width — flakes drifting in the off-screen
        // spawn margin must not dump their snow into the clamped edge columns.
        let withinX = pos.x >= screenBounds.minX && pos.x <= screenBounds.maxX
        if withinX {
            let col   = SnowSystem.column(forX: pos.x, screenBounds: screenBounds, count: heightMap.count)
            let floor = screenBounds.minY + heightMap[col]
            if pos.y - radius <= floor { return .settled }
        }

        let margin = screenBounds.width * 0.5
        if pos.x < screenBounds.minX - margin || pos.x > screenBounds.maxX + margin
            || pos.y < screenBounds.minY - radius {
            return .offScreen
        }

        return .falling
    }
}
