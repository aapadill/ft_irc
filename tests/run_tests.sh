#!/usr/bin/env bash
###############################################################################
# run_tests.sh – POSIX‐safe CI suite for 42 ft_irc
# (auto-cleanup, port‐wait, env‐driven password)
###############################################################################

PORT=6667
# Pick up password from $IRC_PASS, or fall back to secret42
PASS="${IRC_PASS:-secret42}"
SERVER_BIN="./ircserv"
STARTUP_TIMEOUT=2
CLIENT_TIMEOUT=4
failures=0

log()  { printf "[INFO]  %s\n" "$*"; }
pass() { printf "[PASS]  %s\n" "$1"; }
fail() { printf "[FAIL]  %s\n" "$1"; failures=$((failures+1)); }

# cleanup any leftovers
rm -f *.out /tmp/ft_irc.log

# helper to run one test
run_test() {
  local name="$1" pattern="$2" cmds="$3"
  log "Running test: $name"
  tmp=$(mktemp)
  # fire nc in background
  ( printf "%b" "$cmds" | nc localhost "$PORT" >"$tmp" 2>&1 ) &
  nc_pid=$!
  sleep "$CLIENT_TIMEOUT"
  if kill -0 "$nc_pid" 2>/dev/null; then
    kill "$nc_pid" 2>/dev/null
    wait "$nc_pid" 2>/dev/null
    fail "$name (timeout after ${CLIENT_TIMEOUT}s)"
    rm -f "$tmp"
    return
  fi
  wait "$nc_pid" 2>/dev/null
  if grep -qE "$pattern" "$tmp"; then
    pass "$name"
  else
    echo "  ➤ Expected /$pattern/ but got:"
    sed 's/^/    /' "$tmp"
    fail "$name"
  fi
  rm -f "$tmp"
}

###############################################################################
# Build & start server
###############################################################################

log "Building…"
make -j

log "Starting server on port $PORT (password='$PASS')"
"$SERVER_BIN" "$PORT" "$PASS" >/tmp/ft_irc.log 2>&1 &
SERVER_PID=$!

# wait up to 5s for TCP port to open
for i in $(seq 1 5); do
  if ( exec 3<>/dev/tcp/localhost/$PORT ) 2>/dev/null; then
    log "Port $PORT is open—continuing"
    break
  fi
  log "  waiting for port $PORT… ($i/5)"
  sleep 1
done

# cleanup on exit
trap 'kill $SERVER_PID 2>/dev/null; rm -f *.out /tmp/ft_irc.log' EXIT

###############################################################################
# 1. Registration handshake
###############################################################################
run_test "registration"      "001 alice .*:Welcome"    $'PASS '"$PASS"'\r\nNICK alice\r\nUSER alice 0 * :Alice\r\nQUIT\r\n'

###############################################################################
# 2. Duplicate nickname
###############################################################################
run_test "duplicate_nick"    "433 .*:Nickname is already in use" \
                                 $'PASS '"$PASS"'\r\nNICK alice\r\nUSER bob 0 * :Bob\r\nQUIT\r\n'

###############################################################################
# 3. JOIN channel & NAMES
###############################################################################
run_test "join_channel"      "353 chris = #42"         $'PASS '"$PASS"'\r\nNICK chris\r\nUSER chris 0 * :Chris\r\nJOIN #42\r\nQUIT\r\n'

###############################################################################
# 4. PRIVMSG broadcast
###############################################################################
run_test "privmsg"           ":dave!.* PRIVMSG #42 :hi" $'\
PASS '"$PASS"'\r\n\
NICK dave\r\n\
USER dave 0 * :Dave\r\n\
JOIN #42\r\n\
PRIVMSG #42 :hi\r\n\
QUIT\r\n\
'

###############################################################################
# 5. MODE +o & KICK
###############################################################################
run_test "mode_kick"         "KICK #42 dave :rules"   $'PASS '"$PASS"'\r\nNICK op\r\nUSER op 0 * :Op\r\nJOIN #42\r\nMODE #42 +o op\r\nKICK #42 dave :rules\r\nQUIT\r\n'

###############################################################################
# 6a. Set +i and INVITE
###############################################################################
run_test "invite_only_setup" "INVITE frank #invite"   $'PASS '"$PASS"'\r\nNICK op\r\nUSER op 0 * :Op\r\nJOIN #invite\r\nMODE #invite +i\r\nINVITE frank #invite\r\nQUIT\r\n'

###############################################################################
# 6b. Frank joins invited channel
###############################################################################
run_test "invite_only_join"  "JOIN :#invite"           $'PASS '"$PASS"'\r\nNICK frank\r\nUSER frank 0 * :Frank\r\nJOIN #invite\r\nQUIT\r\n'

###############################################################################
# 7. TOPIC on +t channel
###############################################################################
run_test "topic"             "332 george #topic :New Topic" \
                                 $'PASS '"$PASS"'\r\nNICK george\r\nUSER george 0 * :George\r\nJOIN #topic\r\nMODE #topic +t\r\nTOPIC #topic :New Topic\r\nQUIT\r\n'

###############################################################################
# 8. LIST & WHO
###############################################################################
run_test "list_who"          "352 harry #42"           $'PASS '"$PASS"'\r\nNICK harry\r\nUSER harry 0 * :Harry\r\nLIST\r\nWHO #42\r\nQUIT\r\n'

###############################################################################
# 9. PING → PONG
###############################################################################
run_test "ping_pong"         "PONG .*:12345"           $'PASS '"$PASS"'\r\nNICK ivy\r\nUSER ivy 0 * :Ivy\r\nPING :12345\r\nQUIT\r\n'

###############################################################################
# 10. QUIT messaging
###############################################################################
run_test "quit"              ":jim!.* QUIT :bye"      $'PASS '"$PASS"'\r\nNICK jim\r\nUSER jim 0 * :Jim\r\nQUIT :bye\r\n'

###############################################################################
# Summary & Exit
###############################################################################
log "Tests complete. Failures: $failures"
exit $failures
