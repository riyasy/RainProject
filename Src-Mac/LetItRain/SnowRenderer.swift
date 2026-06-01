import Metal
import QuartzCore
import Cocoa

/// Unit `CGPath`s for the four flake shapes, each built once at its natural
/// extent (centered on the origin). These are baked into the atlas texture by
/// `SnowRenderer.buildAtlas` rather than re-tessellated per frame.
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

// MARK: - Metal snow renderer
//
// Each of the four flake shapes is rendered once (antialiased, via Core
// Graphics) into a 2×2 texture atlas at setup. Per frame we emit one rotated,
// scaled quad per flake (sampling its shape's atlas cell) plus a triangle-strip
// for the settled pile — written straight into Metal vertex buffers, with no
// per-frame CGPath construction or CAShapeLayer re-tessellation.
final class SnowRenderer {

    private(set) var metalLayer: CAMetalLayer?

    private var device: MTLDevice?
    private var commandQueue: MTLCommandQueue?
    private var flakePipeline: MTLRenderPipelineState?
    private var pilePipeline: MTLRenderPipelineState?
    private var atlasTexture: MTLTexture?
    private var sampler: MTLSamplerState?
    private var indexBuf: MTLBuffer?    // pre-built quad→triangle index buffer

    private var flakeVtxBufs: [MTLBuffer] = []
    private var pileVtxBufs: [MTLBuffer] = []

    // Triple-buffered vertex data + semaphore — same hazard avoidance as the
    // rain renderer: the CPU writes the next frame into a different buffer than
    // the GPU is still reading.
    private static let kMaxInFlight = 3
    private let inFlight = DispatchSemaphore(value: kMaxInFlight)
    private var frameIndex = 0

    // Geometry / look constants
    private static let kExtent: Float    = 2.5     // unit-shape half-extent baked into each atlas cell
    private static let kSnowAlpha: Float = 0.9     // flake opacity (matches the old CAShapeLayer alpha)
    private static let kPileAlpha: Float = 1.0     // settled pile is opaque white
    private static let kAtlasCell = 128            // px per shape cell
    private static let kAtlasSize = 256            // 2×2 atlas

    private static let kMaxFlakes     = 500
    private static let kMaxFlakeVerts = kMaxFlakes * 4
    private static let kMaxPileVerts  = 400        // ≥ 2 × max pile columns

    // MARK: Setup / teardown

    /// Attach a transparent `CAMetalLayer`, build the pipelines/sampler, bake the
    /// shape atlas, and allocate the vertex rings. Safe to call repeatedly.
    func setup(in view: NSView, screenBounds: CGRect) {
        teardown()
        guard let dev = MetalSystem.device,
              let viewLayer = view.layer else { return }
        device = dev
        commandQueue = dev.makeCommandQueue()

        let layer = CAMetalLayer()
        layer.device = dev
        layer.pixelFormat = .bgra8Unorm
        layer.isOpaque = false
        layer.framebufferOnly = true
        layer.frame = view.bounds
        let scale = NSScreen.main?.backingScaleFactor ?? 1.0
        layer.contentsScale = scale
        layer.drawableSize = CGSize(width: screenBounds.width  * scale,
                                    height: screenBounds.height * scale)
        viewLayer.addSublayer(layer)
        metalLayer = layer

        buildPipelines(device: dev)
        buildSampler(device: dev)
        buildAtlas(device: dev)
        buildBuffers(device: dev)
    }

    /// Detach the Metal layer and drop all GPU resources.
    func teardown() {
        metalLayer?.removeFromSuperlayer()
        metalLayer = nil
        flakePipeline = nil; pilePipeline = nil
        atlasTexture = nil; sampler = nil
        flakeVtxBufs.removeAll(); pileVtxBufs.removeAll()
    }

    // MARK: Per-frame render

