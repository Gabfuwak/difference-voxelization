struct VertexInput {
    @location(0) position: vec3f,
    @location(1) color: vec3f,
    @location(2) uv: vec2f,
}

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) color: vec3f,
    @location(1) uv: vec2f,
}

@group(0) @binding(0) var<uniform> mvp: mat4x4f;
@group(0) @binding(1) var myTexture: texture_2d<f32>;
@group(0) @binding(2) var mySampler: sampler;

@vertex
fn vertexMain(in: VertexInput) -> VertexOutput {
    var out: VertexOutput;
    out.position = mvp * vec4f(in.position, 1.0);
    out.color = in.color;
    out.uv = in.uv;
    return out;
}

@fragment
fn fragmentMain(in: VertexOutput) -> @location(0) vec4f {
    let texColor = textureSample(myTexture, mySampler, in.uv);
    return texColor * vec4f(in.color, 1.0);
}
