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
    char buffer[1024];
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
        int n = dup2(pipefd[1], 2);
        if (n == -1)
        {
            perror("dup2");
            exit(1);
        }
        // close(pipefd[1]); // Close the duplicated file descriptor

        execve("/usr/bin/strace", exec_argv, exec_envp);
        perror("execve");
        exit(1);
    }
    else // Parent process
    {
        wait(NULL);       // Ensure child process resources are reclaimed
        close(pipefd[1]); // Close write end
        int n = read(pipefd[0], buffer, sizeof(buffer));
        write(STDOUT_FILENO, buffer, n);
        wait(NULL); // Ensure child process resources are reclaimed
    }

    return 0;
}
