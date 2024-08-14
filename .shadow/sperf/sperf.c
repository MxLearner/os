#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <regex.h>

#define MAX_CALLS 1000

typedef struct syscall_info
{
    char name[100];
    double total_time;
    int count;
} syscall_info;

int compare(const void *a, const void *b)
{
    syscall_info *callA = (syscall_info *)a;
    syscall_info *callB = (syscall_info *)b;
    return callB->total_time - callA->total_time;
}

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
    exec_argv[1] = "-T"; // 获取每个系统调用的执行时间
    // exec_argv[2] = "-o";        // 指定strace的输出文件，用于追踪程序的输出
    // exec_argv[3] = "/dev/null"; // 将被追踪程序的输出重定向到/dev/null

    for (int i = 1; i < argc; i++)
    {
        exec_argv[i + 1] = argv[i];
    }
    exec_argv[argc + 1] = NULL;
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

        syscall_info calls[MAX_CALLS];
        int num_calls = 0;
        char *line, *contest;
        while ((nbytes = read(pipe_fds[0], buffer, sizeof(buffer) - 1)) > 0)
        {
            buffer[nbytes] = '\0';
            line = strtok_r(buffer, "\n", &contest);
            while (line != NULL)
            {
                char *end_time = strrchr(line, '<');
                if (end_time)
                {
                    *end_time = '\0';
                    double time_taken = atof(end_time + 1);
                    char *syscall_name = line;
                    char *space_pos = strchr(syscall_name, '(');
                    if (space_pos)
                    {
                        *space_pos = '\0';
                        int found = 0;
                        for (int i = 0; i < num_calls; i++)
                        {
                            if (strcmp(calls[i].name, syscall_name) == 0)
                            {
                                calls[i].total_time += time_taken;
                                calls[i].count++;
                                found = 1;
                                break;
                            }
                        }
                        if (!found && num_calls < MAX_CALLS)
                        {
                            strcpy(calls[num_calls].name, syscall_name);
                            calls[num_calls].total_time = time_taken;
                            calls[num_calls].count = 1;
                            num_calls++;
                        }
                    }
                }
                line = strtok_r(NULL, "\n", &contest);
            }
        }

        qsort(calls, num_calls, sizeof(syscall_info), compare);

        printf("Top 5 longest system calls:\n");
        for (int i = 0; i < 5 && i < num_calls; i++)
        {
            printf("%s: %f seconds (called %d times)\n", calls[i].name, calls[i].total_time, calls[i].count);
        }

        // 关闭管道的读端
        close(pipe_fds[0]);

        // 等待子进程结束
        wait(NULL);

        // 父进程退出
        exit(EXIT_SUCCESS);
    }
}
