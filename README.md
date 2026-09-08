# Under The C

Asciiquarium is an aquarium/C animation in ASCII art.

This is a C-port of [Asciiquarium v1.1](https://github.com/cmatsuoka/asciiquarium), see "Credits" below for the original authors.

[Build your own](BUILD.md) or use one of the [releases](https://github.com/xplus2/underthec/releases).

## Usage

```
underthec [-c] [-s] [-t] [-m text|-] [-M color] [-a definition]
underthec {-h|-v}
```

| Option                               | Description                                |
|--------------------------------------|--------------------------------------------|
| `-c`, `--classic`                    | asciiquarium 1.0 mode                      |
| `-s`, `--screensaver`                | exit on any keypress                       |
| `-t`, `--transparent`                | transparent background                     |
| `-m`, `--message <text>`             | background `text`. `-` to read from stdin  |
| `-M`, `--message-color <color>`      | `-m`'s text color (see below)              |
| `-a`, `--aquatic-life <definition>`  | select which creatures show up (see below) |
| `-h`, `--help`                       | show usage                                 |
| `-v`, `--version`                    | show version                               |

### Text colors `-M`|`--message-color`
> Valid text colors: `red`, `green`, `blue`, `yellow`, `magenta`, `cyan`, `white`, `black`.
> Capitalized first letter: bold.

### Aquatic life `-a`/`--aquatic-life` 
Define what's going on in your asciiquarium. It takes a comma-separated definition:

- `fish=<N|auto>`: number of fish (default: `auto`, sized to the terminal)
- flags, present=on, omitted=off: `ducks`, `dolphins`, `ship`, `swan`, `kaiju`, `fishhook`,
  `submarine`, `whale`, `shark`, `jellyfish`, `monster`, `bigfish`, `swordfish`, `crab`

Default (no `-a`): every flag on, `fish=auto`. Example: `-a fish=10,jellyfish,dolphins`


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
