struct Uniforms {
    mvp: mat4x4f,
}
@binding(0) @group(0) var<uniform> uniforms: Uniforms;

@binding(1) @group(0) var diffuseTexture: texture_2d<f32>;
@binding(2) @group(0) var diffuseSampler: sampler;
@binding(3) @group(0) var maskTexture: texture_2d<f32>;

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) color: vec3f,
    @location(1) uv: vec2f,
}

@vertex
fn vertexMain(@location(0) position: vec3f,
              @location(1) color: vec3f,
              @location(2) uv: vec2f) -> VertexOutput {
    var output: VertexOutput;
    output.position = uniforms.mvp * vec4f(position, 1.0);
    output.color = color;
    output.uv = uv;
    return output;
}

@fragment
fn fragmentMain(@location(0) color: vec3f,
                @location(1) uv: vec2f) -> @location(0) vec4f {
    let diffuseColor = textureSample(diffuseTexture, diffuseSampler, uv);
    let maskValue = textureSample(maskTexture, diffuseSampler, uv).r;
    
    // Discard pixels where mask is black (adjust threshold as needed)
    if (maskValue < 0.5) {
        discard;
    }
    
    return vec4f(color * diffuseColor.rgb, 1.0);
}
