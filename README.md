# pgw
Protei Graduation Work

# Выжно
Чтобы работали скрипты, задайте путь до исполнямых файлов в stress_launch.sh и в client/main.cpp

## Описание проекта

Мини-PGW представляет собой упрощённую модель сетевого компонента Packet Gateway, способную обрабатывать UDP-запросы с IMSI, управлять сессиями абонентов, вести CDR-журнал и предоставлять HTTP API для мониторинга и управления.

## Архитектура

### Компоненты системы

1. **pgw_server** - основное серверное приложение
2. **pgw_client** - тестовый клиент
3. **imsi_generator** - утилита для генерации случайных IMSI

### Архитектурная схема

```
┌─────────────────┐    UDP/BCD    ┌──────────────────────┐
│   UDP Client    │─────────────→ │    UDP Server        │
│                 │               │                      │
└─────────────────┘               │  ┌─────────────────┐ │
                                  │  │ Event Dispatcher│ │
┌─────────────────┐    HTTP       │  └─────────────────┘ │
│   HTTP Client   │─────────────→ │         │            │
│  /check_subscriber              │         ▼            │
│  /stop          │               │  ┌─────────────────┐ │
└─────────────────┘               │  │ Session Manager │ │
                                  │  └─────────────────┘ │
                                  │         │            │
                                  │         ▼            │
                                  │  ┌─────────────────┐ │
                                  │  │   CDR Writer    │ │
                                  │  └─────────────────┘ │
                                  └──────────────────────┘
```

### Основные модули

#### 1. UDP Server
- Принимает UDP-пакеты с IMSI в BCD-кодировке
- Использует epoll для эффективной обработки соединений
- Отправляет ответы "created" или "rejected"

#### 2. Event Dispatcher
- Многопоточная обработка событий
- Декодирование IMSI из BCD
- Маршрутизация запросов в Session Manager

#### 3. Session Manager
- Управление жизненным циклом сессий
- Поддержка чёрного списка IMSI
- Автоматическое удаление по таймауту
- Graceful shutdown с настраиваемой скоростью

#### 4. CDR Writer
- Асинхронная запись Call Detail Records
- Ротация логов
- Потокобезопасная запись

#### 5. HTTP Server
- REST API для мониторинга
- Управление graceful shutdown
- Проверка статуса абонентов


## Сборка проекта

### Быстрый старт

```bash
# Клонирование репозитория
git clone <repository-url>
cd pgw

# Создание директории для сборки
mkdir build && cd build

# Конфигурация и сборка
cmake ..
make -j$(nproc)

```

### Детальная сборка

```bash
# Сборка с отладочной информацией
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# Сборка для продакшена
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# Сборка без тестов
cmake -DBUILD_TESTS=OFF ..
make -j$(nproc)
```

## Конфигурация

### Конфигурация сервера (config/pgw_server.json)

```json
{
    "udp_ip": "0.0.0.0",
    "udp_port": 9000,
    "udp_log_file": "pgw.log",
    "udp_log_level": "INFO",
    "http_port": 8080,
    "http_log_file": "pgw.log", 
    "http_log_level": "INFO",
    "session_timeout_sec": 30,
    "cdr_file": "cdr.log",
    "threads_count": 10,
    "shutdown_rate": 20,
    "blacklist": [
        "001010123456789",
        "001010000000001"
    ]
}
```

**Параметры конфигурации:**
- `udp_ip/udp_port` - адрес и порт UDP сервера
- `http_port` - порт HTTP API
- `session_timeout_sec` - таймаут сессии в секундах
- `threads_count` - количество потоков для обработки событий
- `shutdown_rate` - скорость graceful shutdown (сессий/сек)
- `blacklist` - список заблокированных IMSI

### Конфигурация клиента (config/pgw_client.json)

```json
{
    "server_ip": "127.0.0.1",
    "server_port": 9000,
    "log_file": "client.log",
    "log_level": "INFO",
    "response_timeout_ms": 1000
}
```

## Запуск

### Запуск сервера

```bash
# Из директории build
./src/pgw_server

# Сервер автоматически загружает конфигурацию из /config/pgw_server.json
```

### Запуск клиента

```bash
# Отправка одного запроса
./client/pgw_client <IMSI>

# Примеры:
./client/pgw_client 123456789012345

# Периодическая отправка (интервал в мс)
./client/pgw_client <IMSI> <interval_ms>

# Пример: отправка каждые 100ms
./client/pgw_client 123456789012345 100

# С кастомным таймаутом (мс)
./client/pgw_client <IMSI> <interval_ms> <timeout_ms>
./client/pgw_client 123456789012345 100 2000
```

## Нагрузочное тестирование

### Скрипт stress_launch.sh

```bash
# Запуск нагрузочного тестирования
./stress_launch.sh <количество_клиентов> <общий_RPS>

# Пример: 50 клиентов с общим RPS 1000
./stress_launch.sh 50 1000
```

### Остановка всех клиентов

```bash
./stop_all_clients.sh
```

### Анализ результатов

```bash
# Подсчёт среднего времени ответа
./count_avg_ans.sh
```

## Формат данных

### UDP-протокол

- **Запрос**: IMSI в BCD-кодировке (TS 29.274 §8.3)
- **Ответ**: ASCII строка "created" или "rejected"

### BCD-кодировка IMSI

Пример кодирования IMSI "123456789012345":
```
Исходный IMSI: 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
BCD bytes:     21 43 65 87 09 21 43 F5
```

### CDR-записи

Формат: `timestamp,IMSI,action`

```
2024-01-15 14:30:25.123,123456789012345,CREATED
2024-01-15 14:30:55.456,123456789012345,TIMEOUT
2024-01-15 14:31:10.789,987654321098765,REJECTED
```

**Типы действий:**
- `CREATED` - сессия создана
- `REJECTED` - запрос отклонён (чёрный список)
- `TIMEOUT` - сессия завершена по таймауту
- `GRACEFUL_SHUTDOWN` - сессия завершена при graceful shutdown

## Логирование

### Уровни логирования

- `TRACE` - детальная отладочная информация
- `DEBUG` - отладочная информация
- `INFO` - информационные сообщения
- `WARN` - предупреждения
- `ERROR` - ошибки
- `CRITICAL` - критические ошибки

### Компоненты логирования

- **UDP** - события UDP сервера
- **HTTP** - события HTTP API
- **Session** - упра