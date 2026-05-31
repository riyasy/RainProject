import Cocoa

// Unit paths for each shape type — built once at radius = 1.0.
// Rendered by scaling to flake.radius + rotating + translating per flake.
private enum UnitPath {

    // Simple: slightly oval ellipse (radiusX = 1, radiusY = 0.7)
    static let simple: CGPath = {
        CGPath(ellipseIn: CGRect(x: -1, y: -0.7, width: 2, height: 1.4), transform: nil)
    }()

    // Crystal: 6 arms with 2 branches each at 60% of the arm
    static let crystal: CGPath = {
        let p = CGMutablePath()
        let arm: CGFloat = 2.0, branchLen: CGFloat = 0.8
        let branchOff: CGFloat = 30 * .pi / 180
        for i in 0..<6 {
            let a = CGFloat(i) * (.pi / 3)
            let ex = cos(a) * arm, ey = sin(a) * arm
            p.move(to: .zero);  p.addLine(to: CGPoint(x: ex, y: ey))
            let mx = cos(a) * arm * 0.6, my = sin(a) * arm * 0.6
            let mid = CGPoint(x: mx, y: my)
            for sign: CGFloat in [1, -1] {
                let ba = a + sign * branchOff
                p.move(to: mid)
                p.addLine(to: CGPoint(x: mx + cos(ba) * branchLen, y: my + sin(ba) * branchLen))
            }
        }
        return p
    }()

    // Hexagon: outline + 6 spokes from center
    static let hexagon: CGPath = {
        let p = CGMutablePath()
        let r: CGFloat = 2.0
        let pts = (0...6).map { i -> CGPoint in
            let a = CGFloat(i) * (.pi / 3)
            return CGPoint(x: cos(a) * r, y: sin(a) * r)
        }
        p.move(to: pts[0]); for i in 1...6 { p.addLine(to: pts[i]) }
        for i in 0..<6 { p.move(to: .zero); p.addLine(to: pts[i]) }
        return p
    }()

    // Star: 12 main spikes + short cross lines at alternating spikes
    static let star: CGPath = {
        let p = CGMutablePath()
        let outer: CGFloat = 2.5, inner: CGFloat = 1.0, n = 12
        for i in 0..<n {
            let a = CGFloat(i) * 2 * .pi / CGFloat(n)
            p.move(to: .zero)
            p.addLine(to: CGPoint(x: cos(a) * outer, y: sin(a) * outer))
            if i % 2 == 0 {
                let ca = a + .pi / CGFloat(n)
                p.move(to: .zero)
                p.addLine(to: CGPoint(x: cos(ca) * inner, y: sin(ca) * inner))
            }
        }
        return p
    }()
}

final class SnowRenderer {

    private weak var containerLayer: CALayer?
    private weak var snowLayer: CAShapeLayer?         // settled pile
    private weak var filledFlakeLayer: CAShapeLayer?  // Simple shapes (filled)
    private weak var strokedFlakeLayer: CAShapeLayer? // Crystal/Hexagon/Star (stroked)

    // MARK: Setup / teardown

    func setup(in view: NSView, screenBounds: CGRect) {
        teardown()
        guard let viewLayer = view.layer else { return }

        let scale  = NSScreen.main?.backingScaleFactor ?? 1.0
        let bounds = CGRect(origin: .zero, size: view.bounds.size)

        let container = CALayer()
        container.isOpaque = false
        container.frame = view.bounds
        container.contentsScale = scale
        viewLayer.addSublayer(container)
        containerLayer = container

        func shapeLayer() -> CAShapeLayer {
            let sl = CAShapeLayer()
            sl.contentsScale = scale
            sl.frame = bounds
            sl.actions = ["path": NSNull(), "fillColor": NSNull(), "strokeColor": NSNull()]
            return sl
        }

        // Settled snow pile
        let snow = shapeLayer()
        snow.fillColor   = NSColor.white.cgColor
        snow.strokeColor = nil
        container.addSublayer(snow)
        snowLayer = snow

        // Simple flakes (filled)
        let filled = shapeLayer()
        filled.fillColor   = NSColor.white.withAlphaComponent(0.9).cgColor
        filled.strokeColor = nil
        container.addSublayer(filled)
        filledFlakeLayer = filled

        // Complex flakes (stroked, no fill)
        let stroked = shapeLayer()
        stroked.fillColor   = nil
        stroked.strokeColor = NSColor.white.withAlphaComponent(0.9).cgColor
        stroked.lineWidth   = 0.5
        stroked.lineCap     = .round
        container.addSublayer(stroked)
        strokedFlakeLayer = stroked
    }

    func teardown() {
        containerLayer?.removeFromSuperlayer()
        containerLayer = nil; snowLayer = nil
        filledFlakeLayer = nil; strokedFlakeLayer = nil
    }

    // MARK: Per-frame render

    func render(system: SnowSystem, screenBounds: CGRect, rebuildPile: Bool) {
        let filledPath  = CGMutablePath()
        let strokedPath = CGMutablePath()

        for flake in system.flakes {
            // Transform: scale to flake size → rotate → translate to position.
            // Built directly as a single matrix (uniform scale commutes with the
            // rotation) instead of chaining .rotated(by:).concatenating(...), which
            // allocates two intermediate matrices and does extra multiplies per flake.
            //   a = r·cosθ, b = r·sinθ, c = -r·sinθ, d = r·cosθ
            let cosR = CGFloat(cos(flake.rotation)) * flake.radius
            let sinR = CGFloat(sin(flake.rotation)) * flake.radius
            let xf = CGAffineTransform(a: cosR, b: sinR, c: -sinR, d: cosR,
                                       tx: flake.pos.x, ty: flake.pos.y)

            switch flake.shape {
            case .simple:
                filledPath.addPath(UnitPath.simple, transform: xf)
            case .crystal:
                strokedPath.addPath(UnitPath.crystal, transform: xf)
            case .hexagon:
                strokedPath.addPath(UnitPath.hexagon, transform: xf)
            case .star:
                strokedPath.addPath(UnitPath.star, transform: xf)
            }
        }

        filledFlakeLayer?.path  = filledPath
        strokedFlakeLayer?.path = strokedPath

        // Settled snow pile — rebuilt only when the heightMap changed. While the
        // pile is static the CAShapeLayer keeps its cached tessellation and CA just
        // re-composites it, which avoids re-tessellating a wide polygon every frame.
        if rebuildPile, let sl = snowLayer {
            let hm = system.heightMap
            guard hm.count > 1 else { return }
            let floor = screenBounds.minY
            let step  = screenBounds.width / CGFloat(hm.count - 1)
            let path  = CGMutablePath()
            path.move(to: CGPoint(x: screenBounds.minX, y: floor))
            for (i, h) in hm.enumerated() {
                path.addLine(to: CGPoint(x: screenBounds.minX + CGFloat(i) * step, y: floor + h))
            }
            path.addLine(to: CGPoint(x: screenBounds.maxX, y: floor))
            path.closeSubpath()
            sl.path = path
        }
    }
}
