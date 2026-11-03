#!/bin/bash
# Benchmark: standalone (1 process) vs multi-process (UI + Core + Logger).
# Run from week2/benchmark:   bash run_bench.sh [commands_per_run] [runs]
# Needs week2 built first (cd .. && make). Writes results.md

N=${1:-100000}
RUNS=${2:-3}
HERE="$(cd "$(dirname "$0")" && pwd)"
cd "$HERE" || exit 1

CF="-Wall -Wextra -std=gnu99 -I.."
gcc $CF -o bench_standalone bench_standalone.c ../sync.c -pthread -lrt || exit 1
gcc $CF -o bench_multi      bench_multi.c      ../sync.c -pthread -lrt || exit 1
[ -x ../core/core ] && [ -x ../logger/logger ] || { echo "Build week2 first: cd .. && make"; exit 1; }

rm -f /tmp/bench_standalone.txt /tmp/bench_multi.txt
echo "Commands per run: $N   Runs: $RUNS"

for r in $(seq 1 $RUNS); do
    ./bench_standalone $N | grep '^BENCH' >> /tmp/bench_standalone.txt
done

for r in $(seq 1 $RUNS); do
    ( cd ../core && exec ./core > /tmp/core_bench.out 2>&1 ) &
    CORE_PID=$!
    sleep 1
    ./bench_multi $N $CORE_PID | grep '^BENCH' >> /tmp/bench_multi.txt
    wait $CORE_PID 2>/dev/null
done

# average of one key over all runs of a file
avg() { grep -oE " $2=[0-9.]+" "$1" | sed 's/.*=//' | awk '{s+=$1} END {printf "%.2f", s/NR}'; }

SA=/tmp/bench_standalone.txt; MP=/tmp/bench_multi.txt
SA_AVG=$(avg $SA avg_us);  MP_AVG=$(avg $MP avg_us)
SA_TOT=$(avg $SA total_ms); MP_TOT=$(avg $MP total_ms)
OVER=$(awk -v a=$MP_AVG -v b=$SA_AVG 'BEGIN{printf "%.2f", a-b}')
RATIO=$(awk -v a=$MP_AVG -v b=$SA_AVG 'BEGIN{printf "%.1f", a/b}')

{
echo "# Benchmark results"
echo
echo "Workload: $N commands per run (LOAD, ADD, PUSH, POP, STORE repeated), average of $RUNS runs."
echo
echo "| Metric | Standalone (1 process) | Multi-process (UI + Core + Logger) |"
echo "|---|---|---|"
echo "| Total time for $N commands (ms) | $SA_TOT | $MP_TOT |"
echo "| Average time per command (us) | $SA_AVG | $MP_AVG |"
echo "| Median (p50) per command (us) | $(avg $SA p50_us) | $(avg $MP p50_us) |"
echo "| p95 per command (us) | $(avg $SA p95_us) | $(avg $MP p95_us) |"
echo "| p99 per command (us) | $(avg $SA p99_us) | $(avg $MP p99_us) |"
echo "| Total CPU time (ms) | $(avg $SA cpu_ms) | $(avg $MP cpu_ms) |"
echo "| Peak memory, all processes (KB) | $(avg $SA rss_kb) | $(avg $MP rss_kb) |"
echo
echo "**IPC overhead per command:** $OVER us ($MP_AVG - $SA_AVG). The multi-process version is about ${RATIO}x slower per command."
echo
echo "## Multi-process breakdown (average)"
echo
echo "| Process | IPC | CPU time (ms) | Peak memory (KB) |"
echo "|---|---|---|---|"
echo "| UI (benchmark client) | Socket | $(avg $MP ui_cpu_ms) | $(avg $MP ui_kb) |"
echo "| Core | Shared memory + semaphores | $(avg $MP core_cpu_ms) | $(avg $MP core_kb) |"
echo "| Logger | Pipe | $(avg $MP logger_cpu_ms) | $(avg $MP logger_kb) |"
echo
echo "Correct results in every run: standalone=$(grep -c 'correct=1' $SA)/$RUNS, multi=$(grep -c 'correct=1' $MP)/$RUNS. Errors: $(avg $MP errors)."
} | tee results.md
