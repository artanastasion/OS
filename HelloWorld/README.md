## Инструкция по сборке

### Linux/macOS

```bash
chmod +x build.sh  # Даем права на выполнение
./build.sh         # Запускаем сборку
```

### Windows

```cmd
build.cmd  # Запускаем сборку
```

## Структура проекта

```
.
├── CMakeLists.txt    # Конфигурация CMake
├── build.sh          # Скрипт сборки для Linux/macOS
├── build.cmd         # Скрипт сборки для Windows
└── main.cpp          # Исходный код программы
```

## Работа с Git

Для обновления кода:

```bash
git pull
```

Для отправки изменений:

```bash
git add .
git commit -m "Описание изменений"
git push
```