    /// Emit one textured quad per flake plus the pile triangle-strip into the
    /// current ring buffers, then encode the frame. Triple-buffered behind the
    /// in-flight semaphore exactly like the rain renderer.
    func render(system: SnowSystem, screenBounds: CGRect) {
        guard let layer    = metalLayer,
              let idxBuf   = indexBuf,
              let cmdQueue = commandQueue,
              let dev      = device,
              let atlas    = atlasTexture,
              let smp      = sampler,
              flakeVtxBufs.count == SnowRenderer.kMaxInFlight,
              pileVtxBufs.count  == SnowRenderer.kMaxInFlight else { return }

        // autoreleasepool bounds the per-frame transient objects (the
        // CAMetalDrawable and its screen-sized texture, command buffer, etc.).
        autoreleasepool {
            inFlight.wait()
            frameIndex = (frameIndex + 1) % SnowRenderer.kMaxInFlight
            let flakeBuf = flakeVtxBufs[frameIndex]
            let pileBuf  = pileVtxBufs[frameIndex]

            let sw = Float(screenBounds.width), sh = Float(screenBounds.height)
            let stride = MemoryLayout<RainVertex>.stride

            // ── Flake quads ─────────────────────────────────────────────────
            let fPtr = flakeBuf.contents().assumingMemoryBound(to: RainVertex.self)
            var fCnt = 0
            for flake in system.flakes {
                guard fCnt + 4 <= SnowRenderer.kMaxFlakeVerts else { break }
                buildFlakeQuad(flake, sw: sw, sh: sh, into: fPtr, count: &fCnt)
            }

            // ── Settled pile triangle strip ─────────────────────────────────
            let pPtr = pileBuf.contents().assumingMemoryBound(to: RainVertex.self)
            var pCnt = 0
            buildPile(system.heightMap, screenBounds: screenBounds,
                      sw: sw, sh: sh, into: pPtr, count: &pCnt)

            if !dev.hasUnifiedMemory {
                if fCnt > 0 { flakeBuf.didModifyRange(0..<fCnt * stride) }
                if pCnt > 0 { pileBuf.didModifyRange(0..<pCnt * stride) }
            }

            encode(drawable: layer.nextDrawable(), cmdQueue: cmdQueue,
                   pileBuf: pileBuf, pCnt: pCnt,
                   flakeBuf: flakeBuf, fCnt: fCnt,
                   idxBuf: idxBuf, atlas: atlas, sampler: smp)
        }
    }

    // MARK: Private — vertex building

    /// Append one flake as a rotated, radius-scaled quad whose UVs address the
    /// flake's cell in the atlas. Folds the half-size into the rotation so each
    /// corner is `center + R(θ)·(±hs, ±hs)`.
    private func buildFlakeQuad(_ f: SnowFlake, sw: Float, sh: Float,
                                into ptr: UnsafeMutablePointer<RainVertex>,
                                count: inout Int) {
        let hs = Float(f.radius) * SnowRenderer.kExtent          // half-size (px)
        let cr = Float(cos(f.rotation)) * hs
        let sr = Float(sin(f.rotation)) * hs
        let cx = Float(f.pos.x), cy = Float(f.pos.y)
        let (u0, u1, v0, v1) = SnowRenderer.uvRect(for: f.shape)
        let a = SnowRenderer.kSnowAlpha

        // Local quad corners (±1, ±1) scaled by hs and rotated by f.rotation.
        //   off = (lx·cr − ly·sr, lx·sr + ly·cr)
        @inline(__always) func emit(_ lx: Float, _ ly: Float, _ u: Float, _ v: Float) {
            let wx = cx + (lx * cr - ly * sr)
            let wy = cy + (lx * sr + ly * cr)
            ptr[count] = RainVertex(x: wx / sw * 2 - 1, y: wy / sh * 2 - 1,
                                    u: u, v: v, r: 1, g: 1, b: 1, a: a)
            count += 1
        }
        emit(-1, -1, u0, v0)   // 0
        emit( 1, -1, u1, v0)   // 1
        emit(-1,  1, u0, v1)   // 2
        emit( 1,  1, u1, v1)   // 3
    }

    /// Append the settled-pile polygon as a triangle strip: a (floor, top) vertex
    /// pair per `heightMap` column, so adjacent columns form the filled slope.
    private func buildPile(_ hm: [CGFloat], screenBounds: CGRect,
                           sw: Float, sh: Float,
                           into ptr: UnsafeMutablePointer<RainVertex>,
                           count: inout Int) {
        guard hm.count > 1 else { return }
        let floor = Float(screenBounds.minY)
        let minX  = Float(screenBounds.minX)
        let step  = sw / Float(hm.count - 1)
        let a = SnowRenderer.kPileAlpha

        // Triangle strip: (bottom, top) vertex pair per column.
        for (i, h) in hm.enumerated() {
            guard count + 2 <= SnowRenderer.kMaxPileVerts else { break }
            let x = minX + Float(i) * step
            let top = floor + Float(h)
            ptr[count]   = RainVertex(x: x / sw * 2 - 1, y: floor / sh * 2 - 1,
                                      u: 0, v: 0, r: 1, g: 1, b: 1, a: a)
            ptr[count+1] = RainVertex(x: x / sw * 2 - 1, y: top   / sh * 2 - 1,
                                      u: 0, v: 0, r: 1, g: 1, b: 1, a: a)
            count += 2
        }
    }

    /// Atlas cell (u0, u1, v0, v1) for each shape. 2×2 layout, v=0 at top:
    ///   simple ┃ crystal      (top row)
    ///   hexagon ┃ star        (bottom row)
    private static func uvRect(for shape: SnowflakeShape) -> (Float, Float, Float, Float) {
        switch shape {
        case .simple:  return (0.0, 0.5, 0.0, 0.5)
        case .crystal: return (0.5, 1.0, 0.0, 0.5)
        case .hexagon: return (0.0, 0.5, 0.5, 1.0)
        case .star:    return (0.5, 1.0, 0.5, 1.0)
        }
    }

