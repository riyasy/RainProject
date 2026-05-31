import Metal

/// Process-wide Metal device. A process should use a single `MTLDevice`, and
/// acquiring the system default device isn't free, so both renderers share this
/// rather than re-creating it on every `setup(...)`.
enum MetalSystem {
    static let device: MTLDevice? = MTLCreateSystemDefaultDevice()
}
