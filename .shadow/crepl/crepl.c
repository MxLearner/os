#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/wait.h>

#define MAX_FUNCS 200
#define TEMPLATE "/tmp/creplXXXXXX"

int count = 0;

void compile_function(const char *line)
{
    int fd;
    char fname[250];
    char soName[256]; // 用于存储动态库的完整路径

    snprintf(fname, sizeof(fname), TEMPLATE);
    fd = mkstemp(fname);
    if (fd == -1)
    {
        perror("Failed to create temporary file");
        return;
    }

    // 写入 C 代码到临时文件
    if (write(fd, line, strlen(line)) == -1)
    {
        perror("Failed to write to file");
        close(fd);
        unlink(fname); // 清理临时文件
        return;
    }
    close(fd);

    // 构建动态库的完整路径名
    snprintf(soName, sizeof(soName), "%s.so", fname);

    pid_t pid = fork();
    if (pid == -1)
    {
        perror("Failed to fork");
        return;
    }
    else if (pid == 0)
    {
        // 在子进程中编译源代码到动态库
        execlp("gcc", "gcc", "-shared", "-fPIC", fname, "-o", soName, NULL);
        perror("Failed to compile");
        _exit(1); // 确保子进程退出
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
        unlink(fname); // 删除源代码文件
        if (status != 0)
        {
            fprintf(stderr, "Compilation failed\n");
            unlink(soName); // 如果编译失败，也删除动态库文件
            return;
        }
    }
    printf("Compiled successfully to %s\n", soName);
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
            compile_function(line);
        }
        else
        {
            execute_expression(line);
        }
    }
}
