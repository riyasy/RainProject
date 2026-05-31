import Cocoa

// CAShapeLayer-based renderer.
//
// Architecture: one CAShapeLayer for rain trails (all lines batched into one
// CGMutablePath) + kBuckets CAShapeLayers for splatters (grouped by fade alpha
// so we avoid per-drop layer allocation). Both are GPU-composited by Core
// Animation — equivalent to Direct2D's batched DrawLine/FillEllipse approach.

final class CGRenderer {

    private weak var containerLayer: CALayer?
    private weak var dropLayer: CAShapeLayer?
    private var splatLayers: [CAShapeLayer] = []

    // Splatter fade is quantised into this many alpha bands.
    // Bucket i covers splatterTime in [i/kBuckets … (i+1)/kBuckets) × kSplatterDuration.
    private static let kBuckets = 6

    // MARK: Setup

    func setup(in view: NSView, screenBounds: CGRect) {
        containerLayer?.removeFromSuperlayer()
        dropLayer = nil
        splatLayers.removeAll()

        guard let viewLayer = view.layer else { return }
        let scale  = NSScreen.main?.backingScaleFactor ?? 1.0
        let bounds = CGRect(origin: .zero, size: view.bounds.size)

        let container = CALayer()
        container.isOpaque = false
        container.contentsScale = scale
        container.frame = view.bounds
        viewLayer.addSublayer(container)
        containerLayer = container

        // One layer for all falling-drop trails.
        let dl = CAShapeLayer()
        dl.fillColor   = nil
        dl.strokeColor = CGColor(red: 0.6, green: 0.82, blue: 1.0, alpha: 0.75)
        dl.lineWidth   = 2.0
        dl.lineCap     = .butt
        dl.contentsScale = scale
        dl.frame  = bounds
        dl.actions = ["path": NSNull(), "strokeColor": NSNull()]
        container.addSublayer(dl)
        dropLayer = dl

        // kBuckets layers for splatters — one alpha band each.
        for i in 0..<CGRenderer.kBuckets {
            let alpha = alphaForBucket(i)
            let sl = CAShapeLayer()
            sl.fillColor   = CGColor(red: 0.6, green: 0.82, blue: 1.0, alpha: alpha)
            sl.strokeColor = nil
            sl.contentsScale = scale
            sl.frame  = bounds
            sl.actions = ["path": NSNull(), "fillColor": NSNull()]
            container.addSublayer(sl)
            splatLayers.append(sl)
        }
    }

    // MARK: Per-frame render

    func render(drops: [RainDrop], screenBounds: CGRect,
                color: (r: Float, g: Float, b: Float)) {
        let cr = CGFloat(color.r), cg = CGFloat(color.g), cb = CGFloat(color.b)

        // ── Falling-drop trails — one CGMutablePath, one GPU draw ──────────
        if let dl = dropLayer {
            let path = CGMutablePath()
            for drop in drops where !drop.touchedGround {
                let h = drop.pos, t = drop.trailTail
                guard screenBounds.contains(h) || screenBounds.contains(t) else { continue }
                path.move(to: t)
                path.addLine(to: h)
            }
            dl.path        = path
            dl.strokeColor = CGColor(red: cr, green: cg, blue: cb, alpha: 0.75)
        }

        // ── Splatters — one CGMutablePath per alpha bucket ──────────────────
        let bucketPaths = (0..<CGRenderer.kBuckets).map { _ in CGMutablePath() }
        for drop in drops where drop.touchedGround {
            let t   = drop.splatterTime / kSplatterDuration
            let idx = min(Int(t * Double(CGRenderer.kBuckets)), CGRenderer.kBuckets - 1)
            for sp in drop.splatters where sp.isAlive {
                bucketPaths[idx].addEllipse(in: CGRect(x: sp.pos.x - sp.radius,
                                                       y: sp.pos.y - sp.radius,
                                                       width:  sp.radius * 2,
                                                       height: sp.radius * 2))
            }
        }
        for (i, sl) in splatLayers.enumerated() {
            sl.path      = bucketPaths[i]
            sl.fillColor = CGColor(red: cr, green: cg, blue: cb, alpha: alphaForBucket(i))
        }
    }

    // MARK: Private

    private func alphaForBucket(_ i: Int) -> CGFloat {
        CGFloat(CGRenderer.kBuckets - i) / CGFloat(CGRenderer.kBuckets) * 0.7
    }
}
