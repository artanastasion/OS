#ifndef BACKGROUND_H
#define BACKGROUND_H

#ifdef __cplusplus
extern "C" {
#endif

    int launch_background(const char* program, const char* const argv[]);

    int wait_for_process(int pid);

    int is_process_running(int pid);

#ifdef __cplusplus
}
#endif

#endif // BACKGROUND_H