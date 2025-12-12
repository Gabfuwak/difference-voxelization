const vertices = array(
    vec2f(0, 0.5),
    vec2f(-0.5, -0.5),
    vec2f(0.5, -0.5)
);

const colors = array(
    vec4f(1, 0, 0, 1),
    vec4f(0, 1, 0, 1),
    vec4f(0, 0, 1, 1),
);

struct VertexOut {
    @builtin(position) position: vec4f,
    @location(0) color: vec4f,
};

@vertex
fn vertexMain(@builtin(vertex_index) i: u32) -> VertexOut {
    var out: VertexOut;
    out.position = vec4f(vertices[i], 0, 1);
    out.color = colors[i];
    return out;
}

@fragment
fn fragmentMain(in: VertexOut) -> @location(0) vec4f {
    return in.color;
}
