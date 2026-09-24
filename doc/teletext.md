# Teletext output

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

## Examples
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
