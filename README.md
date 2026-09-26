# Asciiquarium - Under The C

Asciiquarium is an aquarium/C animation in ASCII art.
This is a C-port of [Asciiquarium v1.1](https://github.com/cmatsuoka/asciiquarium),
see "Credits" below for the original authors.

Here is a [Live Demo](https://xplus2.github.io/underthec/?m=Press+h+for+help&M=white&n=UnderTheC) of the WebAssembly.
See [doc/wasm.md](doc/wasm.md) for a list of available GET parameters.

Target platforms:
* Linux
  - Terminal: amd64, arm64, armel, armhf, i386, riscv64
  - XScreensaver: amd64, arm64 (see [doc/screensaver.md](doc/screensaver.md))
* macOS
  - Terminal: arm64
* Windows
  - Terminal: amd64, arm64
  - Screensaver: amd64, arm64 (see [doc/screensaver.md](doc/screensaver.md))
* WebAssembly
* EBU Teletext: text and mosaic, t42 and TS/PES  (see [doc/teletext.md](doc/teletext.md))


## Build
See [doc/build.md](doc/build.md) for detailed instructions.
It's a walk in the waterpark, but if you prefer pre-built releases by GitHub workflows, look [here](https://github.com/xplus2/underthec/releases).

### Dependencies

Not really. 
`libx11`+`libxft` for the X11 screensaver, if you're still on X11.

From time to time, it is recommended to feed the fish.

## Usage

```sh
underthec [-c [1.0|1.1]] [-s] [-t] [-p pace] [-u N] [-f N] [-m text|-] [-M color] [-P position] [-n text] [--no-castle]
          [-a definition] [teletext options...]
underthec {-h|-v}
```

| Short | Long                 | Parameter        | Description                           |
|-------|----------------------|------------------|---------------------------------------|
| `-a`  | `--aquatic-life`     | `<def>`          | decide what's in (see below)          |
| `-c`  | `--classic`          | `[1.0\|1.1]`     | Asciiquarium 1.0 / 1.1 modes          |
| `-m`  | `--message`          | `<text>`         | background `text`. `-` for stdin      |
| `-M`  | `--message-color`    | `<color>`        | `-m`'s text color (see below)         |
| `-P`  | `--message-position` | `<pos>`          | `-m`'s placement (see below)          |
| `-p`  | `--pace`             | `<pace>`         | speed, 0.01-10 (default: 1)           |
| `-u`  | `--uturn-chance`     | `<N>`            | fish turn chance (default: 1 in 200)  |
| `-f`  | `--fps`              | `<N>`            | render fps, 1-120 (default: 10)       |
| `-s`  | `--screensaver`      |                  | (terminal) exit on any keypress       |
| `-t`  | `--transparent`      |                  | transparent background                |
|       | `--teletext`         | `<t42\|ts>`      | Teletext to stdout (see below)        |
|       | `--teletext-mode`    | `<text\|mosaic>` | teletext glyphs, default: text        |
|       | `--mcast`            | `<GROUP:PORT>`   | MPEG-TS teletext multicast group      |
|       | `--ttl`              | `<N>`            | multicast TTL (default: 1)            |
|       | `--iface`            | `<if>`           | multicast interface                   |
|       | `--teletext-caption` | `<text>`         | teletext caption (default: UNDERTHEC) |
| `-n`  | `--castle-name`      | `<text>`         | text on the castle, max 11 chars      |
|       | `--no-castle`        |                  | disable the castle                    |
| `-h`  | `--help`             |                  | show usage                            |
| `-v`  | `--version`          |                  | show version                          |

### Classic mode `-c`/`--classic`
- `-c 1.0` (or bare `-c`): original 1.0 fish/monster look
- `-c 1.1`: upstream 1.1 fish/monster look, restricted to upstream's species

Not combinable with `-a`.

### Text colors `-M`|`--message-color`
> Valid text colors: `red`, `green`, `blue`, `yellow`, `magenta`, `cyan`, `white`, `black`.
> Capitalized first letter: bold.

### Message position `-P`|`--message-position`
| Value     | Placement                                               |
|-----------|---------------------------------------------------------|
| `middle`  | horizontally+vertically centered (default)              |
| `center`  | horizontally centered, vertical top                     |
| `marquee` | vertically centered, scrolls right to left, repeats     |
| `swim`    | top row, scrolls right to left, repeats                 |
| `event`   | like `swim`, but takes turns with ducks/swans/ship/etc  |

### Aquatic life `-a`/`--aquatic-life` 
Define what's going on in your asciiquarium. It takes a comma-separated definition:

- `fish=<N|auto>`: number of fish (default: `auto`, sized to the terminal)
- flags, present=on, omitted=off: `ducks`, `dolphins`, `ship`, `swan`, `kaiju`, `fishhook`,
  `submarine`, `whale`, `shark`, `jellyfish`, `monster`, `bigfish`, `swordfish`, `crab`,
  `seahorse`, `rowers`

Default (no `-a`): every flag on, `fish=auto`. Example: `-a fish=10,jellyfish,dolphins`

### Environment variables
Each mirrors a command-line option.
If both an env var and its cmdline option are given, the cmdline option wins.

| Variable                               | Mirrors               |
|----------------------------------------|-----------------------|
| `UNDERTHEC_FISH=auto\|number`          | `fish=` from `-a`     |
| `UNDERTHEC_AQUATIC_LIFE=<def>`         | `-a`, except fish     |
| `UNDERTHEC_CLASSIC=1.0\|1.1`           | `-c`                  |
| `UNDERTHEC_MESSAGE=<text>`             | `-m`                  |
| `UNDERTHEC_MESSAGE_COLOR=<color>`      | `-M`                  |
| `UNDERTHEC_MESSAGE_POSITION=<pos>`     | `-P`                  |
| `UNDERTHEC_PACE=<pace>`                | `-p`                  |
| `UNDERTHEC_FPS=<N>`                    | `-f`                  |
| `UNDERTHEC_SCREENSAVER=0\|1`           | `-s`                  |
| `UNDERTHEC_UTURN_CHANCE=<N>`           | `-u`                  |
| `UNDERTHEC_TRANSPARENT=0\|1`           | `-t`                  |
| `UNDERTHEC_TELETEXT=t42\|ts`           | `--teletext`          |
| `UNDERTHEC_TELETEXT_MODE=text\|mosaic` | `--teletext-mode`     |
| `UNDERTHEC_TELETEXT_CAPTION=<text>`    | `--teletext-caption`  |
| `UNDERTHEC_MCAST=<GROUP:PORT>`         | `--mcast`             |
| `UNDERTHEC_MCAST_TTL=<N>`              | `--ttl`               |
| `UNDERTHEC_MCAST_IFACE=<if>`           | `--iface`             |
| `UNDERTHEC_CASTLE_NAME=<text>`         | `-n`                  |
| `UNDERTHEC_NO_CASTLE=0\|1`             | `--no-castle`         |

### Key bindings

| Key | Action                                                                   |
|-----|--------------------------------------------------------------------------|
| `f` | Feed: drop flakes for the fish                                           |
| `h` | Help about keys                                                          |
| `p` | Pause / resume                                                           |
| `r` | Redraw (recreate everything with fresh random positions)                 |
| `s` | Settings. Arrow to move the selector, `+`/`-`/`[space]` to make changes  |
| `t` | Toggle background transparency                                           |
| `q` | Quit (`^C` works too)                                                    |

`SIGUSR1` also triggers a feed.

## Credits

* The original asciiquarium program and most of its design are by [Kirk Baucom](https://robobunny.com/projects/asciiquarium/html/)
* A lot of the ASCII art is by [Joan Stark](https://web.archive.org/web/20091027174549/http://www.geocities.com/SoHo/7373/)
* This is a direct port of [cmatsuoka/asciiquarium](https://github.com/cmatsuoka/asciiquarium)
* Jellyfish is from [nothub/asciiquarium](https://github.com/nothub/asciiquarium) 

## License

It is a ship of Theseus. Anyway, Asciiquarium's original skipper (and crew, see "Credits" above) 
put it under GPL-2.0-or-later, see [LICENSE](LICENSE).

So, GPL-2.0-or-later it is.
