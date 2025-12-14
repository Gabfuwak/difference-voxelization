struct Params {
  resolution : vec2<f32>,
  time : f32,
  seed : f32,
};

@group(0) @binding(0) var<uniform> params : Params;

fn hash12(p: vec2<f32>) -> f32 {
  let p3 = fract(vec3<f32>(p.xyx) * 0.1031);
  let p3a = p3 + dot(p3, p3.yzx + 33.33 + params.seed);
  return fract((p3a.x + p3a.y) * p3a.z);
}

struct VSOut {
  @builtin(position) pos : vec4<f32>,
  @location(0) uv : vec2<f32>,
};

@vertex
fn vs_main(@builtin(vertex_index) vi: u32) -> VSOut {
  var out: VSOut;
  // Fullscreen triangle
  let p = array<vec2<f32>, 3>(
    vec2<f32>(-1.0, -1.0),
    vec2<f32>( 3.0, -1.0),
    vec2<f32>(-1.0,  3.0)
  );
  out.pos = vec4<f32>(p[vi], 0.0, 1.0);
  out.uv = 0.5 * (out.pos.xy + vec2<f32>(1.0));
  return out;
}

@fragment
fn fs_main(in: VSOut) -> @location(0) vec4<f32> {
  let frag = in.uv * params.resolution;
  let n = hash12(frag + vec2<f32>(params.time * 60.0, params.time * 13.0));
  return vec4<f32>(vec3<f32>(n), 1.0);
}
