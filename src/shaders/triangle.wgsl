@group(0) @binding(0) var<uniform> time: f32;

@vertex
fn vert(@builtin(vertex_index) i: u32) -> @builtin(position) vec4f {
    const pos = array(
        vec2f(0, 0.5),
        vec2f(-0.5, -0.5),
        vec2f(0.5, -0.5)
    );
    return vec4f(pos[i], 0, 1);
}

@fragment
fn frag() -> @location(0) vec4f {
    const tau = radians(180.0);
    const a = vec3f(0.5, 0.5, 0.5);
    const b = vec3f(0.5, 0.5, 0.5);
    const c = vec3f(1.0, 1.0, 1.0);
    const d = vec3f(0, 0.33, 0.67);
    let t = time;

    return vec4f(a + b * cos(tau * (c * t + d)), 1);
}
