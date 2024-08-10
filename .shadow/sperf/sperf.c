#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

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
    exec_argv[1] = "-T";        // 获取每个系统调用的执行时间
    exec_argv[2] = "-o";        // 指定strace的输出文件，用于追踪程序的输出
    exec_argv[3] = "/dev/null"; // 将被追踪程序的输出重定向到/dev/null

    for (int i = 1; i < argc; i++)
    {
        exec_argv[i + 3] = argv[i];
    }
    exec_argv[argc] = NULL;
    exec_envp[0] = path_str;
    exec_envp[1] = NULL;

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

        // 打开/dev/null用于标准输出
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull == -1)
        {
            perror("open");
            exit(EXIT_FAILURE);
        }

        // 将stdout重定向到/dev/null
        if (dup2(devnull, STDOUT_FILENO) == -1)
        {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        // 将stderr重定向到管道的写端
        if (dup2(pipe_fds[1], STDERR_FILENO) == -1)
        {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        // 关闭不再需要的文件描述符
        close(devnull);
        close(pipe_fds[1]);

        execve("/usr/bin/strace", exec_argv, exec_envp);

        perror("execve");
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
            printf("%s", buffer);
        }

        // 关闭管道的读端
        close(pipe_fds[0]);

        // 等待子进程结束
        wait(NULL);

        // 父进程退出
        exit(EXIT_SUCCESS);
    }
}
