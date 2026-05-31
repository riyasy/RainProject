import CoreGraphics

struct Splatter {
    var pos: CGPoint
    var vel: CGPoint    // px/s
    let radius: CGFloat
    var bounceCount = 0

    var isAlive: Bool { bounceCount < kMaxBounces }

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
