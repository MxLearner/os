#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <stdlib.h>

int is_numeric(const char *str);
void list_proc_dirs(int proc, int p, int n, int n_layer);
void read_process_info(char *pid, char *name, char *ppid);

int main(int argc, char *argv[])
{
  int p = 0, n = 0, v = 0;
  for (int i = 1; i < argc; i++)
  {
    assert(argv[i]);
    if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--show-pids") == 0)
    {
      p = 1;
    }
    else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--numeric-sort") == 0)
    {
      n = 1;
    }
    else if (strcmp(argv[i], "-V") == 0 || strcmp(argv[i], "--version") == 0)
    {
      v = 1;
    }
    else
    {
      fprintf(stderr, "Usage: %s [-p] [-n] [-V]\n", argv[0]);
      return 1;
    }
  }
  assert(!argv[argc]);
  if (v)
  {
    fprintf(stderr, "pstree made by qiyi Wang\n");
    return 0;
  }
  list_proc_dirs(1, p, n, 0);
  return 0;
}

int is_numeric(const char *str)
{
  while (*str)
  {
    if (!isdigit(*str))
      return 0;
    str++;
  }
  return 1;
}

void list_proc_dirs(int proc, int p, int n, int n_layer)
{
  struct dirent *entry;
  DIR *dp;
  char proc_name[256];
  int child_cout = 0;
  int child_proc[256];

  dp = opendir("/proc");
  if (dp == NULL)
  {
    perror("Failed to open /proc directory");
    return;
  }

  while ((entry = readdir(dp)) != NULL)
  {
    if (entry->d_type == DT_DIR && is_numeric(entry->d_name))
    {
      char name[256];
      char ppid[256];
      read_process_info(entry->d_name, name, ppid);
      if (proc == atoi(entry->d_name))
      {
        strcpy(proc_name, name);
        if (p)
        {
          printf("%s[%s]\n", proc_name, entry->d_name);
        }
        else
        {
          printf("%s\n", proc_name);
        }
      }
      if (proc == atoi(ppid))
      {
        child_cout++;
        child_proc[child_cout] = atoi(entry->d_name);
      }
    }
  }
  if (child_cout > 0)
  {
    if (n)
    {
      for (int i = 1; i < child_cout; i++)
      {
        for (int j = i + 1; j <= child_cout; j++)
        {
          if (child_proc[i] > child_proc[j])
          {
            int temp = child_proc[i];
            child_proc[i] = child_proc[j];
            child_proc[j] = temp;
          }
        }
      }
    }
    for (int i = 1; i <= child_cout; i++)
    {
      for (int j = 0; j < n_layer; j++)
      {
        printf("|  ");
      }
      printf("——");
      list_proc_dirs(child_proc[i], p, n, n_layer + 1);
    }
  }

  closedir(dp);
}

void read_process_info(char *pid, char *name, char *ppid)
{
  char path[256];
  FILE *file;
  char line[256];
  snprintf(path, sizeof(path), "/proc/%s/status", pid);
  file = fopen(path, "r");
  if (file == NULL)
  {
    return;
  }
  while (fgets(line, sizeof(line), file) != NULL)
  {
    if (strncmp(line, "Name:", 5) == 0)
    {
      sscanf(line, "Name:\t%s", name);
    }
    else if (strncmp(line, "PPid:", 5) == 0)
    {
      sscanf(line, "PPid:\t%s", ppid);
    }
  }
  fclose(file);
}
