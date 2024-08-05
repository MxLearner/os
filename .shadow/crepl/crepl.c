#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <sys/wait.h>

#define MAX_FUNCS 200
#define MAX_FILES 1000 // 假设最多处理1000个文件
#define TEMPLATE "/tmp/creplXXXXXX"

char c_files[MAX_FILES][256];  // 用于存储所有.c文件的路径
char so_files[MAX_FILES][256]; // 用于存储所有.so文件的路径
int file_count = 0;            // 记录文件数量
char func_names[MAX_FILES][256];
void *func_ptrs[MAX_FILES];

void cleanup_files()
{
    for (int i = 0; i < file_count; ++i)
    {
        unlink(c_files[i]);  // 删除所有.c文件
        unlink(so_files[i]); // 删除所有.so文件
        printf("Deleted %s\n", c_files[i]);
        printf("Deleted %s\n", so_files[i]);
    }
}

void compile_function(const char *line)
{
    int fd;
    char fname[250];
    char c_fname[256]; // 用于存储新的临时文件名
    char soName[256];  // 用于存储动态库的完整路径

    snprintf(fname, sizeof(fname), TEMPLATE);
    fd = mkstemp(fname);
    if (fd == -1)
    {
        perror("Failed to create temporary file");
        return;
    }
    // 构造新的文件名，添加.c后缀
    snprintf(c_fname, sizeof(c_fname), "%s.c", fname);
    strcpy(c_files[file_count], c_fname); // 存储.c文件路径

    // 重命名文件
    if (rename(fname, c_fname) == -1)
    {
        perror("Failed to rename file");
        close(fd);
        unlink(fname); // 尝试清理原始临时文件
        return;
    }

    // 写入 C 代码到新的临时文件
    if (write(fd, line, strlen(line)) == -1)
    {
        perror("Failed to write to file");
        close(fd);
        unlink(c_fname); // 清理新的临时文件
        return;
    }

    close(fd);

    // 构建动态库的完整路径名
    snprintf(soName, sizeof(soName), "%s.so", fname);
    strcpy(so_files[file_count], soName); // 存储.so文件路径

    pid_t pid = fork();
    if (pid == -1)
    {
        perror("Failed to fork");
        return;
    }
    else if (pid == 0)
    {
        // 在子进程中编译源代码到动态库
        execlp("gcc", "gcc", "-shared", "-fPIC", c_fname, "-o", soName, NULL);
        perror("Failed to compile");
        _exit(1); // 确保子进程退出
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
        if (status != 0)
        {
            fprintf(stderr, "Compilation failed\n");
            unlink(soName); // 如果编译失败，也删除动态库文件
            return;
        }
    }
    printf("Compiled successfully to %s\n", soName);

    void *handle = dlopen(soName, RTLD_LAZY);
    if (!handle)
    {
        fprintf(stderr, "Failed to load %s: %s\n", soName, dlerror());
        return;
    }

    char func_name[256];
    sscanf(line, "int %s(", func_name);
    func_name[strlen(func_name) - 1] = '\0'; // 删除最后一个字符，即'('
    func_ptrs[file_count] = dlsym(handle, func_name);
    strcpy(func_names[file_count], func_name);
    file_count++;
}

void execute_expression(char *expr)
{
    char filename[] = "/tmp/crepl_expr_XXXXXX.c";
    int fd = mkstemps(filename, 2); // Create and open a temporary file with a ".c" extension
    if (fd == -1)
    {
        perror("Failed to create temporary file for expression");
        return;
    }

    FILE *f = fdopen(fd, "w");
    fprintf(f, "#include <stdio.h>\n");
    for (int i = 0; i < file_count; i++)
    {
        fprintf(f, "int %s();\n", func_names[i]);
    }
    fprintf(f, "int main() { printf(\"%%d\\n\", %s); return 0; }\n", expr);
    fclose(f);

    char outname[256];
    sprintf(outname, "/tmp/crepl_expr_%d", getpid());
    char command[512];
    sprintf(command, "gcc %s -o %s -ldl", filename, outname);
    system(command); // Compile the file

    sprintf(command, "%s", outname);
    system(command); // Execute the compiled program

    unlink(filename); // Cleanup
    unlink(outname);
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
            cleanup_files(); // 清理所有文件
            break;
        }

        if (strncmp(line, "int", 3) == 0)
        {
            compile_function(line);
        }
        else
        {
            execute_expression(line); // 执行其他表达式
        }
    }
    return 0;
}
