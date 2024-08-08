#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <assert.h>
#include <signal.h>
#include <sys/time.h>
#include <fcntl.h> // Required for open()

int main(int argc, char *argv[])
{
    signal(SIGPIPE, SIG_IGN);

    char *exec_argv[256];
    char *exec_envp[256];

    char *path_env = getenv("PATH");
    if (path_env == NULL)
    {
        perror("getenv");
        exit(1);
    }
    char path_str[10240];
    snprintf(path_str, sizeof(path_str), "PATH=%s", path_env);

    exec_argv[0] = "strace";

    for (int i = 1; i < argc; i++)
    {
        exec_argv[i] = argv[i];
    }
    exec_argv[argc] = NULL;
    exec_envp[0] = path_str;
    exec_envp[1] = NULL;

    pid_t pid = fork();
    int pipefd[2];
    char buffer[10240];
    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        exit(1);
    }

    if (pid == -1)
    {
        perror("fork");
        exit(1);
    }

    struct timeval last_time, current_time;
    gettimeofday(&last_time, NULL);

    if (pid == 0) // Child process
    {
        close(pipefd[0]); // Close read end

        // Redirect standard output to /dev/null
        int dev_null_fd = open("/dev/null", O_WRONLY);
        if (dev_null_fd == -1)
        {
            perror("open /dev/null");
            exit(1);
        }
        dup2(dev_null_fd, 1); // Redirect stdout to /dev/null

        // Redirect standard error to pipe
        dup2(pipefd[1], 2);
        close(pipefd[1]); // Close the duplicated file descriptor

        execve("/usr/bin/strace", exec_argv, exec_envp);
        perror("execve");
        exit(1);
    }
    else // Parent process
    {
        close(pipefd[1]); // Close write end

        while (1)
        {
            int status;
            pid_t result = waitpid(pid, &status, WNOHANG); // Non-blocking wait
            if (result == 0)
            {
                usleep(100000); // Wait 100 milliseconds
                gettimeofday(&current_time, NULL);
                long elapsed = (current_time.tv_sec - last_time.tv_sec) * 1000000L + (current_time.tv_usec - last_time.tv_usec);
                if (elapsed >= 100000) // 100 milliseconds
                {
                    int n = read(pipefd[0], buffer, sizeof(buffer) - 1);
                    if (n > 0)
                    {
                        buffer[n] = '\0';
                        printf("%s", buffer);
                        fflush(stdout);
                        last_time = current_time;
                    }
                }
            }
            else
            {
                // Child process has terminated
                int n = 0;
                do
                {
                    n = read(pipefd[0], buffer, sizeof(buffer) - 1);
                    if (n > 0)
                    {
                        buffer[n] = '\0';
                        printf("%s", buffer);
                    }
                } while (n > 0);
                break;
            }
        }

        close(pipefd[0]); // Close read end
        fflush(stdout);
        wait(NULL); // Ensure child process resources are reclaimed
    }

    return 0;
}
