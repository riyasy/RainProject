import CoreGraphics

struct RainDrop {
    var pos: CGPoint
    var vel: CGPoint          // var — direction changes are applied live
    let trailLength: CGFloat
    var touchedGround = false
    var isDead = false
    var splatterTime: Double = 0
    var splatters: [Splatter] = []

    private let screenBounds: CGRect

    init(screenBounds: CGRect, windX: CGFloat, stagger: Bool = false) {
        self.screenBounds = screenBounds
        trailLength = .random(in: kTrailMin...kTrailMax)
        vel = CGPoint(x: windX, y: kVelY)
        pos = RainDrop.randomSpawn(in: screenBounds, stagger: stagger)
    }

    /// Upstream end of the visual trail (opposite to velocity direction).
    var trailTail: CGPoint {
        let mag = (vel.x * vel.x + vel.y * vel.y).squareRoot()
        guard mag > 0 else { return pos }
        return CGPoint(x: pos.x - (vel.x / mag) * trailLength,
                       y: pos.y - (vel.y / mag) * trailLength)
    }

    /// Advances the drop's position and checks for ground contact.
    /// Splatters are updated separately by the caller so they share the
    /// same physicsFloor call with current screenBounds/dockObstacle.
    mutating func updateDrop(dt: CGFloat, dockObstacle: CGRect) {
        guard !isDead, !touchedGround else { return }
        pos.x += vel.x * dt
        pos.y += vel.y * dt
        let floor = physicsFloor(x: pos.x, screenBounds: screenBounds, dockObstacle: dockObstacle)
        if pos.y <= floor {
            touchedGround = true
            pos.y = floor
            if pos.x >= screenBounds.minX && pos.x <= screenBounds.maxX {
                spawnSplatters(floorY: floor)
            } else {
                isDead = true
            }
        }
    }

    mutating func reset(windX: CGFloat) {
        touchedGround = false
        isDead = false
        splatterTime = 0
        splatters.removeAll(keepingCapacity: true)
        vel = CGPoint(x: windX, y: kVelY)
        pos = RainDrop.randomSpawn(in: screenBounds, stagger: false)
    }

    // MARK: Private

    private static func randomSpawn(in b: CGRect, stagger: Bool) -> CGPoint {
        let xMargin = b.width / 3
        let x = CGFloat.random(in: (b.minX - xMargin)...(b.maxX + xMargin))
        let y = stagger
            ? CGFloat.random(in: b.minY...(b.maxY + b.height * 0.25))
            : CGFloat.random(in: b.maxY...(b.maxY + b.height * 0.25))
        return CGPoint(x: x, y: y)
    }

    mutating func spawnSplatters(floorY: CGFloat) {
        splatters.reserveCapacity(kSplatterCount)
        for _ in 0..<kSplatterCount {
            let base: CGFloat = Bool.random() ? .random(in: 20...70) : .random(in: 110...160)
            let angle = base * .pi / 180
            let r = CGFloat.random(in: 1.0...2.5)
            splatters.append(Splatter(
                pos: CGPoint(x: pos.x, y: floorY + r),
                vel: CGPoint(x: kSplatterSpeed * cos(angle), y: kSplatterSpeed * sin(angle)),
                radius: r
            ))
        }
    }
}