    // MARK: Private — setup helpers

    /// Build the two pipelines: textured flakes (`snowFlakeFragment`) and the
    /// flat-white pile (reusing `dropFragment`). Both share `mainVertex` and the
    /// `RainVertex` layout, with source-over alpha blending.
    private func buildPipelines(device dev: MTLDevice) {
        guard let lib    = dev.makeDefaultLibrary(),
              let vert   = lib.makeFunction(name: "mainVertex"),
              let flakeF = lib.makeFunction(name: "snowFlakeFragment"),
              let pileF  = lib.makeFunction(name: "dropFragment") else { return }

        let vd = MTLVertexDescriptor()
        vd.attributes[0].format = .float2; vd.attributes[0].offset = 0;  vd.attributes[0].bufferIndex = 0
        vd.attributes[1].format = .float2; vd.attributes[1].offset = 8;  vd.attributes[1].bufferIndex = 0
        vd.attributes[2].format = .float4; vd.attributes[2].offset = 16; vd.attributes[2].bufferIndex = 0
        vd.layouts[0].stride = MemoryLayout<RainVertex>.stride

        func pipe(_ frag: MTLFunction) -> MTLRenderPipelineState? {
            let d = MTLRenderPipelineDescriptor()
            d.vertexFunction = vert; d.fragmentFunction = frag; d.vertexDescriptor = vd
            let ca = d.colorAttachments[0]!
            ca.pixelFormat = .bgra8Unorm; ca.isBlendingEnabled = true
            ca.sourceRGBBlendFactor        = .sourceAlpha
            ca.destinationRGBBlendFactor   = .oneMinusSourceAlpha
            ca.sourceAlphaBlendFactor      = .one
            ca.destinationAlphaBlendFactor = .oneMinusSourceAlpha
            return try? dev.makeRenderPipelineState(descriptor: d)
        }
        flakePipeline = pipe(flakeF)
        pilePipeline  = pipe(pileF)
    }

    /// Linear + mipmapped, clamp-to-edge sampler — antialiases the baked shapes
    /// as the tiny on-screen quads sample down the atlas.
    private func buildSampler(device dev: MTLDevice) {
        let d = MTLSamplerDescriptor()
        d.minFilter = .linear
        d.magFilter = .linear
        d.mipFilter = .linear
        d.sAddressMode = .clampToEdge
        d.tAddressMode = .clampToEdge
        sampler = dev.makeSamplerState(descriptor: d)
    }

    /// Bake the four unit shapes into a 2×2 RGBA atlas (white shape, antialiased
    /// coverage in alpha) using the same Core Graphics drawing as before.
    private func buildAtlas(device dev: MTLDevice) {
        let size = SnowRenderer.kAtlasSize, cell = SnowRenderer.kAtlasCell
        let bytesPerRow = size * 4
        guard let cs = CGColorSpace(name: CGColorSpace.sRGB),
              let ctx = CGContext(data: nil, width: size, height: size,
                                  bitsPerComponent: 8, bytesPerRow: bytesPerRow,
                                  space: cs,
                                  bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue) else { return }

        // Flip to a top-left origin so CG pixel rows match Metal texel rows.
        ctx.translateBy(x: 0, y: CGFloat(size))
        ctx.scaleBy(x: 1, y: -1)
        ctx.setShouldAntialias(true)
        ctx.setLineCap(.round)
        ctx.setLineJoin(.round)
        ctx.setStrokeColor(NSColor.white.cgColor)
        ctx.setFillColor(NSColor.white.cgColor)

        let pxPerUnit = CGFloat(cell) / 2 / CGFloat(SnowRenderer.kExtent)  // unit → px in a cell

        // Draw a unit path centered in its cell. lineWidth is in unit coords
        // (after scaleBy), so on-screen stroke ≈ 0.3·radius pt → ~0.5 pt at the
        // typical flake size, matching the old constant-width stroke closely.
        func draw(_ path: CGPath, col: Int, row: Int, fill: Bool) {
            ctx.saveGState()
            ctx.translateBy(x: CGFloat(col * cell) + CGFloat(cell) / 2,
                            y: CGFloat(row * cell) + CGFloat(cell) / 2)
            ctx.scaleBy(x: pxPerUnit, y: pxPerUnit)
            ctx.setLineWidth(0.3)
            ctx.addPath(path)
            if fill { ctx.fillPath() } else { ctx.strokePath() }
            ctx.restoreGState()
        }
        draw(UnitPath.simple,  col: 0, row: 0, fill: true)    // top-left
        draw(UnitPath.crystal, col: 1, row: 0, fill: false)   // top-right
        draw(UnitPath.hexagon, col: 0, row: 1, fill: false)   // bottom-left
        draw(UnitPath.star,    col: 1, row: 1, fill: false)   // bottom-right

        guard let data = ctx.data else { return }
        let desc = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: .rgba8Unorm, width: size, height: size, mipmapped: true)
        desc.usage = [.shaderRead]
        desc.storageMode = dev.hasUnifiedMemory ? .shared : .managed
        guard let tex = dev.makeTexture(descriptor: desc) else { return }
        tex.replace(region: MTLRegionMake2D(0, 0, size, size),
                    mipmapLevel: 0, withBytes: data, bytesPerRow: bytesPerRow)

