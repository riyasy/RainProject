import CoreGraphics

// MARK: - Physics constants (all per-second; multiplied by delta-time each tick)
//
// Rain tuning knobs. Each note says what raising (↑) / lowering (↓) it does.

let kMaxWindX: CGFloat      = 450   // max horizontal wind (px/s) at direction ±5.
                                    // ↑ stronger slant at the slider extremes; ↓ gentler.
let kVelY: CGFloat          = -900  // fall speed (px/s; negative = down in AppKit's y-up space).
                                    // more negative = faster, straighter rain; less negative = slower.
let kTrailMin: CGFloat      = 30    // shortest drop streak (px).
let kTrailMax: CGFloat      = 100   // longest drop streak (px). ↑ both = longer, comet-like streaks; ↓ stubbier.


let kRainDropMultiplier     = 3    // Falling-drop count per intensity unit. 
                                    // ↑ denser rain per intensity step (more drops, more CPU/GPU); ↓ sparser.

let kSplatterCount          = 3     // droplets thrown up per landing. ↑ bushier splash; ↓ sparser.
let kSplatterSpeed: CGFloat = 210   // splatter launch speed (px/s). ↑ higher & wider splash; ↓ smaller pop.
let kGravity: CGFloat       = 500   // gravity on splatters (px/s²). ↑ lower/snappier arcs; ↓ floatier, taller.
let kAirDamp: CGFloat       = 0.60  // horizontal speed bleed-off rate (per s). ↑ splatters lose sideways drift sooner; ↓ skate farther.
let kBounceDamp: CGFloat    = 0.35  // fraction of vertical speed kept per floor bounce (0–1).
                                    // ↑ toward 1 = bouncier, taller rebounds; ↓ toward 0 = dead, no bounce.
let kMaxBounces             = 2     // bounces before a splatter dies. ↑ keeps bouncing longer; ↓ settles sooner.
let kSplatterDuration       = 1.8   // splash burst lifetime (s) before it fades out. ↑ lingers longer; ↓ vanishes quicker.

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
