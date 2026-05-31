import Foundation

// Ken Perlin's improved 3D gradient noise — the Mac equivalent of the Windows
// build's FastNoiseLite. Output is roughly in [-1, 1]. Deterministic (uses the
// reference permutation table) so the snow field looks identical run-to-run.
enum SnowNoise {

    private static let p: [Int] = {
        let source = [151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,
            103,30,69,142,8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,
            94,252,219,203,117,35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,
            168,68,175,74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,60,
            211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,65,25,63,161,1,216,
            80,73,209,76,132,187,208,89,18,169,200,196,135,130,116,188,159,86,164,100,
            109,198,173,186,3,64,52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,
            212,207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,119,248,152,2,
            44,154,163,70,221,153,101,155,167,43,172,9,129,22,39,253,19,98,108,110,79,
            113,224,232,178,185,112,104,218,246,97,228,251,34,242,193,238,210,144,12,
            191,179,162,241,81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,
            184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,222,114,67,29,24,
            72,243,141,128,195,78,66,215,61,156,180]
        return source + source   // doubled to avoid index wrapping
    }()

    private static func fade(_ t: Double) -> Double { t * t * t * (t * (t * 6 - 15) + 10) }
    private static func lerp(_ t: Double, _ a: Double, _ b: Double) -> Double { a + t * (b - a) }

    private static func grad(_ hash: Int, _ x: Double, _ y: Double, _ z: Double) -> Double {
        let h = hash & 15
        let u = h < 8 ? x : y
        let v = h < 4 ? y : (h == 12 || h == 14 ? x : z)
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v)
    }

    static func value(_ x: Double, _ y: Double, _ z: Double) -> Double {
        let xi = Int(floor(x)) & 255
        let yi = Int(floor(y)) & 255
        let zi = Int(floor(z)) & 255
        let xf = x - floor(x), yf = y - floor(y), zf = z - floor(z)
        let u = fade(xf), v = fade(yf), w = fade(zf)

        let a  = p[xi] + yi
        let aa = p[a] + zi
        let ab = p[a + 1] + zi
        let b  = p[xi + 1] + yi
        let ba = p[b] + zi
        let bb = p[b + 1] + zi

        let x1 = lerp(u, grad(p[aa],     xf,     yf,     zf),
                         grad(p[ba],     xf - 1, yf,     zf))
        let x2 = lerp(u, grad(p[ab],     xf,     yf - 1, zf),
                         grad(p[bb],     xf - 1, yf - 1, zf))
        let y1 = lerp(v, x1, x2)
        let x3 = lerp(u, grad(p[aa + 1], xf,     yf,     zf - 1),
                         grad(p[ba + 1], xf - 1, yf,     zf - 1))
        let x4 = lerp(u, grad(p[ab + 1], xf,     yf - 1, zf - 1),
                         grad(p[bb + 1], xf - 1, yf - 1, zf - 1))
        let y2 = lerp(v, x3, x4)
        return lerp(w, y1, y2)
    }
}
