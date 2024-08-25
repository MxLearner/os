#ifndef __COMMON_H__
#define __COMMON_H__

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define DEBUG
#include "debug.h"
#include "bmp.h"
#include "fat32.h"

#define FOLDER "./recov"

struct DataSeg
{
  void *head;           // 指向数据段起始位置的指针
  bool eof;             // 是否到达文件末尾的标志
  struct Image *holder; // 指向持有该数据段的图像的指针
  struct DataSeg *prev; // 指向上一个数据段的指针
  struct DataSeg *next; // 指向下一个数据段的指针
};

struct Image
{
  char name[128];     // 图像文件的名称
  char sha1[128];     // 图像的 SHA1 哈希值
  size_t size;        // 图像的大小
  int clus;           // 图像的起始簇号
  FILE *file;         // 文件指针，用于文件 I/O 操作
  struct BMP *bmp;    // 指向 BMP 图像结构体的指针
  struct Image *prev; // 指向上一个图像的指针
  struct Image *next; // 指向下一个图像的指针
};

enum ClusterTypes
{
  TYPE_FDT, // file entry
  TYPE_BMP, // bmp image
  TYPE_EMP  // empty entry
};

void recover_images();
int get_cluster_type(void *, int);
void handle_fdt(void *, int, bool);
bool handle_fdt_aux(void *, int, bool);
void handle_image(struct Image *, size_t, int);
void *get_next_cluster(uint8_t *);

#endif
