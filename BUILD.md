# Building

## CMake (primary)

```sh
cmake -B build
cmake --build build
```

The `Release` build type (the default) links statically wherever the target platform allows it, and strips the resulting binary.

Static musl build on Linux (requires `musl-gcc`):

```sh
cmake -B build-musl -DCMAKE_TOOLCHAIN_FILE=cmake/musl-toolchain.cmake
cmake --build build-musl
```

### Windows
Cross-compiling a static Windows build requires the `mingw-w64` cross toolchain:

It creates the terminal binary as well as the screensaver.

```sh
cmake -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-toolchain.cmake
cmake --build build-win
```

Windows builds also produce the screensaver `underthec.scr`.
It needs `windres` (comes with mingw-w64).
With configure + make, `--host=*-mingw32` builds `build/underthec.scr`.

### macOS
The CMake build above should natively work on macOS.
Apple's libSystem does not support fully static binaries.

## configure + make (legacy)

```sh
./configure
make
make install
```

`./configure --help` lists the available options (`--prefix`, `--host`
for cross-compiling, `--no-static`, `--debug`, `--emcc`).
Pass `--debug` for an unstripped `-g -O0` build.

## WebAssembly

Requires Emscripten (`emcc`, `emcmake`). Output is `underthec.js`, `underthec.wasm`
and `index.html` in the build directory.

```sh
emcmake cmake -B build-web
cmake --build build-web
```
or
```sh
./configure --emcc
make
```

Browsers don't load wasm from `file://`, serve the directory over HTTP.
