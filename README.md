# CorePhys

CorePhys is a Jimokomi-focused fork of Box2D.

The first goal of this fork is to give Jimokomi a physics engine we can modify, build, and version directly while preserving the current Box2D C API used by the engine. The public include path is still `box2d/box2d.h` in this initial fork step, and the exported C symbols still use Box2D's `b2` naming.

## Current Scope

- CMake project name: `corephys`
- Library target: `corephys`
- CMake package namespace: `corephys::`
- Internal source is based on Box2D 3.2.0
- Existing Box2D C headers and runtime symbols are intentionally preserved for source compatibility with Jimokomi

## Building

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

When embedded through Jimokomi, CorePhys is built from `third_party/corephys` by the root Jimokomi `CMakeLists.txt`.

## Upstream

CorePhys is based on Box2D by Erin Catto.

- Upstream project: https://github.com/erincatto/box2d
- Documentation: https://box2d.org/documentation/

## License

CorePhys keeps the upstream Box2D MIT license. See `LICENSE`.
