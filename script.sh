script_or_variable.sh > "${XDG_RUNTIME_DIR:-/tmp}/doomy.txt" # once now, so doomy finds the file when it starts
while true; do 
  script_or_variable.sh > "${XDG_RUNTIME_DIR:-/tmp}/doomy.txt"
  sleep 1
done &
