# WebAssembly

Keys like `h` for help, `f` to feed or `s`for settings work. 
Click or tap also feeds, but not necessarily at this position.

## Query params
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
