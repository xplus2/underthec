# Asciiquarium - Under The C

Asciiquarium is an aquarium/C animation in ASCII art.

This is a C-port of [Asciiquarium v1.1](https://github.com/cmatsuoka/asciiquarium), see "Credits" below for the original authors.

[Build your own](BUILD.md) or use one of the [releases](https://github.com/xplus2/underthec/releases).

## Usage

```
underthec [-c [1.0|1.1]] [-s] [-t] [-p pace] [-m text|-] [-M color] [-P position] [-a definition]
underthec {-h|-v}
```

| Option                               | Description                                |
|--------------------------------------|--------------------------------------------|
| `-a`, `--aquatic-life <definition>`  | select which creatures show up (see below) |
| `-c`, `--classic [1.0\|1.1]`         | asciiquarium 1.0 / 1.1 modes               |
| `-m`, `--message <text>`             | background `text`. `-` to read from stdin  |
| `-M`, `--message-color <color>`      | `-m`'s text color (see below)              |
| `-P`, `--message-position <pos>`     | `-m`'s placement (see below)               |
| `-p`, `--pace <pace>`                | speed multiplier, 0.01-10 (default: 1)     |
| `-u`, `--uturn-chance <N>`           | fish turn once per N ticks (default: 200)  |
| `-f`, `--fps <N>`                    | render fps, 1-120 (default: 10)            |
| `-s`, `--screensaver`                | exit on any keypress                       |
| `-t`, `--transparent`                | transparent background                     |
| `-h`, `--help`                       | show usage                                 |
| `-v`, `--version`                    | show version                               |

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
  `submarine`, `whale`, `shark`, `jellyfish`, `monster`, `bigfish`, `swordfish`, `crab`

Default (no `-a`): every flag on, `fish=auto`. Example: `-a fish=10,jellyfish,dolphins`

### Environment variables
Each mirrors a command-line option.
If both an env var and its cmdline option are given, the cmdline option wins.

| Variable                          | Mirrors           |
|-----------------------------------|-------------------|
| `UNDERTHEC_FISH=auto\|number`     | `fish=` from `-a` |
| `UNDERTHEC_AQUATIC_LIFE=<def>`    | `-a`, except fish |
| `UNDERTHEC_CLASSIC=1.0\|1.1`      | `-c`              |
| `UNDERTHEC_MESSAGE=<text>`        | `-m`              |
| `UNDERTHEC_MESSAGE_COLOR=<color>` | `-M`              |
| `UNDERTHEC_MESSAGE_POSITION=<pos>`| `-P`              |
| `UNDERTHEC_PACE=<pace>`           | `-p`              |
| `UNDERTHEC_FPS=<N>`               | `-f`              |
| `UNDERTHEC_SCREENSAVER=0\|1`      | `-s`              |
| `UNDERTHEC_UTURN_CHANCE=<N>`      | `-u`              |
| `UNDERTHEC_TRANSPARENT=0\|1`      | `-t`              |

### Key bindings

| Key          | Action                                                   |
|--------------|----------------------------------------------------------|
| `q` or `^C`  | Quit                                                     |
| `r`          | Redraw (recreate everything with fresh random positions) |
| `p`          | Pause / resume                                           |
| `t`          | Toggle background transparency                           |

## Credits

* The original asciiquarium program and most of its design are by [Kirk Baucom](https://robobunny.com/projects/asciiquarium/html/)
* A lot of the ASCII art is by [Joan Stark](http://www.geocities.com/SoHo/7373/)
* This is a direct port of [cmatsuoka/asciiquarium](https://github.com/cmatsuoka/asciiquarium)
* Jellyfish if from [nothub/asciiquarium](https://github.com/nothub/asciiquarium) 

## License

GPL-2.0-or-later, see [LICENSE](LICENSE).
