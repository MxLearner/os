#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <assert.h>

int main(int argc, char *argv[])
{
    char *exec_argv[256];
    char *exec_envp[256];

    char *path_env = getenv("PATH");
    if (path_env == NULL)
    {
        perror("getenv");
        exit(1);
    }
    char path_str[1024];
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
    if (pid == 0)
    {
        close(pipefd[0]);
        dup2(pipefd[1], 1);
        execve("/usr/bin/strace", exec_argv, exec_envp);
        perror("execve");
        exit(1);
    }
    else
    {
        close(pipefd[1]); // 关闭父进程中的写端

        while (1)
        {
            int n = read(pipefd[0], buffer, sizeof(buffer) - 1); // 留出一个字符位置给终结符
            if (n > 0)
            {
                buffer[n] = '\0'; // 确保字符串正确终结
                printf("%s", buffer);
            }
            else if (n == 0)
            {
                // 当没有数据可读时，read返回0，这意味着管道写端已经被所有持有者关闭
                break;
            }
            else
            {
                // 如果read返回-1，可能出现错误
                perror("read");
                break;
            }
        }

        close(pipefd[0]); // 关闭读端
        wait(NULL);       // 等待子进程结束
    }

    return 0;
}
