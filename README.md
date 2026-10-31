![](https://raw.githubusercontent.com/su8/hellxcb/master/1pic.png)

# doomy

Dead simple X11 bar for use with scripts in a loop

# Compile

```makefile
# if on BSD: mv Makefile Make.old && cp -r Make.bsd Makefile
make -j8 # where 8 are CPU cores.
sudo make install

doomy --help # to see the available options
```

# Usage

The script below comes with this repository.

The program in use in the above picture is from my other project https://github.com/su8/pinky-bar

Just add the following in your `.xinitrc`, right before the `case` label to test different WM's

```bash
# Top src script to output the desired data and `doomy' to read it from /tmp/doomy.txt
# filename outputForDoomy.sh
while true; do 
  script_or_variable.sh > /tmp/doomy.txt
  sleep 1
done &

# And from the same shell call
doomy &
# Enjoy :}
```

### Colours

```bash
# red , greem, blue, yellow
echo [red]Hello [green]World [blue]Hi [yellow]again [/]pink string > /tmp/doomy.txt
```

# Requirements

gcc/clang

argp , on BSD it's called argp-standalone

libXft

xorg libraries and headers
