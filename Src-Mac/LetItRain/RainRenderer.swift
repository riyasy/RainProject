import Metal
import QuartzCore
import Cocoa

// MARK: - Vertex layout (must match RainShaders.metal attribute indices)

/// One interleaved vertex shared by both renderers and every shader. The field
/// order/offsets must stay in lock-step with the `MTLVertexDescriptor` set up in
/// each renderer and the attribute indices in `RainShaders.metal`.
struct RainVertex {
    var x, y: Float     // NDC position
    var u, v: Float     // UV — circle SDF clip (splatter) / atlas sample (snow)
    var r, g, b, a: Float
}

// MARK: - Renderer

/// Metal renderer for rain. Each frame it walks the drop list, writing drop
/// trails as line-quads and splatter bursts as circle-SDF quads into pre-sized,
/// triple-buffered vertex buffers, then issues one indexed draw per primitive
/// type. See `RainShaders.metal` for the matching vertex/fragment functions.
final class RainRenderer {

    private(set) var metalLayer: CAMetalLayer?

    private var device: MTLDevice?
    private var commandQueue: MTLCommandQueue?
    private var dropPipeline: MTLRenderPipelineState?
    private var splatterPipeline: MTLRenderPipelineState?
    private var dropVtxBufs: [MTLBuffer] = []
    private var splatVtxBufs: [MTLBuffer] = []
    private var indexBuf: MTLBuffer?    // pre-built quad→triangle index buffer

    // Triple-buffered vertex data. The CPU writes frame N+1 into a *different*
    // buffer than the GPU is still reading for frame N, so there's no read/write
    // hazard on a shared buffer. The semaphore caps the CPU to kMaxInFlight frames
    // ahead — every wait() is balanced by exactly one signal().
    private static let kMaxInFlight = 2
    private let inFlight = DispatchSemaphore(value: kMaxInFlight)
    private var frameIndex = 0

    private static let kMaxDropVerts  = 500 * 4
    private static let kMaxSplatVerts = 500 * kSplatterCount * 4

    // MARK: Setup

    /// Attach a transparent `CAMetalLayer` to `view` and build the pipelines and
    /// buffers. Safe to call repeatedly; the previous layer is removed first.
    func setup(in view: NSView, screenBounds: CGRect) {
        guard let dev = MetalSystem.device else { return }
        device = dev
        commandQueue = dev.makeCommandQueue()

        let layer = CAMetalLayer()
        layer.device = dev
        layer.pixelFormat = .bgra8Unorm
        layer.isOpaque = false
        layer.framebufferOnly = true
        // Each drawable is a full-screen surface (≈24 MB on Retina), so cap the
        // pool to 2 — double-buffering is plenty for this overlay and saves one
        // screen-sized surface vs. the default of 3. Matches kMaxInFlight.
        layer.maximumDrawableCount = 2
        layer.frame = view.bounds
        let scale = NSScreen.main?.backingScaleFactor ?? 1.0
        layer.contentsScale = scale
        layer.drawableSize = CGSize(width: screenBounds.width  * scale,
                                    height: screenBounds.height * scale)
        metalLayer?.removeFromSuperlayer()
        metalLayer = layer
        view.layer?.addSublayer(layer)

        buildPipelines(device: dev)
        buildBuffers(device: dev)
    }

    /// Detach the Metal layer. The GPU resources are released when the renderer
    /// is next set up or deallocated.
    func teardown() {
        metalLayer?.removeFromSuperlayer()
        metalLayer = nil
    }

    // MARK: Per-frame render

