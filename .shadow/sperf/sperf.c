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
    exec_argv[0] = "strace";
    exec_argv[1] = "ls";
    exec_argv[2] = NULL;
    exec_envp[0] = "PATH=/bin";
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
        close(pipefd[1]);
        int n = read(pipefd[0], buffer, sizeof(buffer));
        assert(n > 0);
        buffer[n] = '\0';
        printf("%s", buffer);
        close(pipefd[0]);
        wait(NULL);
    }

    return 0;
}
