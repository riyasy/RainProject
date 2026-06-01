import CoreGraphics

/// A single droplet thrown off where a rain drop lands. It's a tiny ballistic
/// body — gravity pulls it down, air drag bleeds off horizontal speed, and it
/// loses energy on each floor bounce — that dies after `kMaxBounces`.
struct Splatter {
    var pos: CGPoint
    var vel: CGPoint    // px/s
    let radius: CGFloat
    var bounceCount = 0

    /// Still drawn/updated until it has bounced `kMaxBounces` times.
    var isAlive: Bool { bounceCount < kMaxBounces }

    /// Integrate one step: advance, apply gravity + horizontal air damping, and
    /// reflect off the shared `physicsFloor` (losing `kBounceDamp` of its speed).
    mutating func update(dt: CGFloat, screenBounds: CGRect, dockObstacle: CGRect) {
        pos.x += vel.x * dt
        pos.y += vel.y * dt
        vel.y -= kGravity * dt
        vel.x *= (1.0 - kAirDamp * dt)

        let floor = physicsFloor(x: pos.x, screenBounds: screenBounds, dockObstacle: dockObstacle)
        if pos.y - radius < floor {
            pos.y = floor + radius
            vel.y = -vel.y * kBounceDamp
            bounceCount += 1
        }
    }
}