    /// Builds vertex data from the current drop list and issues a single Metal frame.
    func render(drops: [RainDrop], screenBounds: CGRect,
                color: (r: Float, g: Float, b: Float)) {
        guard let layer    = metalLayer,
              let idxBuf   = indexBuf,
              let cmdQueue = commandQueue,
              let dev      = device,
              dropVtxBufs.count  == RainRenderer.kMaxInFlight,
              splatVtxBufs.count == RainRenderer.kMaxInFlight else { return }

        // autoreleasepool bounds the per-frame transient objects (the
        // CAMetalDrawable and its screen-sized texture, command buffer, etc.)
        // so they don't accumulate between run-loop drains.
        autoreleasepool {
            // Claim a free in-flight slot, then advance to its buffer ring index.
            // From here on, every exit path must signal() exactly once (the command
            // buffer's completion handler, or the early-outs in encode()).
            inFlight.wait()
            frameIndex = (frameIndex + 1) % RainRenderer.kMaxInFlight
            let dropBuf  = dropVtxBufs[frameIndex]
            let splatBuf = splatVtxBufs[frameIndex]

            let sw = Float(screenBounds.width), sh = Float(screenBounds.height)

            let dPtr = dropBuf.contents().assumingMemoryBound(to: RainVertex.self)
            let sPtr = splatBuf.contents().assumingMemoryBound(to: RainVertex.self)
            var dCnt = 0, sCnt = 0
            let (cr, cg, cb) = color

            for drop in drops {
                if !drop.touchedGround {
                    buildDropQuad(drop: drop, screenBounds: screenBounds,
                                  sw: sw, sh: sh, r: cr, g: cg, b: cb,
                                  into: dPtr, count: &dCnt)
                } else {
                    buildSplatterQuads(drop: drop, sw: sw, sh: sh,
                                       r: cr, g: cg, b: cb,
                                       into: sPtr, count: &sCnt)
                }
            }

            if !dev.hasUnifiedMemory {
                if dCnt > 0 { dropBuf.didModifyRange(0..<dCnt  * MemoryLayout<RainVertex>.stride) }
                if sCnt > 0 { splatBuf.didModifyRange(0..<sCnt * MemoryLayout<RainVertex>.stride) }
            }

            encode(drawable: layer.nextDrawable(),
                   cmdQueue: cmdQueue,
                   dropBuf: dropBuf, dCnt: dCnt,
                   splatBuf: splatBuf, sCnt: sCnt,
                   idxBuf: idxBuf)
        }
    }

    // MARK: Private — setup helpers

    /// Build the two render pipelines (drop trails, splatter dots). Both share the
    /// `RainVertex` layout and premultiplied-style source-over alpha blending, and
    /// differ only in fragment function.
    private func buildPipelines(device dev: MTLDevice) {
        guard let lib   = dev.makeDefaultLibrary(),
              let vert  = lib.makeFunction(name: "mainVertex"),
              let dropF = lib.makeFunction(name: "dropFragment"),
              let splatF = lib.makeFunction(name: "splatterFragment") else { return }

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
        dropPipeline     = pipe(dropF)
        splatterPipeline = pipe(splatF)
    }

