# webgpu-dawn-template

## Setup
```bash
git clone https://github.com/0xbolt/webgpu-dawn-template
cd webgpu-dawn-template
git submodule update --init
```

## Requirements
- cmake
- emscripten

## Native Build
```bash
cmake -GNinja -B build && cmake --build build
```

## Web Build
```bash
emcmake cmake -GNinja -B build-web && cmake --build build-web
```

## References
- https://github.com/beaufortfrancois/webgpu-cross-platform-app



# TODO



## Rendering
- pipeline to opencv (DONE)
- multi camera
- proper lighting
- camera noise (gaussian noise on every color channel on every pixel)

## Vision
- The Algorithm (tm)
	 	- Image difference
	- Raycasting
	- Simple voxel grid
	- Ray/voxel intersection
	- Octree
- Additional
	- Trajectory analysis? (detect if it's a boid bird or a plane)

## Scene

- target object
- simple terrain (simplex noise?)
- boids (optional)
- tree that moves with wind (billboard is fine, slight deformation with a noise function)
- insect noise (random balls moving in the camera view)
