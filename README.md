# doomy

![doomy](https://raw.githubusercontent.com/su8/hellxcb/refs/heads/master/hellxcb.png)

Dead simple X11 bar. Every second it draws one line of text that your script writes to a file.

The bar in the picture above runs on [hellxcb](https://github.com/su8/hellxcb), with its text from [pinky-bar](https://github.com/su8/pinky-bar).

## Dependencies

A C compiler, `make`, `libX11`, `libXft`, `libXrender`, `fontconfig` and `argp` (part of glibc; `argp-standalone` on BSD).

Debian:

```sh
apt install gcc make libx11-dev libxft-dev libxrender-dev libfontconfig-dev
```

Fedora:

```sh
dnf install gcc make libX11-devel libXft-devel libXrender-devel fontconfig-devel
```

FreeBSD:

```sh
pkg install argp-standalone libXft pkgconf
```

## Installation

```sh
make                  # on BSD: make -f Make.bsd
sudo make install     # to /usr/bin
sudo make uninstall   # to remove it
```

Put these lines in `~/.xinitrc`, before the line that starts your window manager:

```sh
f="${XDG_RUNTIME_DIR:-/tmp}/doomy.txt"
your_script.sh > "$f"   # once now, so doomy finds the file when it starts
while true; do
  # mv swaps in the whole file at once, so doomy never reads it empty or half written
  your_script.sh > "$f.tmp"; mv "$f.tmp" "$f"
  sleep 1
done &
doomy &
```

`script.sh` in this repository holds the same loop.

## Usage

doomy reads the first line of `$XDG_RUNTIME_DIR/doomy.txt`, a folder only you can write to.
When that file is missing, it reads `/tmp/doomy.txt`, which older scripts write to.

- An empty file gives an empty bar.
- A line longer than 999 bytes is cut off.
- Deleting the file (and `/tmp/doomy.txt`, if there is one) stops doomy.

| Option | Default | What it does |
|---|---|---|
| `-b`, `--bgcolor` | `#282a2e` | background colour |
| `-t`, `--textcolor` | `#b294bb` | text colour |
| `-f`, `--font` | `xft#DejaVu Sans:size=9:style=bold` | font, e.g. `"Hack:size=10"` |
| `-h`, `--barheight` | `18` | height of the bar in pixels |
| `-s`, `--top` | `1` | `1` at the top of the screen, `0` at the bottom |
| `-a`, `--alpha` | `100` | opacity of the background in percent; the text stays solid |
| `-m`, `--tmode` | `auto` | how to be transparent, see below |

Colours are names like `red` or `#rrggbb`. doomy exits with a message on one it does not know.

### Colours

`[red]`, `[green]`, `[blue]` and `[yellow]` colour the text after them; `[/]` goes back to the text colour:

```sh
echo '[red]Hello [green]World [blue]Hi [yellow]again [/]plain' > "${XDG_RUNTIME_DIR:-/tmp}/doomy.txt"
```

### Emojis

Characters the font does not have, such as emojis or Chinese, are drawn with another installed font that has them:

```sh
echo '🔋 85% 🎵 music 🌡 45°C' > "${XDG_RUNTIME_DIR:-/tmp}/doomy.txt"
```

- **Bitmap Noto Color Emoji** (`NotoColorEmoji.ttf`, the usual one): colour emojis. Needs libXft 2.3.5 or newer.
- **Noto Emoji**: one-colour emojis, in the colour of their tag.
- **Only the COLRv1 colour emoji font** (`Noto-COLRv1.ttf`, the one Fedora ships): emojis are blank, because libXft cannot draw that format. Install Noto Emoji too.

### Transparency

With `--alpha` below 100, the background lets the desktop show through. `--tmode` picks how:

- `argb`: real transparency. It needs a compositor such as [picom](https://github.com/yshui/picom) or xcompmgr.
- `pseudo`: the wallpaper behind the bar, tinted with the background colour. It needs no compositor, but does not show windows behind the bar.
- `auto` (the default): `argb` when a compositor runs, else `pseudo`.

```sh
doomy --alpha 70
```

### With hellxcb

[hellxcb](https://github.com/su8/hellxcb) writes its desktop, mode and window counts to `$XDG_RUNTIME_DIR/hellxcb.txt`.
Its README shows a loop that puts that line into doomy's file.

## Testing

The smoke test runs doomy on a virtual X server and needs `Xvfb`:

```sh
make check                     # run the smoke test
SMOKE_DISPLAY=:98 make check   # use another display
```