    /// Allocate the triple-buffered vertex rings (sized to the max particle
    /// count) and the shared quad→triangle index buffer.
    private func buildBuffers(device dev: MTLDevice) {
        let opts: MTLResourceOptions = dev.hasUnifiedMemory ? .storageModeShared : .storageModeManaged
        let stride = MemoryLayout<RainVertex>.stride
        frameIndex = 0
        dropVtxBufs = (0..<RainRenderer.kMaxInFlight).compactMap { _ in
            dev.makeBuffer(length: RainRenderer.kMaxDropVerts  * stride, options: opts)
        }
        splatVtxBufs = (0..<RainRenderer.kMaxInFlight).compactMap { _ in
            dev.makeBuffer(length: RainRenderer.kMaxSplatVerts * stride, options: opts)
        }

        // Each quad (4 verts) → 2 triangles (6 indices): [0,1,2, 1,3,2]
        let maxQuads = max(RainRenderer.kMaxDropVerts, RainRenderer.kMaxSplatVerts) / 4
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

    // MARK: Private — vertex building

    /// Append one drop's trail as a 4-vertex quad (head `pos` → `trailTail`),
    /// given a constant half-width perpendicular to the velocity. Skips the drop
    /// when fully off-screen or when the buffer is full.
    private func buildDropQuad(drop: RainDrop, screenBounds: CGRect,
                                sw: Float, sh: Float,
                                r: Float, g: Float, b: Float,
                                into ptr: UnsafeMutablePointer<RainVertex>,
                                count: inout Int) {
        let h = drop.pos, t = drop.trailTail
        guard screenBounds.contains(h) || screenBounds.contains(t) else { return }
        guard count + 4 <= RainRenderer.kMaxDropVerts else { return }

        let v = drop.vel
        let mag = (v.x * v.x + v.y * v.y).squareRoot()
        let hw = CGFloat(1.5)
        // (px, py): unit normal to the velocity, scaled to half-width and into NDC,
        // so the two long edges of the trail quad sit either side of the streak.
        let px = Float(-v.y / mag * hw / screenBounds.width)
        let py = Float( v.x / mag * hw / screenBounds.height)
        let hx = Float(h.x) / sw * 2 - 1, hy = Float(h.y) / sh * 2 - 1
        let tx = Float(t.x) / sw * 2 - 1, ty = Float(t.y) / sh * 2 - 1

        ptr[count]   = RainVertex(x: tx-px, y: ty-py, u: 0, v: 0, r: r, g: g, b: b, a: 0.75)
        ptr[count+1] = RainVertex(x: tx+px, y: ty+py, u: 0, v: 0, r: r, g: g, b: b, a: 0.75)
        ptr[count+2] = RainVertex(x: hx-px, y: hy-py, u: 0, v: 0, r: r, g: g, b: b, a: 0.75)
        ptr[count+3] = RainVertex(x: hx+px, y: hy+py, u: 0, v: 0, r: r, g: g, b: b, a: 0.75)
        count += 4
    }

    /// Append a quad per live splatter droplet of a landed drop. The UVs span
    /// `[-1, 1]` so `splatterFragment` can clip each quad to a circle, and the
    /// whole burst fades out over `kSplatterDuration`.
    private func buildSplatterQuads(drop: RainDrop,
                                     sw: Float, sh: Float,
                                     r: Float, g: Float, b: Float,
                                     into ptr: UnsafeMutablePointer<RainVertex>,
                                     count: inout Int) {
        let alpha = Float(max(0, 1 - drop.splatterTime / kSplatterDuration)) * 0.7
        for sp in drop.splatters where sp.isAlive {
            guard count + 4 <= RainRenderer.kMaxSplatVerts else { break }
            let cx = Float(sp.pos.x) / sw * 2 - 1, cy = Float(sp.pos.y) / sh * 2 - 1
            let rx = Float(sp.radius) / sw * 2
            let ry = Float(sp.radius) / sh * 2
            ptr[count]   = RainVertex(x: cx-rx, y: cy-ry, u: -1, v: -1, r: r, g: g, b: b, a: alpha)
            ptr[count+1] = RainVertex(x: cx+rx, y: cy-ry, u:  1, v: -1, r: r, g: g, b: b, a: alpha)
            ptr[count+2] = RainVertex(x: cx-rx, y: cy+ry, u: -1, v:  1, r: r, g: g, b: b, a: alpha)
            ptr[count+3] = RainVertex(x: cx+rx, y: cy+ry, u:  1, v:  1, r: r, g: g, b: b, a: alpha)
            count += 4
        }
    }

    // MARK: Private — Metal encode + present

    /// Encode and present one frame: clear to transparent, draw the drop quads
    /// then the splatter quads, and signal the in-flight semaphore on completion.
    /// Any early-out here must `signal()` so the `wait()` in `render` stays balanced.
    private func encode(drawable: (any CAMetalDrawable)?,
                        cmdQueue: MTLCommandQueue,
                        dropBuf: MTLBuffer, dCnt: Int,
                        splatBuf: MTLBuffer, sCnt: Int,
                        idxBuf: MTLBuffer) {
        guard let drawable,
              let cmdBuf    = cmdQueue.makeCommandBuffer(),
              let dropPipe  = dropPipeline,
              let splatPipe = splatterPipeline else { inFlight.signal(); return }

        let pass = MTLRenderPassDescriptor()
        pass.colorAttachments[0].texture     = drawable.texture
        pass.colorAttachments[0].loadAction  = .clear
        pass.colorAttachments[0].clearColor  = MTLClearColorMake(0, 0, 0, 0)
        pass.colorAttachments[0].storeAction = .store

        guard let enc = cmdBuf.makeRenderCommandEncoder(descriptor: pass) else { inFlight.signal(); return }

        if dCnt > 0 {
            enc.setRenderPipelineState(dropPipe)
            enc.setVertexBuffer(dropBuf, offset: 0, index: 0)
            enc.drawIndexedPrimitives(type: .triangle, indexCount: (dCnt / 4) * 6,
                                      indexType: .uint16, indexBuffer: idxBuf,
                                      indexBufferOffset: 0)
        }
        if sCnt > 0 {
            enc.setRenderPipelineState(splatPipe)
            enc.setVertexBuffer(splatBuf, offset: 0, index: 0)
            enc.drawIndexedPrimitives(type: .triangle, indexCount: (sCnt / 4) * 6,
                                      indexType: .uint16, indexBuffer: idxBuf,
                                      indexBufferOffset: 0)
        }

        enc.endEncoding()
        cmdBuf.addCompletedHandler { [inFlight] _ in inFlight.signal() }
        cmdBuf.present(drawable)
        cmdBuf.commit()
    }
}
