#!/bin/bash

LOG_FILE="client.log"

# Проверка на существование файла
if [[ ! -f "$LOG_FILE" ]]; then
    echo "Файл $LOG_FILE не найден."
    exit 1
fi

# Извлекаем значения avg_response, суммируем и считаем среднее
awk '
    /avg_response=/ {
        # Разделим строку по символу =
        split($0, parts, "avg_response=");
        split(parts[2], parts2, "ms");
        if (parts2[1] != "") {
            sum += parts2[1];
            count++;
        }
    }
    END {
        if (count > 0) {
            printf "Средний avg_response: %.3f ms\n", sum / count;
        } else {
            print "Нет значений avg_response в логе.";
        }
    }
' "$LOG_FILE"