        if let cmd = commandQueue?.makeCommandBuffer(),
           let blit = cmd.makeBlitCommandEncoder() {
            blit.generateMipmaps(for: tex)
            blit.endEncoding()
            cmd.commit()
            cmd.waitUntilCompleted()
        }
        atlasTexture = tex
    }

    /// Allocate the triple-buffered flake and pile vertex rings and the shared
    /// quad→triangle index buffer (used only by the flake draw).
    private func buildBuffers(device dev: MTLDevice) {
        let opts: MTLResourceOptions = dev.hasUnifiedMemory ? .storageModeShared : .storageModeManaged
        let stride = MemoryLayout<RainVertex>.stride
        frameIndex = 0
        flakeVtxBufs = (0..<SnowRenderer.kMaxInFlight).compactMap { _ in
            dev.makeBuffer(length: SnowRenderer.kMaxFlakeVerts * stride, options: opts)
        }
        pileVtxBufs = (0..<SnowRenderer.kMaxInFlight).compactMap { _ in
            dev.makeBuffer(length: SnowRenderer.kMaxPileVerts * stride, options: opts)
        }

        // Each quad (4 verts) → 2 triangles (6 indices): [0,1,2, 1,3,2]
        let maxQuads = SnowRenderer.kMaxFlakeVerts / 4
        var idx = [UInt16](repeating: 0, count: maxQuads * 6)
        for q in 0..<maxQuads {
            let b = UInt16(q * 4)
            idx[q*6+0] = b;   idx[q*6+1] = b+1; idx[q*6+2] = b+2
            idx[q*6+3] = b+1; idx[q*6+4] = b+3; idx[q*6+5] = b+2
        }
        indexBuf = dev.makeBuffer(bytes: idx,
                                   length: idx.count * MemoryLayout<UInt16>.size,
                                   options: .storageModeShared)
    }

    // MARK: Private — Metal encode + present

    /// Encode and present one frame: clear to transparent, draw the pile strip
    /// (behind), then the textured flake quads (in front), and signal the
    /// in-flight semaphore on completion. Early-outs must `signal()` to stay balanced.
    private func encode(drawable: (any CAMetalDrawable)?,
                        cmdQueue: MTLCommandQueue,
                        pileBuf: MTLBuffer, pCnt: Int,
                        flakeBuf: MTLBuffer, fCnt: Int,
                        idxBuf: MTLBuffer,
                        atlas: MTLTexture, sampler: MTLSamplerState) {
        guard let drawable,
              let cmdBuf    = cmdQueue.makeCommandBuffer(),
              let flakePipe = flakePipeline,
              let pilePipe  = pilePipeline else { inFlight.signal(); return }

        let pass = MTLRenderPassDescriptor()
        pass.colorAttachments[0].texture     = drawable.texture
        pass.colorAttachments[0].loadAction  = .clear
        pass.colorAttachments[0].clearColor  = MTLClearColorMake(0, 0, 0, 0)
        pass.colorAttachments[0].storeAction = .store

        guard let enc = cmdBuf.makeRenderCommandEncoder(descriptor: pass) else { inFlight.signal(); return }

        // Settled pile first (behind the flakes).
        if pCnt > 0 {
            enc.setRenderPipelineState(pilePipe)
            enc.setVertexBuffer(pileBuf, offset: 0, index: 0)
            enc.drawPrimitives(type: .triangleStrip, vertexStart: 0, vertexCount: pCnt)
        }
        // Falling flakes on top.
        if fCnt > 0 {
            enc.setRenderPipelineState(flakePipe)
            enc.setVertexBuffer(flakeBuf, offset: 0, index: 0)
            enc.setFragmentTexture(atlas, index: 0)
            enc.setFragmentSamplerState(sampler, index: 0)
            enc.drawIndexedPrimitives(type: .triangle, indexCount: (fCnt / 4) * 6,
                                      indexType: .uint16, indexBuffer: idxBuf,
                                      indexBufferOffset: 0)
        }

        enc.endEncoding()
        cmdBuf.addCompletedHandler { [inFlight] _ in inFlight.signal() }
        cmdBuf.present(drawable)
        cmdBuf.commit()
    }
}
