#!/usr/bin/env bash
# run_lab3.sh — усі сценарії ЛР3 (методичка §6) у Wokwi через wokwi-cli, одна збірка прошивки.
#
#   WOKWI_CLI_TOKEN=... tools/run_lab3.sh            # усі сценарії
#   WOKWI_CLI_TOKEN=... tools/run_lab3.sh s1_idle    # лише вибрані
#
# Результат: docs/metrics/<сценарій>.log (вивід UART), docs/captures/lr3_btn_latency.vcd
# (логічний аналізатор, профіль lr3), docs/metrics/results.md + vcd_btn.md (таблиці).
set -uo pipefail
cd "$(dirname "$0")/.."

: "${WOKWI_CLI_TOKEN:?потрібен WOKWI_CLI_TOKEN (https://wokwi.com/dashboard/ci)}"
export WOKWI_CLI_TOKEN
export PATH="$HOME/.wokwi/bin:$PATH"
if ! command -v wokwi-cli >/dev/null; then
  echo "== встановлюю wokwi-cli"
  curl -sL https://wokwi.com/ci/install.sh | sh || { echo "!! не вдалося встановити wokwi-cli"; exit 1; }
fi
wokwi-cli --version 2>/dev/null | head -1

[ -f firmware/build/firmware.hex ] || { echo "!! немає firmware/build/firmware.hex — спершу make build"; exit 1; }
mkdir -p docs/metrics docs/captures
python3 tools/lr3_gen_scenarios.py >/dev/null

# FR-12: та сама схема без MPU-6050
python3 - <<'PY'
import json
d = json.load(open('diagram.json'))
d['parts'] = [p for p in d['parts'] if p['id'] != 'imu']
d['connections'] = [c for c in d['connections'] if not (c[0].startswith('imu:') or c[1].startswith('imu:'))]
json.dump(d, open('tests/sim/diagram_noimu.json', 'w'), indent=2)
# Для запису аналізатора: без SCL (D7) і TX (D5) — I²C на 100 кГц за 20 с дає 30 МБ VCD,
# і Wokwi обриває запис (ЛР3, перший прогін). Решта каналів — ті самі, що в профілі lr3.
d = json.load(open('diagram.json'))
d['connections'] = [c for c in d['connections'] if c[0] not in ('logic:D5', 'logic:D7')]
json.dump(d, open('tests/sim/diagram_vcd.json', 'w'), indent=2)
PY

ALL="s1_idle s2_nominal s3_worst longop event_longop uart_flood fault breakpoint fr02_static fr12_noimu boot_power vcd_btn long_run"
LIST="${*:-$ALL}"
declare -A RESULT
for sc in $LIST; do
  y="tests/sim/lr3_${sc}.yaml"
  [ -f "$y" ] || { echo "!! немає $y"; continue; }
  # таймаут (мс симуляції) = сума затримок сценарію × 2 + 60 с запасу
  t=$(python3 -c "import re;print(sum(int(x) for x in re.findall(r'delay: (\d+)ms',open('$y').read()))*2+60000)")
  extra=()
  [ "$sc" = fr12_noimu ] && extra+=(--diagram-file tests/sim/diagram_noimu.json)
  [ "$sc" = vcd_btn ] && extra+=(--diagram-file tests/sim/diagram_vcd.json --vcd-file docs/captures/lr3_btn_latency.vcd)
  echo "== $sc (timeout $((t/1000)) с симуляції) $(date +%H:%M:%S)"
  wokwi-cli --quiet --timeout "$t" --scenario "$y" --serial-log-file "docs/metrics/${sc}.log" \
            "${extra[@]}" . >/dev/null 2>"docs/metrics/${sc}.err"
  rc=$?
  RESULT[$sc]=$rc
  echo "   exit=$rc  рядків логу: $(wc -l < "docs/metrics/${sc}.log" 2>/dev/null || echo 0)"
done

python3 tools/lr3_metrics.py docs/metrics/*.log > docs/metrics/results.md
[ -f docs/captures/lr3_btn_latency.vcd ] && python3 tools/lr3_vcd.py docs/captures/lr3_btn_latency.vcd > docs/metrics/vcd_btn.md

echo
echo "== підсумок (0 — сценарій пройшов; 42 — таймаут, не дочекався рядка)"
for sc in $LIST; do printf '   %-14s %s\n' "$sc" "${RESULT[$sc]:-—}"; done
echo "Таблиці: docs/metrics/results.md, docs/metrics/vcd_btn.md"
