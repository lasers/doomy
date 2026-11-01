f="${XDG_RUNTIME_DIR:-/tmp}/doomy.txt"
script_or_variable.sh > "$f" # once now, so doomy finds the file when it starts
while true; do
  # mv swaps in the whole file at once, so doomy never reads it empty or half written
  script_or_variable.sh > "$f.tmp"; mv "$f.tmp" "$f"
  sleep 1
done &
