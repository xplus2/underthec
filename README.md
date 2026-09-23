# Asciiquarium - Under The C

Asciiquarium is an aquarium/C animation in ASCII art.

This is a C-port of [Asciiquarium v1.1](https://github.com/cmatsuoka/asciiquarium),
see "Credits" below for the original authors.

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
| `--teletext <t42\|ts>`               | EBU Teletext stream to stdout (sea below)  |
| `--teletext-mode <text\|mosaic>`     | teletext glyphs, default: text             |
| `--mcast <GROUP:PORT>`               | MPEG-TS teletext to a multicast group      |
| `--ttl <N>`                          | multicast TTL/hops, 1-255 (default: 1)     |
| `--iface <if>`                       | multicast interface                        |
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
  `submarine`, `whale`, `shark`, `jellyfish`, `monster`, `bigfish`, `swordfish`, `crab`,
  `seahorse`

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
| `UNDERTHEC_TELETEXT=t42\|ts`      | `--teletext`      |
| `UNDERTHEC_TELETEXT_MODE=text\|mosaic` | `--teletext-mode` |
| `UNDERTHEC_MCAST=<GROUP:PORT>`    | `--mcast`         |
| `UNDERTHEC_MCAST_TTL=<N>`         | `--ttl`           |
| `UNDERTHEC_MCAST_IFACE=<if>`      | `--iface`         |


### Key bindings

| Key | Action                                                                       |
|-----|------------------------------------------------------------------------------|
| `f` | Feed: drop flakes for the fish                                               |
| `h` | Help about keys                                                              |
| `p` | Pause / resume                                                               |
| `r` | Redraw (recreate everything with fresh random positions)                     |
| `s` | Settings. Arrow to move the selector, `+`/`-`/`[space]` to make changes |
| `t` | Toggle background transparency                                               |
| `q` | Quit (`^C` works too)                                                        |

`SIGUSR1` also triggers a feed.



### WebAssembly

Keys work (`h`for help), click or tap also feeds, but not necessarily at this position.

Options can go in the URL query, by long or short name (long wins if both are set),
with the same values as on the command line:

| long               | short | example                |
|--------------------|-------|------------------------|
| `classic`          | `c`   | `?classic` or `?c=1.1` |
| `aquatic-life`     | `a`   | `?a=fish=5,ducks,crab` |
| `message`          | `m`   | `?m=HowMuchIsTheFish`  |
| `message-color`    | `M`   | `?M=Yellow`            |
| `message-position` | `P`   | `?P=middle`            |
| `pace`             | `p`   | `?p=2`                 |
| `uturn-chance`     | `u`   | `?u=50`                |
| `fps`              | `f`   | `?f=30`                |

An invalid value shows the error instead of the aquarium.

### Windows screensaver

`underthec.scr` is a native Windows multi-monitor screensaver.
Right-click it and choose "Install", or copy it to `C:\Windows\System32`,
then select it in the screensaver settings.

### Teletext output

Instead of the terminal, output can be rendered as an EBU Teletext page (page 100) and streamed:

* `--teletext t42`: raw 42-byte teletext packets to stdout (t42)
* `--teletext ts`: MPEG-TS to stdout, teletext as private PES (EN 300 472) on PID 0x100, with PAT, PMT and PCR
* `--mcast GROUP:PORT`: the same MPEG-TS over UDP multicast
  IPv6 groups are written `[GROUP]:PORT`.
* `--iface` takes a local address (IPv4) or an interface name (IPv6). Not available on Windows.
* `--teletext-mode mosaic` draws 2x3 mosaic blocks. `--teletext-mode text` (default) draws text characters, one per cell.
  Text mode is 39x25. The Level 1 character set swaps `# [ \ ] ^ _ ` { | } ~` for national characters, so those are sent
  as X/26 enhancement packets (Level 1.5). Decoders without X/26 support show look-alikes instead.

Binary output is refused when stdout is a terminal.
Output is paced by `-f` (default 10 frames/s).
In mosaic mode the tank is fixed at 78x25 cells, drawn as 2x3 mosaic blocks in the 7 teletext colors.
Bold is ignored, gray ("bold black") is mapped to white.
Each second, the whole page is retransmitted, in between only changed rows.

Examples:
* Generate a ready-made: `underthec --teletext ts > aquarium.ts`
* Multicast: `underthec --mcast 239.1.1.1:5004`

If you want to test it locally in VLC, it will need an alibi-video ES:

* `underthec --mcast 239.1.1.1:5000 --iface 127.0.0.1`
* ```sh 
  ffmpeg -f lavfi -i color=c=black:s=720x576:r=25 -i "udp://239.1.1.1:5000?localaddr=127.0.0.1" \
    -map 0:v -map 1:s -c:v mpeg2video -b:v 500k -c:s copy \
    -f mpegts "udp://239.1.1.2:5000?ttl=1&pkt_size=1316"
  ```
* Wait until ffmpeg produces an output
* and launch VLC like this:
  `vlc udp://@239.1.1.2:5000` (or open VLC, ^N and enter `udp://@239.1.1.2:5000`)
* Press the Teletext button, stay on page 100

## Credits

* The original asciiquarium program and most of its design are by [Kirk Baucom](https://robobunny.com/projects/asciiquarium/html/)
* A lot of the ASCII art is by [Joan Stark](http://www.geocities.com/SoHo/7373/)
* This is a direct port of [cmatsuoka/asciiquarium](https://github.com/cmatsuoka/asciiquarium)
* Jellyfish if from [nothub/asciiquarium](https://github.com/nothub/asciiquarium) 

## License

GPL-2.0-or-later, see [LICENSE](LICENSE).
