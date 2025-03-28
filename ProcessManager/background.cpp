#include "background.hpp"
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#endif

int launch_background(const char* program, const char* const argv[]) {
#ifdef _WIN32
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    char cmd_line[32768] = {0};
    strcpy(cmd_line, program);

    for (int i = 1; argv && argv[i]; i++) {
        strcat(cmd_line, " ");
        strcat(cmd_line, argv[i]);
    }

    if (!CreateProcess(NULL, cmd_line, NULL, NULL, FALSE,
                       CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        return -1;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return (int)pi.dwProcessId;
#else
    pid_t pid = fork();
    if (pid == 0) {
        execvp(program, (char* const*)argv);
        _exit(127);
    }
    return pid > 0 ? (int)pid : -1;
#endif
}

int wait_for_process(int pid) {
#ifdef _WIN32
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | SYNCHRONIZE, FALSE, (DWORD)pid);
    if (!hProcess) return -1;

    WaitForSingleObject(hProcess, INFINITE);

    DWORD exit_code;
    if (!GetExitCodeProcess(hProcess, &exit_code)) {
        CloseHandle(hProcess);
        return -1;
    }

    CloseHandle(hProcess);
    return (int)exit_code;
#else
    int status;
    if (waitpid((pid_t)pid, &status, 0) == -1) return -1;
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
#endif
}

int is_process_running(int pid) {
#ifdef _WIN32
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, (DWORD)pid);
    if (!hProcess) return -1;

    DWORD exit_code;
    if (!GetExitCodeProcess(hProcess, &exit_code)) {
        CloseHandle(hProcess);
        return -1;
    }

    CloseHandle(hProcess);
    return exit_code == STILL_ACTIVE ? 1 : 0;
#else
    int status;
    int result = waitpid((pid_t)pid, &status, WNOHANG);

    if (result == -1) return -1;
    if (result == 0) return 1;
    return 0;
#endif
}