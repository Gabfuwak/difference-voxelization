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
