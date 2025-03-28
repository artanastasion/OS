#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <string>
#include <atomic>
#include <mutex>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/file.h>
#endif

std::atomic<int> counter(0);
std::mutex log_mutex;
std::atomic<bool> running(true);
bool is_main_process = true;
int lockfile_fd = -1;

void write_log(const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex);
    std::ofstream logfile("counter.log", std::ios::app);
    if (logfile) {
        logfile << message << std::endl;
    }
}

std::string get_current_time() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    auto timer = std::chrono::system_clock::to_time_t(now);
    std::tm bt = *std::localtime(&timer);

    char buf[80];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &bt);
    sprintf(buf + strlen(buf), ".%03d", (int)ms.count());

    return buf;
}

void process_task(int type) {
    int pid = getpid();
    std::string time = get_current_time();
    write_log("Process " + std::to_string(pid) + " started at " + time + " (type " + std::to_string(type) + ")");

    if (type == 1) {
        counter.fetch_add(10);
    } else if (type == 2) {
        int current = counter.load();
        counter.store(current * 2);
        std::this_thread::sleep_for(std::chrono::seconds(2));
        current = counter.load();
        counter.store(current / 2);
    }

    write_log("Process " + std::to_string(pid) + " exited at " + get_current_time());
}

void timer_thread() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        counter++;
    }
}

void logging_thread() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Только главный процесс пишет в лог
        if (is_main_process) {
            write_log(get_current_time() + " - PID: " + std::to_string(getpid()) +
                     ", Counter: " + std::to_string(counter.load()));
        }
    }
}

void spawner_thread() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(3));

        // Только главный процесс создает дочерние
        if (!is_main_process) continue;

        static bool process1_running = false;
        static bool process2_running = false;

        if (!process1_running) {
            process1_running = true;
#ifdef _WIN32
            if (_spawnl(_P_NOWAIT, "counter.exe", "counter.exe", "child1", NULL) == -1)
#else
            if (fork() == 0) {
                execl("./counter", "./counter", "child1", NULL);
                exit(0);
            }
#endif
            process1_running = false;
        } else {
            write_log("Process 1 still running - skipping spawn");
        }

        if (!process2_running) {
            process2_running = true;
#ifdef _WIN32
            if (_spawnl(_P_NOWAIT, "counter.exe", "counter.exe", "child2", NULL) == -1)
#else
            if (fork() == 0) {
                execl("./counter", "./counter", "child2", NULL);
                exit(0);
            }
#endif
            process2_running = false;
        } else {
            write_log("Process 2 still running - skipping spawn");
        }
    }
}

bool acquire_lock() {
#ifdef _WIN32
    HANDLE h = CreateFile("counter.lock", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return false;
    CloseHandle(h);
    return true;
#else
    lockfile_fd = open("counter.lock", O_CREAT | O_RDWR, 0666);
    if (lockfile_fd == -1) return false;
    if (flock(lockfile_fd, LOCK_EX | LOCK_NB) == -1) {
        close(lockfile_fd);
        return false;
    }
    return true;
#endif
}

void release_lock() {
#ifdef _WIN32
    DeleteFile("counter.lock");
#else
    if (lockfile_fd != -1) {
        flock(lockfile_fd, LOCK_UN);
        close(lockfile_fd);
        unlink("counter.lock");
    }
#endif
}

int main(int argc, char* argv[]) {
    // Проверка типа процесса
    if (argc > 1) {
        is_main_process = false;
        if (strcmp(argv[1], "child1") == 0) {
            process_task(1);
        } else if (strcmp(argv[1], "child2") == 0) {
            process_task(2);
        }
        return 0;
    }

    // Определяем главный процесс через блокировку файла
    is_main_process = acquire_lock();
    if (!is_main_process) {
        std::cout << "Running as secondary instance" << std::endl;
    }

    // Инициализация лога
    write_log("Process started - PID: " + std::to_string(getpid()) +
              " at " + get_current_time() +
              (is_main_process ? " (MAIN)" : " (SECONDARY)"));

    // Запуск потоков
    std::thread timer(timer_thread);
    std::thread logger(logging_thread);
    std::thread spawner(spawner_thread);

    // Интерфейс командной строки
    std::string input;
    while (running) {
        std::cout << "Current counter: " << counter << std::endl;
        std::cout << "Enter new value or 'q' to quit: ";
        std::getline(std::cin, input);

        if (input == "q") {
            running = false;
            break;
        }

        try {
            int new_value = std::stoi(input);
            counter = new_value;
        } catch (...) {
            std::cout << "Invalid input!" << std::endl;
        }
    }

    // Остановка потоков
    running = false;
    timer.join();
    logger.join();
    spawner.join();

    // Освобождение блокировки
    if (is_main_process) {
        release_lock();
    }

    return 0;
}