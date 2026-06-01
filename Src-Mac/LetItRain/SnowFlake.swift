import CoreGraphics

// Snow tuning knobs. Each note says what raising (↑) / lowering (↓) it does.
private let kSnowGravity: CGFloat        = 10    // steady downward pull (accel). ↑ flakes fall faster & straighter; ↓ driftier, hangs longer.
private let kSnowMaxSpeed: CGFloat       = 75    // speed cap (px/s). ↑ lets flakes move faster (more frantic gusts); ↓ keeps it calm.
private let kNoiseIntensity: CGFloat     = 20    // strength of the noise-driven wind/swirl. ↑ wilder sideways drift & speed variation; ↓ straighter, uniform fall.
private let kNoiseScale: Double          = 0.01  // noise frequency in space & time. ↑ tighter, more chaotic eddies; ↓ broad, slow-rolling gusts.
private let kSnowMinRadius: CGFloat      = 0.8   // smallest flake (pt).
private let kSnowMaxRadius: CGFloat      = 2.5   // largest flake (pt). ↑ both = chunkier snow; bigger flakes also settle faster (deposit ∝ radius).

// Falling-flake count per intensity unit. Mirrors the Windows build's
// SnowFlake::SNOW_FLAKE_MULTIPLIER; tuned together with kSnowEdgeMargin so the
// on-screen density stays constant while fewer off-screen flakes are simulated.
// ↑ denser snowfall per intensity step (more flakes, more CPU); ↓ sparser.
let kSnowFlakeMultiplier = 14

// Horizontal off-screen spawn/despawn margin, as a fraction of screen width
// (drift headroom for seamless edges). Smaller = fewer off-screen flakes
// simulated; too small risks flakes popping in/out at the edges under drift.
let kSnowEdgeMargin: CGFloat = 0.2

/// Outcome of advancing a flake one step, telling `SnowSystem` what to do next.
enum SnowUpdateResult {
    case settled    // reached the pile — accumulate its height and respawn
    case offScreen  // drifted out of bounds — respawn
    case falling    // still in the air — keep it
}

/// The four flake silhouettes. `SnowRenderer` bakes one atlas cell per case.
enum SnowflakeShape { case simple, crystal, hexagon, star }

/// One snowflake: a drifting body with momentum, a fixed shape/size, and a
/// spin. Motion is driven by a Perlin-noise *acceleration* field (see `update`)
/// rather than a fixed velocity, so neighbouring flakes fall at different rates.
struct SnowFlake {
    var pos: CGPoint
    var vel: CGPoint
    let radius: CGFloat
    let shape: SnowflakeShape
    var rotation: Double           // current spin angle (radians)
    let rotationSpeed: Double      // constant spin rate (radians/s), can be negative

    private let screenBounds: CGRect

    /// Spawn a flake with a random size, shape (weighted 40/30/20/10 toward the
    /// simpler shapes), spin, and x-position across a half-width-wider band than
    /// the screen. `stagger: true` scatters the initial fill down the screen;
    /// otherwise it enters just above the top edge.
    init(screenBounds: CGRect, stagger: Bool = false) {
        self.screenBounds = screenBounds
        radius       = CGFloat.random(in: kSnowMinRadius...kSnowMaxRadius)
        rotation     = Double.random(in: 0...(2 * .pi))
        rotationSpeed = Double.random(in: -2.0...2.0)
        vel          = CGPoint(x: 0, y: -CGFloat.random(in: 5...10))

        let r = Int.random(in: 0...99)
        shape = r < 40 ? .simple : r < 70 ? .crystal : r < 90 ? .hexagon : .star

        let xMargin = screenBounds.width * kSnowEdgeMargin
        let x = CGFloat.random(in: (screenBounds.minX - xMargin)...(screenBounds.maxX + xMargin))
        let y = stagger ? CGFloat.random(in: screenBounds.minY...screenBounds.maxY)
                        : screenBounds.maxY + radius
        pos = CGPoint(x: x, y: y)
    }

    /// Integrate one step and report whether the flake settled, went off-screen,
    /// or is still falling. `time` advances the noise field's third axis so the
    /// drift evolves over time; `heightMap` gives the pile height to settle onto.
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

        let margin = screenBounds.width * kSnowEdgeMargin
        if pos.x < screenBounds.minX - margin || pos.x > screenBounds.maxX + margin
            || pos.y < screenBounds.minY - radius {
            return .offScreen
        }

        return .falling
    }
}
