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
