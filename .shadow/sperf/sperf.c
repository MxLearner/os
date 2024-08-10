#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    char *exec_argv[256];
    char *exec_envp[2560];

    char *path_env = getenv("PATH");
    if (path_env == NULL)
    {
        perror("getenv");
        exit(1);
    }
    snprintf(exec_envp, sizeof(exec_envp), "PATH=%s", path_env);

    exec_argv[0] = "strace";

    int pipe_fds[2];
    pid_t pid;

    // 创建管道
    if (pipe(pipe_fds) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // 创建子进程
    pid = fork();
    if (pid == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {
        // 子进程

        // 关闭管道的读端
        close(pipe_fds[0]);

        // 将stdout和stderr都重定向到管道的写端
        if (dup2(pipe_fds[1], STDOUT_FILENO) == -1 || dup2(pipe_fds[1], STDERR_FILENO) == -1)
        {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        // 关闭原始的写端描述符
        close(pipe_fds[1]);

        execve("strace", exec_argv, exec_envp);

        // 如果execlp返回，说明执行失败
        perror("execlp");
        exit(EXIT_FAILURE);
    }
    else
    {
        // 父进程

        // 关闭管道的写端
        close(pipe_fds[1]);

        // 从管道读取数据
        char buffer[1024];
        int nbytes;
        while ((nbytes = read(pipe_fds[0], buffer, sizeof(buffer) - 1)) > 0)
        {
            buffer[nbytes] = '\0';
            printf("hhh\n%s", buffer);
        }

        // 关闭管道的读端
        close(pipe_fds[0]);

        // 等待子进程结束
        wait(NULL);

        // 父进程退出
        exit(EXIT_SUCCESS);
    }
}
