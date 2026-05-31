import CoreGraphics

// MARK: - Physics constants (all per-second; multiplied by delta-time each tick)

let kMaxWindX: CGFloat      = 450   // px/s at direction ±5
let kVelY: CGFloat          = -900  // px/s downward (AppKit y-up)
let kTrailMin: CGFloat      = 30
let kTrailMax: CGFloat      = 100

let kSplatterCount          = 3
let kSplatterSpeed: CGFloat = 210
let kGravity: CGFloat       = 500
let kAirDamp: CGFloat       = 0.55
let kBounceDamp: CGFloat    = 0.55
let kMaxBounces             = 2
let kSplatterDuration       = 1.8   // seconds

// MARK: - Unified floor

/// Single source of truth for the ground level at any x position.
/// Both RainDrop and Splatter query this every tick so physics is always in sync.
func physicsFloor(x: CGFloat, screenBounds: CGRect, dockObstacle: CGRect) -> CGFloat {
    if !dockObstacle.isEmpty && x >= dockObstacle.minX && x <= dockObstacle.maxX {
        return dockObstacle.maxY      // top surface of the Dock icon bar
    }
    return screenBounds.minY          // screen floor
}

/// Maps the direction slider value (-5…+5) to a horizontal wind velocity (px/s).
func windX(forDirection direction: Int) -> CGFloat {
    kMaxWindX * CGFloat(direction) / 5.0
}
