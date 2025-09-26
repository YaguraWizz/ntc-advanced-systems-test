#!/usr/bin/env bash
set -euo pipefail

DATA_FILE="task2_http/data/input.txt"
EXPECTED="tests/task2_expected.json"

# Возможные пути до бинарника
CANDIDATES=(
  "task2_http/build/bin/app"
  "task2_http/build/bin/app.exe"
  "task2_http/build/bin/Debug/app.exe"
  "task2_http/build/bin/Release/app.exe"
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

# Запуск сервера (только URL/порт)
"$APP_BIN" -u 127.0.0.1:5556 &
SERVER_PID=$!
sleep 1

# Отправка текста в сервер и получение ответа
OUTPUT=$(curl -s -X POST --data-binary "@$DATA_FILE" http://127.0.0.1:5556/analyze)

# Остановка сервера
kill $SERVER_PID || true

# Сравнение ответа с эталоном (через jq для нормализации JSON)
diff <(echo "$OUTPUT" | jq -S .) <(jq -S . "$EXPECTED")
if [[ $? -eq 0 ]]; then
  echo "✅ Test passed!"
else
  echo "❌ Test failed!"
  exit 1
fi