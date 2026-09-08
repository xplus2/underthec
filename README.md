# Under The C

Asciiquarium is an aquarium/C animation in ASCII art.

This is a C-port of [Asciiquarium v1.1](https://github.com/cmatsuoka/asciiquarium), see "Credits" below for the original authors.
 
## Building

### CMake (primary)

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
underthec [-c] [-m text|-] [-M color]
underthec {-h|-v}
```

| Option                          | Description                                |
|---------------------------------|--------------------------------------------|
| `-c`, `--classic`               | asciiquarium 1.0 mode                      |
| `-m`, `--message <text>`        | background `text`. `-` to read from stdin  |
| `-M`, `--message-color <color>` | `-m` text color*:                          |
| `-h`, `--help`                  | Show usage and exit                        |
| `-v`, `--version`               | Show version and exit                      |

Valid text colors: `red`, `green`, `blue`, `yellow`, `magenta`, `cyan`, `white`, `black`.
Capitalized first letter: bold.

Key bindings:

| Key  | Action                                                   |
|------|----------------------------------------------------------|
| `q`  | Quit                                                     |
| `r`  | Redraw (recreate everything with fresh random positions) |
| `p`  | Pause / resume                                           |

## Credits

* The original asciiquarium program and most of its design are by [Kirk Baucom](https://robobunny.com/projects/asciiquarium/html/)
* A lot of the ASCII art is by [Joan Stark](http://www.geocities.com/SoHo/7373/)
* This is a direct port of [cmatsuoka/asciiquarium](https://github.com/cmatsuoka/asciiquarium)
* Jellyfish if from [nothub/asciiquarium](https://github.com/nothub/asciiquarium) 

## License

GPL-2.0-or-later, see [LICENSE](LICENSE).
