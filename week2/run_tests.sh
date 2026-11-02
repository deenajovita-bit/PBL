#!/bin/bash
# Integration tests for the 3-process simulator.
# Run from the week2_multiprocess folder after `make`:   bash run_tests.sh
# Needs: core/core  UI/ui  logger/logger

ROOT="$(cd "$(dirname "$0")" && pwd)"
CORE="$ROOT/core/core"
UI="$ROOT/UI/ui"
PASS=0; FAIL=0

leftovers() { ls /tmp/sim_core.sock /dev/shm/sim_state /dev/shm/sem.sim_state_lock /dev/shm/sem.sim_core_ready 2>/dev/null; }

check() {   # check "name" condition-result(0=ok)
    if [ "$2" -eq 0 ]; then echo "PASS  $1"; PASS=$((PASS+1)); else echo "FAIL  $1"; FAIL=$((FAIL+1)); fi
}

start_core() { ( cd "$ROOT/core" && rm -f simulator.log && exec ./core > /tmp/core_test.out 2>&1 ) & CORE_PID=$!; sleep 1; }
stop_core()  { kill $CORE_PID 2>/dev/null; wait $CORE_PID 2>/dev/null; }
run_ui()     { printf "$1" | timeout 20 "$UI" 2>&1; }

for b in "$CORE" "$UI" "$ROOT/logger/logger"; do
    [ -x "$b" ] || { echo "Missing $b - run make first."; exit 1; }
done
rm -f /tmp/sim_core.sock; rm -f /dev/shm/sim_state /dev/shm/sem.sim_* 2>/dev/null

# 1. UI started before Core: clear message, no hang
OUT=$(timeout 5 "$UI" 2>&1 </dev/null); RC=$?
echo "$OUT" | grep -q "Core process is running"; A=$?
[ $RC -ne 0 ] && [ $RC -ne 124 ]; B=$?
check "1  UI before Core: clear message, exits (no hang)" $((A+B))

# 2. Valid commands
start_core
OUT=$(run_ui 'LOAD 10\nADD 5\nPUSH 7\nSTORE 3\nEXIT\n')
echo "$OUT" | grep -c "SUCCESS" | grep -q "^5$"; check "2  Valid commands (LOAD ADD PUSH STORE EXIT) all succeed" $?
stop_core

# 3. Wrong command: error, no crash. 4. STATE shows right values
start_core
OUT=$(run_ui 'LOAD 10\nADD 5\nFOO\nPUSH 7\nSTATE\nEXIT\n')
echo "$OUT" | grep -q "Invalid command"; check "3  Wrong command gives error, Core keeps running" $?
echo "$OUT" | grep -q "ACC = 15" && echo "$OUT" | grep -q "Stack : size=1"; check "4  STATE shows right values from shared memory" $?
stop_core

# 5. Many commands quickly
start_core
CMDS='LOAD 0\n'; for i in $(seq 1 1000); do CMDS+='ADD 1\n'; done; CMDS+='STATE\nEXIT\n'
OUT=$(run_ui "$CMDS")
echo "$OUT" | grep -q "ACC = 1000"; check "5  1000 quick commands: no lost or broken data (ACC=1000)" $?
stop_core

# 6. Log file has all executed commands
start_core
run_ui 'LOAD 10\nADD 5\nPUSH 7\nEXIT\n' >/dev/null; sleep 1
N=$(grep -c "EXECUTION" "$ROOT/core/simulator.log")
[ "$N" -eq 3 ]; check "6  Logger wrote 3 EXECUTION lines to simulator.log (got $N)" $?
grep -q "Pipe closed by Core" "$ROOT/core/simulator.log"; check "7  Logger stopped when Core closed the pipe" $?
stop_core

# 8. Clean exit: nothing left behind after EXIT
start_core; run_ui 'LOAD 1\nEXIT\n' >/dev/null; sleep 1; stop_core
[ -z "$(leftovers)" ]; check "8  After EXIT: no leftover socket / shared memory / semaphores" $?
pgrep -x logger >/dev/null; [ $? -ne 0 ]; check "9  After EXIT: Logger process is gone" $?

# 10. UI killed suddenly: Core stops cleanly
start_core
( sleep 30 | "$UI" >/dev/null 2>&1 ) & SUB=$!
sleep 1; pkill -9 -x ui; sleep 1.5
kill -0 $CORE_PID 2>/dev/null; [ $? -ne 0 ]; check "10 UI killed suddenly: Core notices and stops" $?
[ -z "$(leftovers)" ]; check "11 UI killed suddenly: nothing left behind" $?
kill $SUB 2>/dev/null; stop_core; rm -f /tmp/sim_core.sock

echo "-----------------------------------"
echo "Passed: $PASS   Failed: $FAIL"
[ $FAIL -eq 0 ]
