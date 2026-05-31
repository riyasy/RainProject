#include <metal_stdlib>
using namespace metal;

struct Vtx {
    float2 pos [[attribute(0)]];
    float2 uv  [[attribute(1)]];
    float4 col [[attribute(2)]];
};

struct V2F {
    float4 pos [[position]];
    float2 uv;
    float4 col;
};

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
