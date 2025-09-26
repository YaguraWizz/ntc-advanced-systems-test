#!/usr/bin/env bash
set -euo pipefail

DATA_DIR="task1_zmq/data"
EXPECTED="tests/task1_expected.txt"

# Возможные пути до бинарника
CANDIDATES=(
  "task1_zmq/build/bin/app"
  "task1_zmq/build/bin/app.exe"
  "task1_zmq/build/bin/Debug/app.exe"
  "task1_zmq/build/bin/Release/app.exe"
)

APP_BIN=""

for path in "${CANDIDATES[@]}"; do
  if [[ -f "$path" ]]; then
    APP_BIN="$path"
    break
  fi
done

if [[ -z "$APP_BIN" ]]; then
  echo "❌ Executable not found in expected locations."
  exit 1
fi

echo "▶ Using binary: $APP_BIN"

# Запуск сервера
"$APP_BIN" -d "$DATA_DIR" -u 127.0.0.1:5555 &
SERVER_PID=$!
sleep 1

# Запуск клиента (ограничиваемся только первым блоком с "Sorted Student List")
OUTPUT=$("$APP_BIN" -m client -u 127.0.0.1:5555 | awk '
  /Sorted Student List/ {capture=1}
  capture {print}
  /=======================================================/ && capture && ++count==2 {exit}
')

# Остановка сервера
kill $SERVER_PID || true

# Проверка результата
diff <(echo "$OUTPUT") "$EXPECTED"
if [[ $? -eq 0 ]]; then
  echo "✅ Test passed!"
else
  echo "❌ Test failed!"
  exit 1
fi