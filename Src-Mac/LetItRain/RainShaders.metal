#include <metal_stdlib>
using namespace metal;

// Shaders shared by both renderers. Vertices arrive pre-transformed into NDC
// (the CPU does the layout), so the vertex stage is a passthrough; the fragment
// stage differentiates the primitive types.

// Input vertex — attribute indices match the Swift `RainVertex` layout.
struct Vtx {
    float2 pos [[attribute(0)]];   // NDC position
    float2 uv  [[attribute(1)]];   // circle SDF clip / atlas sample coords
    float4 col [[attribute(2)]];   // RGBA tint
};

// Interpolated vertex → fragment payload.
struct V2F {
    float4 pos [[position]];
    float2 uv;
    float4 col;
};

// Passthrough: positions are already in clip space.
vertex V2F mainVertex(Vtx in [[stage_in]]) {
    V2F out;
    out.pos = float4(in.pos, 0.0, 1.0);
    out.uv  = in.uv;
    out.col = in.col;
    return out;
}

// Rain drop trail — plain quad, no clipping
fragment float4 dropFragment(V2F in [[stage_in]]) {
    return in.col;
}

// Splatter dot — circle SDF clip via UV
fragment float4 splatterFragment(V2F in [[stage_in]]) {
    if (length(in.uv) > 1.0) discard_fragment();
    return in.col;
}

// Snowflake — sample the baked shape atlas (white shape, coverage in the alpha
// channel) and tint by the vertex color. The texture supplies the antialiased
// shape; the vertex color supplies the flake's opacity.
fragment float4 snowFlakeFragment(V2F in [[stage_in]],
                                  texture2d<float> atlas [[texture(0)]],
                                  sampler smp [[sampler(0)]]) {
    float cov = atlas.sample(smp, in.uv).a;
    return float4(in.col.rgb, in.col.a * cov);
}
