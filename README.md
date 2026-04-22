# CorePhys

CorePhys is a Jimokomi-focused 2D physics engine.

The first goal of this fork is to give Jimokomi a physics engine we can modify, build, and version directly. The public include path is `corephys/corephys.h`.

## Current Scope

- CMake project name: `corephys`
- Library target: `corephys`
- CMake package namespace: `corephys::`
- Internal source is based on the upstream 3.2.0 physics engine lineage
- Existing `b2` C runtime symbols are intentionally preserved for source compatibility with Jimokomi during this first rebrand pass

## Building

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

When embedded through Jimokomi, CorePhys is built from `third_party/corephys` by the root Jimokomi `CMakeLists.txt`.

## Lineage

CorePhys keeps the upstream solver lineage and license attribution. Project-facing names, build targets, include paths, and documentation in this repository use CorePhys naming.

## License

CorePhys uses the MIT license. See `LICENSE`.
