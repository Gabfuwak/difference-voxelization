# Multi-view detection system 

(very) Inspired by Consistently Inconsistent : https://www.youtube.com/watch?v=YZkLQsv3huo

## Setup
```bash
git clone https://github.com/Gabfuwak/difference-voxelization
cd difference-voxelization
git submodule update --init
```

## Requirements
- cmake
- Eigen
- OpenCV

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
- https://github.com/ConsistentlyInconsistentYT/Pixeltovoxelprojector



# TODO

## Rendering
- pipeline to opencv (DONE)
- proper multi object scene rendering (DONE)
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
    - Moving camera (estimate optical flow)

## Scene

- target object
- simple terrain (simplex noise?)
- boids (optional)
- tree that moves with wind (billboard is fine, slight deformation with a noise function)
- insect noise (random balls moving in the camera view)
