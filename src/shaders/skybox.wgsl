struct Globals {
    viewProjection: mat4x4f,
    viewProjectionInv: mat4x4f,
    cameraPos: vec3f,
};

@group(0) @binding(0) var<uniform> globals: Globals;
@group(1) @binding(0) var skyboxTexture: texture_cube<f32>;
@group(1) @binding(1) var skyboxSampler: sampler;

struct VertexInput {
    @builtin(vertex_index) vertexIndex: u32,
};

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) pos: vec4f,
};

@vertex
fn vertexMain(in: VertexInput) -> VertexOutput {
    const vertices = array(
        vec2f(-1, -1),
        vec2f(3, -1),
        vec2f(-1, 3),
    );

    var out: VertexOutput;
    out.position = vec4f(vertices[in.vertexIndex], 0, 1);
    out.pos = out.position;
    return out;
}

@fragment
fn fragmentMain(in: VertexOutput) -> @location(0) vec4f {
    let pos = globals.viewProjectionInv * in.pos;
    let dir = normalize(pos.xyz / pos.w - globals.cameraPos);
    return textureSample(skyboxTexture, skyboxSampler, dir);
}
