# Building

## CMake (primary)

```
cmake -B build
cmake --build build
```

The `Release` build type (the default) links statically wherever the target platform allows it, and strips the resulting binary.

Static musl build on Linux (requires `musl-gcc`):

```
cmake -B build-musl -DCMAKE_TOOLCHAIN_FILE=cmake/musl-toolchain.cmake
cmake --build build-musl
```

### Windows
Cross-compiling a static Windows build requires the `mingw-w64` cross toolchain:

```
cmake -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-toolchain.cmake
cmake --build build-win
```

### macOS
The CMake build above should natively work on macOS.
Apple's libSystem does not support fully static binaries.

## configure + make (legacy)

```
./configure
make
make install
```

`./configure --help` lists the available options (`--prefix`, `--host`
for cross-compiling, `--no-static`, `--debug`).
Pass `--debug` for an unstripped `-g -O0` build.
