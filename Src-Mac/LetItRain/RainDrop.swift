import CoreGraphics

/// One falling rain drop and the lifecycle it owns: it falls, lands on the
/// `physicsFloor`, spawns a short-lived splatter burst, then recycles itself.
/// A `RainDrop` is reused for the life of the app — `reset(windX:)` respawns it
/// rather than allocating a new one — so the per-frame drop array stays stable.
struct RainDrop {
    var pos: CGPoint
    var vel: CGPoint          // var — direction changes are applied live
    let trailLength: CGFloat  // length of the visual streak behind `pos`
    var touchedGround = false // true once landed; then the splatter phase runs
    var isDead = false        // flagged for recycling at the end of the tick
    var splatterTime: Double = 0   // seconds since landing (drives the burst fade)
    var splatters: [Splatter] = []

    private let screenBounds: CGRect

    /// Spawn a drop falling at `kVelY` with horizontal `windX`. Pass
    /// `stagger: true` for the initial fill so drops start scattered down the
    /// screen instead of all entering from the top edge at once.
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

    /// Recycle a dead drop back to a fresh fall. Keeps the splatter array's
    /// capacity so respawning never reallocates.
    mutating func reset(windX: CGFloat) {
        touchedGround = false
        isDead = false
        splatterTime = 0
        splatters.removeAll(keepingCapacity: true)
        vel = CGPoint(x: windX, y: kVelY)
        pos = RainDrop.randomSpawn(in: screenBounds, stagger: false)
    }

    // MARK: Private

    /// Random spawn point. X is drawn across the screen plus a third-width margin
    /// on each side (so wind-blown drops can enter from off-screen); Y starts just
    /// above the top edge, or anywhere down the screen when `stagger` is set.
    private static func randomSpawn(in b: CGRect, stagger: Bool) -> CGPoint {
        let xMargin = b.width / 3
        let x = CGFloat.random(in: (b.minX - xMargin)...(b.maxX + xMargin))
        let y = stagger
            ? CGFloat.random(in: b.minY...(b.maxY + b.height * 0.25))
            : CGFloat.random(in: b.maxY...(b.maxY + b.height * 0.25))
        return CGPoint(x: x, y: y)
    }

    /// Emit the landing burst: `kSplatterCount` droplets launched up-and-outward
    /// from the impact point. Each angle is biased left (20–70°) or right
    /// (110–160°) of vertical so the burst fans to the sides rather than straight up.
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
