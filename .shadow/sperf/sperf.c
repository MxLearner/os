#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <assert.h>
#include <signal.h>

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
    if (pid == 0) // Child process
    {
        close(pipefd[0]);
        dup2(pipefd[1], 1);
        execve("/usr/bin/strace", exec_argv, exec_envp);
        perror("execve");
        exit(1);
    }
    else // Parent process
    {
        close(pipefd[1]); // 关闭父进程中的写端

        while (1)
        {
            int status;
            pid_t result = waitpid(pid, &status, WNOHANG); // Non-blocking wait
            if (result == 0)
            {
                usleep(100000); // 等待 100 毫秒
                // 子进程还在运行
                int n = read(pipefd[0], buffer, sizeof(buffer) - 1);
                if (n > 0)
                {
                    buffer[n] = '\0';
                    printf("%s", buffer);
                }
                        }
            else
            {
                // 子进程已结束
                break;
            }
        }

        close(pipefd[0]); // 关闭读端
        wait(NULL);       // 确保子进程资源被回收
    }

    return 0;
}
