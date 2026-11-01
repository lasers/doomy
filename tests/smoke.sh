#!/bin/sh
# smoke test: run doomy on a virtual X server, do the things that used to crash it,
# and check how it ends. Needs: Xvfb
set -u
BIN=${1:-./doomy}
export DISPLAY=${SMOKE_DISPLAY:-:99} # another display: SMOKE_DISPLAY=:98 make check
export XDG_RUNTIME_DIR=$(mktemp -d) # a doomy.txt of our own
F=$XDG_RUNTIME_DIR/doomy.txt
Xvfb "$DISPLAY" -screen 0 800x600x24 >/dev/null 2>&1 & XPID=$!
trap 'kill $XPID 2>/dev/null; rm -rf "$XDG_RUNTIME_DIR"' EXIT
sleep 1
fail=0

# expect NAME GOT WANT: pass if the two values are equal
expect() { if [ "$2" = "$3" ]; then echo "ok   $1"; else echo "FAIL $1 (got $2, want $3)"; fail=1; fi; }
# alive NAME: write the text in $F, pass if doomy is still drawing it 2 seconds later (timeout's 124)
alive() { timeout 2 "$BIN" 2>/dev/null; expect "$1" $? 124; }

echo '[red]red [green]green [blue]blue [yellow]yellow [/]plain 🔋 日本' > "$F"; alive "colour tags, emojis and CJK"
awk 'BEGIN { while (i++ < 5000) printf "x"; print "" }' > "$F";      alive "a line longer than the buffer"
: > "$F";                                                               alive "an empty file"

# deleting the file is how doomy is stopped: it used to free its drawing twice and crash
echo text > "$F"; timeout 5 "$BIN" 2>/dev/null & P=$!; sleep 1.5; rm "$F"; wait $P
expect "deleting doomy.txt exits cleanly" $? 0

echo text > "$F"; timeout 2 "$BIN" -b nosuchcolour 2>/dev/null
expect "an unknown --bgcolor exits with an error" $? 1

exit $fail
