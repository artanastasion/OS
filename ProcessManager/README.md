# Лабораторная работа №2
**Кроссплатформенная библиотека** для запуска программ в фоновом режиме с возможностью ожидания завершения и получения
кода возврата. 

## Требования

- C++17 или новее
- CMake ≥ 3.10
- Windows (WinAPI) или UNIX-совместимая ОС (POSIX)

## Структура проекта

```
├── CMakeLists.txt          # Конфигурация сборки  
├── background.hpp          # Заголовочный файл библиотеки  
├── background.cpp          # Реализация для Windows/UNIX  
└── main.cpp                # Тестовая утилита  
```

## Сборка

### Linux/macOS

```bash 
mkdir build && cd build  
cmake ..  
make  
```

### Windows (MinGW)

```bash
mkdir build && cd build  
cmake -G "MinGW Makefiles" ..  
mingw32-make  
```

## API библиотеки

### Запуск процесса

```cpp 
int start_background(const char* program_path, int& status);  
```  

- `program_path` — путь к исполняемому файлу
- `status` — выходной параметр (0 при успешном запуске)
- **Возвращает**: PID дочернего процесса или -1 при ошибке

### Ожидание завершения

```cpp 
int wait_program(int pid, int* exit_code);  
```  

- `pid` — идентификатор процесса
- `exit_code` — указатель для сохранения кода возврата
- **Возвращает**: 0 при успехе, -1 при ошибке

## Проверка работы

Запустите тестовую утилиту:

```bash 
./background_test  
```  

**Ожидаемый вывод**:

``` 
Process started with PID: 12345  
Process is running...  
[вывод дочерней программы]  
Process exited with code: 0  
```

