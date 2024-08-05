#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/wait.h>

#define MAX_FUNCS 200
#define TEMPLATE "/tmp/creplXXXXXX"

int count = 0;

void complie_function(const char *line)
{
    int fd;
    char fname[256];
    snprintf(fname, sizeof(fname), TEMPLATE);
    fd = mkstemp(fname);
    if (fd == -1)
    {
        perror("Failed to create temporary file");
        return;
    }

    write(fd, line, strlen(line));
    close(fd);

    pid_t pid = fork();
    if (pid == -1)
    {
        perror("Failed to fork");
        return;
    }
    else if (pid == 0)
    {
        execlp("gcc", "gcc", "-shared", "-fPIC", fname, "-o", "tmp.so", NULL);
        perror("Failed to compile");
        _exit(1);
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
        if (status != 0)
        {
            fprintf(stderr, "Compilation failed\n");
            return;
        }
    }
    printf("Compiled successfully\n");
}

void execute_expression(const char *line)
{
}

int main(int argc, char *argv[])
{
    static char line[4096];

    while (1)
    {
        printf("crepl> ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin))
        {
            break;
        }

        if (strncmp(line, "int", 3) == 0)
        {
            complie_function(line);
        }
        else
        {
            execute_expression(line);
        }
    }
}
