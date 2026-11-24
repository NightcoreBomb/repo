#!/usr/bin/env bash
# run_lab4.sh — вимірювання ЛР4 (методичка §7): ОБИДВІ прошивки тими самими сценаріями в один день.
#
#   WOKWI_CLI_TOKEN=... tools/run_lab4.sh              # усе (≈ 17 хв симуляції)
#   WOKWI_CLI_TOKEN=... tools/run_lab4.sh lab3 s1_idle # лише прошивка lab3, лише S1
#   WOKWI_CLI_TOKEN=... tools/run_lab4.sh os           # лише сценарії ОС
#
# Прошивки — зібрані бінарники з docs/metrics/fw/ (саме ті, що вимірювались; tools/fw_snapshot.sh):
#   lab3.hex        super loop, тег lab3 (джерело 126ed3cd)
#   lab4.hex        FreeRTOS, тік 1000 Гц                      (make)
#   lab4-ostest.hex + команди `os …` (OSTEST=1)                  (make OSTEST=1 BUILD_DIR=build-ostest)
#   lab4-tick100.hex тік 100 Гц                                  (make TICK_HZ=100 BUILD_DIR=build-tick100)
#   lab4-nocheck.hex OSTEST=1, configCHECK_FOR_STACK_OVERFLOW=0
#   lab4-heapsmall.hex configTOTAL_HEAP_SIZE=3000
# Кожен прогін — у тимчасовій теці-проєкті Wokwi (diagram.json + wokwi.toml на свій .hex).
# Результат: docs/metrics/lab3/*.log, docs/metrics/lab4/*.log, docs/metrics/os/*.log,
#            docs/captures/lr4_*.vcd, docs/comparison-data.md (tools/lr4_compare.py).
set -uo pipefail
cd "$(dirname "$0")/.."
ROOT=$PWD

: "${WOKWI_CLI_TOKEN:?потрібен WOKWI_CLI_TOKEN (https://wokwi.com/dashboard/ci)}"
export WOKWI_CLI_TOKEN
export PATH="$HOME/bin:$HOME/.wokwi/bin:$PATH"
command -v wokwi-cli >/dev/null || { curl -sL https://wokwi.com/ci/install.sh | sh || exit 1; }
wokwi-cli --version 2>/dev/null | head -1

FW=docs/metrics/fw
for f in lab3 lab4 lab4-ostest lab4-tick100 lab4-nocheck lab4-heapsmall; do
  [ -f "$FW/$f.hex" ] || { echo "!! немає $FW/$f.hex"; exit 1; }
done
mkdir -p docs/metrics/lab3 docs/metrics/lab4 docs/metrics/os docs/captures
python3 tools/lr3_gen_scenarios.py >/dev/null
python3 tools/lr4_gen_scenarios.py >/dev/null

python3 - <<'PY'
import json
d = json.load(open('diagram.json'))
d['parts'] = [p for p in d['parts'] if p['id'] != 'imu']
d['connections'] = [c for c in d['connections'] if not (c[0].startswith('imu:') or c[1].startswith('imu:'))]
json.dump(d, open('tests/sim/diagram_noimu.json', 'w'), indent=2)
d = json.load(open('diagram.json'))       # аналізатор без SCL (D7) і TX (D5) — див. run_lab3.sh
d['connections'] = [c for c in d['connections'] if c[0] not in ('logic:D5', 'logic:D7')]
json.dump(d, open('tests/sim/diagram_vcd.json', 'w'), indent=2)
PY

TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
declare -A RESULT
ORDER=()

# run <fw> <scenario-yaml> <log> [diagram] [vcd]
run() {
  local fw=$1 y=$2 log=$3 diag=${4:-diagram.json} vcd=${5:-}
  local p="$TMP/$fw"
  mkdir -p "$p"
  cp "$FW/$fw.hex" "$p/firmware.hex"
  printf "[wokwi]\nversion = 1\nfirmware = 'firmware.hex'\n" > "$p/wokwi.toml"
  cp "$diag" "$p/diagram.json"
  local t
  t=$(python3 -c "import re;print(sum(int(x) for x in re.findall(r'delay: (\d+)ms',open('$y').read()))*2+60000)")
  local extra=()
  [ -n "$vcd" ] && extra+=(--vcd-file "$ROOT/$vcd")
  echo "== $fw  $(basename "$y")  → $log  $(date +%H:%M:%S)"
  wokwi-cli --quiet --timeout "$t" --scenario "$ROOT/$y" --serial-log-file "$ROOT/$log" \
            "${extra[@]}" "$p" >/dev/null 2>"$ROOT/${log%.log}.err"
  local rc=$?
  RESULT["$fw:$(basename "$log")"]=$rc; ORDER+=("$fw:$(basename "$log")")
  echo "   exit=$rc  рядків: $(wc -l < "$log" 2>/dev/null || echo 0)"
}

BASE="s1_idle s2_nominal s3_worst longop event_longop uart_flood fault breakpoint fr02_static fr12_noimu boot_power vcd_btn long_run"
WHAT=${1:-all}; shift || true
ONLY="${*:-$BASE}"

baseline() {                       # $1 = lab3 | lab4
  for sc in $ONLY; do
    local diag=diagram.json vcd=
    [ "$sc" = fr12_noimu ] && diag=tests/sim/diagram_noimu.json
    [ "$sc" = vcd_btn ] && { diag=tests/sim/diagram_vcd.json; vcd=docs/captures/lr4_btn_$1.vcd; }
    run "$1" "tests/sim/lr3_$sc.yaml" "docs/metrics/$1/$sc.log" "$diag" "$vcd"
  done
}

os_suite() {
  run lab4           tests/sim/lr4_s3_os.yaml     docs/metrics/os/s3_os.log
  run lab4           tests/sim/lr4_vcd_trace.yaml docs/metrics/os/vcd_trace.log tests/sim/diagram_vcd.json docs/captures/lr4_tasks.vcd
  run lab4-ostest    tests/sim/lr4_os.yaml        docs/metrics/os/os_tests.log
  run lab4-tick100   tests/sim/lr3_s3_worst.yaml  docs/metrics/os/s3_tick100.log
  run lab4-nocheck   tests/sim/lr4_nocheck.yaml   docs/metrics/os/nocheck.log
  run lab4-heapsmall tests/sim/lr4_heapsmall.yaml docs/metrics/os/heapsmall.log
}

case "$WHAT" in
  all)  baseline lab3; baseline lab4; os_suite ;;
  lab3) baseline lab3 ;;
  lab4) baseline lab4 ;;
  os)   os_suite ;;
  *)    echo "використання: $0 [all|lab3|lab4|os] [сценарії…]"; exit 2 ;;
esac

python3 tools/lr4_compare.py > docs/comparison-data.md
for d in lab3 lab4; do
  ls docs/metrics/$d/*.log >/dev/null 2>&1 && python3 tools/lr3_metrics.py docs/metrics/$d/*.log > docs/metrics/$d/results.md
done
for d in lab3 lab4; do
  [ -f docs/captures/lr4_btn_$d.vcd ] && python3 tools/lr3_vcd.py docs/captures/lr4_btn_$d.vcd > docs/metrics/$d/vcd_btn.md
done

echo; echo "== підсумок (0 — пройшов; 42 — таймаут wait-serial)"
for k in "${ORDER[@]}"; do printf '   %-32s %s\n' "$k" "${RESULT[$k]}"; done
echo "Порівняння: docs/comparison-data.md"
