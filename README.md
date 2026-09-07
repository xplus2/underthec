# Under The C

Asciiquarium is an aquarium/C animation in ASCII art.

This is a port of [Asciiquarium v1.1](https://robobunny.com/projects/asciiquarium/), see "Credits" below for the original authors.
 
## Building

### CMake (primary)

```
cmake -B build
cmake --build build
```

The `Release` build type (the default) links statically wherever the
target platform allows it, and strips the resulting binary.

Static musl build on Linux (requires `musl-gcc`):

```
cmake -B build-musl -DCMAKE_TOOLCHAIN_FILE=cmake/musl-toolchain.cmake
cmake --build build-musl
```

#### Windows
Cross-compiling a static Windows build requires the `mingw-w64` cross toolchain:

```
cmake -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-toolchain.cmake
cmake --build build-win
```

#### macOS
The CMake build above should natively work on macOS. 
Apple's libSystem does not support fully static binaries.

### configure + make (legacy)

```
./configure
make
make install
```

`./configure --help` lists the available options (`--prefix`, `--host`
for cross-compiling, `--no-static`, `--debug`).
Pass `--debug` for an unstripped `-g -O0` build.

## Usage

```
underthec [-c] [-m text|-]
underthec {-h|-v}
```

| Option                   | Description                               |
|--------------------------|-------------------------------------------|
| `-c`, `--classic`        | asciiquarium 1.0 mode                     |
| `-m`, `--message <text>` | background `text`. `-` to read from stdin |
| `-h`, `--help`           | Show usage and exit                       |
| `-v`, `--version`        | Show version and exit                     |

Key bindings:

| Key  | Action                                                   |
|------|----------------------------------------------------------|
| `q`  | Quit                                                     |
| `r`  | Redraw (recreate everything with fresh random positions) |
| `p`  | Pause / resume                                           |

## Credits

* The original asciiquarium program and most of its design are by Kirk Baucom.
* Additional fish and sea monster species by Claudio Matsuoka.
* Yellow Submarine by Carl Pilcher
* Swordfish by ctr
* Restored ducks, dolphins, swan and fishhook from previous versions
* The ASCII art is by Joan Stark.

## License

GPL-2.0-or-later, see [LICENSE](LICENSE).
