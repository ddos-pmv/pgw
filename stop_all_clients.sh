#!/bin/bash

# Убить все процессы, имя которых pgw_client
pkill -f pgw_client

# Проверка
if pgrep -f pgw_client > /dev/null; then
    echo "Некоторые процессы pgw_client всё ещё работают"
    exit 1
else
    echo "Все процессы pgw_client завершены"
    exit 0
fi