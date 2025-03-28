#include "background.hpp"
#include <stdio.h>

int main() {
    const char *program =
#ifdef _WIN32
        "notepad.exe";
#else
            "/bin/ls";
#endif

    const char *args[] = {
#ifdef _WIN32
        program, NULL
#else
        program, "-l", "/", NULL
#endif
    };

    int pid = launch_background(program, args);
    if (pid == -1) {
        printf("Failed to launch process\n");
        return 1;
    }

    printf("Process started with PID: %d\n", pid);

    int running = is_process_running(pid);
    if (running == 1) {
        printf("Process is running...\n");
    } else if (running == -1) {
        printf("Error checking process status\n");
    }

    int exit_code = wait_for_process(pid);
    if (exit_code == -1) {
        printf("Failed to wait for process\n");
        return 1;
    }

    printf("Process exited with code: %d\n", exit_code);
    return 0;
}
