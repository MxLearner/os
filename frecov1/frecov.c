#include "frecov.h"

bool saving_files = false;
static struct Disk *disk;
static struct DataSeg fdt_list;
static struct Image image_list;

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    printf("Usage: frecov FILE [--save]\n");
    printf("Check your input and try again.\n");
    exit(EXIT_FAILURE);
  }
  else
  {
    saving_files = argc > 2 && !strcmp(argv[2], "--save");

    disk = disk_load_fat(argv[1]);
    // 是否将图片保存到文件夹中
    if (saving_files)
    {
      struct stat st = {};
      if (stat(FOLDER, &st) == -1)
      {
        mkdir(FOLDER, 0700);
      }
      Assert(stat(FOLDER, &st) != -1, "create recovery folder failed.");
    }

    recover_images();
  }
  return 0;
}

struct Disk *disk_load_fat(const char *file)
{
  int fd = 0;
  struct stat sb = {};
  struct Disk *ret = malloc(sizeof(struct Disk));
  // 加载文件镜像
  fd = open(file, O_RDONLY);
  Assert(fd, "failed to open file");
  Assert(!fstat(fd, &sb), "fstat failed");

  ret->head = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  ret->tail = ret->head + (size_t)sb.st_size;
  Assert(ret->head != MAP_FAILED, "mmap failed");

  disk_get_sections(ret);
  return ret;
}

void disk_get_sections(struct Disk *disk)
{
  // 获取 boot sector，共 512 字节
  disk->mbr = (struct MBR *)disk->head;
  Assert(disk->mbr->SignatureWord == 0xaa55, "Expecting signature 0xaa55, got 0x%x", disk->mbr->SignatureWord);
  // 获取 FSInfo
  disk->fsinfo = (struct FSInfo *)((void *)disk->mbr + 512);
  // 计算从磁盘开始到第一个FAT表的偏移量
  size_t offst = (size_t)disk->mbr->BPB_BytsPerSec * disk->mbr->BPB_RsvdSecCnt;
  // 计算单个FAT表的大小
  size_t fatsz = (size_t)disk->mbr->BPB_BytsPerSec * disk->mbr->BPB_FATSz32;
  // 两个FAT表的地址
  disk->fat[0] = (struct FAT *)(((void *)disk->head) + offst);
  disk->fat[1] = (struct FAT *)(((void *)disk->head) + offst + fatsz * (disk->mbr->BPB_NumFATs - 1));
  // 计算数据区的偏移量
  offst += (size_t)fatsz * disk->mbr->BPB_NumFATs;
  offst += (size_t)disk->mbr->BPB_BytsPerSec * (disk->mbr->BPB_RootClus - 2) * disk->mbr->BPB_SecPerClus;
  disk->data = (((void *)disk->head) + offst);
}

unsigned char check_sum(unsigned char *c)
{
  unsigned char sum = 0;
  for (int i = 0; i < 11; ++i)
  {
    sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + c[i];
  }
  return sum;
}

void recover_images()
{
  fdt_list.prev = &fdt_list;
  fdt_list.next = &fdt_list;
  image_list.prev = &image_list;
  image_list.next = &image_list;

  // 计算每个簇的字节大小
  size_t clusz = (size_t)disk->mbr->BPB_BytsPerSec * disk->mbr->BPB_SecPerClus;
  // 计算每个簇的文件数
  int nr_clu = clusz / 32;
  // 从数据区的开始遍历每一个簇
  for (void *p = disk->data; p < disk->tail; p += clusz)
  {
    switch (get_cluster_type(p, nr_clu))
    {
    case TYPE_FDT:
      handle_fdt(p, nr_clu, false);
      break;
    case TYPE_BMP:
    default:
      break;
    }
  }
  handle_fdt(NULL, nr_clu, false);
  handle_fdt(NULL, nr_clu, true);

  for (struct Image *image = image_list.next; image != &image_list; image = image->next)
  {
    handle_image(image, clusz, nr_clu);
  }
}

const char empty_entry[32] = {};
int get_cluster_type(void *c, int nr)
{
  // 比较簇的前 32 字节
  if (!memcmp(c, empty_entry, 32))
    return TYPE_EMP;

  struct FDT *f = (struct FDT *)c;
  bool has_long_name = false;
  unsigned char chk_sum = 0;
  for (int i = 0; i < nr; ++i)
  {
    if (i && !f[i].state && !f[i].attr) // 目录项为空 并且不是第一个条目
      break;
    if (f[i].state == 0xe5) // 表示该目录项已被删除
      continue;
    if (f[i].attr == ATTR_DIRECTORY && !f[i].file_size) // 表示该目录项是一个子目录 并且文件大小为 0
      continue;
    if (f[i].attr == ATTR_LONG_NAME) // 表示该目录项是长文件名
    {
      if (f[i].fst_clus)
        return TYPE_BMP;
      if (f[i].type)
        return TYPE_BMP;
      if (!has_long_name)
      {
        chk_sum = f[i].chk_sum;
        has_long_name = true;
      }
      else
      {
        if (f[i].chk_sum != chk_sum)
          return TYPE_BMP;
      }
    }
    else
    {
      if (__builtin_popcount(f[i].attr) != 1) // 检查其属性标志 attr 是否只有一个位被置位
        return TYPE_BMP;
      if (has_long_name)
      {
        unsigned char cs = check_sum((unsigned char *)f[i].name);
        if (chk_sum != cs)
          return TYPE_BMP;
      }
      has_long_name = false;
    }
  }
  return TYPE_FDT;
}

static int pos = 127;
static char file_name[128] = {};
static unsigned char chk_sum = 0;
static inline void copy_name(struct FDT *f)
{
  file_name[--pos] = f->name3[2];
  file_name[--pos] = f->name3[0];
  for (int i = 10; i >= 0; i -= 2)
  {
    file_name[--pos] = f->name2[i];
  }
  for (int i = 8; i >= 0; i -= 2)
  {
    file_name[--pos] = f->name1[i];
  }
}
void handle_fdt(void *c, int nr, bool force)
{
  if (c)
  {
    // 添加新的FDT簇到链表
    struct DataSeg *d = malloc(sizeof(struct DataSeg));
    d->head = c;
    d->eof = false;
    d->holder = NULL;
    d->next = fdt_list.next;
    d->prev = &fdt_list;
    fdt_list.next = d;
    d->next->prev = d;
  }

  bool succ = true;
  while (succ)
  {
    succ = false;
    for (struct DataSeg *d = fdt_list.next; d != &fdt_list; d = d->next)
    {
      if (handle_fdt_aux(d->head, nr, force))
      {
        // 从链表中删除已经处理过的FDT簇
        d->prev->next = d->next;
        d->next->prev = d->prev;
        free(d);
        succ = true;
        break;
      }
    }
  }
}
bool handle_fdt_aux(void *c, int nr, bool force)
{
  struct FDT *f = (struct FDT *)c;
  if (force)
  {
    // 表示从最末尾开始构建文件名。
    pos = 127;
  }
  else
  {
    // if (f[0].state == 0xe5) return false;
    if (f[0].attr == ATTR_LONG_NAME)
    {
      if (f[0].state & LAST_LONG_ENTRY)
      {
        chk_sum = f[0].chk_sum;
      }
      else
      {
        if (chk_sum != f[0].chk_sum)
          return false;
      }
    }
    else
    {
      if (f[0].attr != ATTR_DIRECTORY && pos != 127)
      {
        unsigned char cs = check_sum((unsigned char *)f[0].name);
        if (chk_sum != cs)
          return false;
      }
    }
  }

  for (int i = 0; i < nr; ++i)
  {
    if (f[i].attr == ATTR_LONG_NAME)
    {
      if (pos == 127)
        chk_sum = f[i].chk_sum;
      copy_name(f + i);
    }
    else
    {
      if (f[i].attr != ATTR_DIRECTORY && pos != 127)
      {
        size_t len = strlen(file_name + pos);
        if (!strncmp(file_name + pos + len - 4, ".bmp", 4))
        {
          // 该文件是一个 BMP 图像
          uint32_t clus = ((uint32_t)f[i].fst_clus_HI << 16) | f[i].fst_clus_LO;
          if (clus)
          {
            struct Image *image = malloc(sizeof(struct Image));
            sprintf(image->name, "%s", file_name + pos);
            image->size = f[i].file_size;
            image->clus = clus;

            image->file = NULL;
            image->bmp = malloc(sizeof(struct BMP));

            // 放到图像链表中
            image->prev = image_list.prev;
            image->next = &image_list;
            image_list.prev = image;
            image->prev->next = image;
          }
        }
      }
      pos = 127;
      chk_sum = 0;
      file_name[pos] = '\0';
    }
  }
  return true;
}

void handle_image(struct Image *image, size_t sz, int nr)
{
  // 计算图像数据的实际内存地址。
  void *clus = disk->data + sz * (image->clus - disk->mbr->BPB_RootClus);
  struct BMP_Header *header = (struct BMP_Header *)clus;
  struct BMP_Info *info = (struct BMP_Info *)(header + 1);
  if (image->size != header->size || image->size < 32)
  {
    return;
  }

  void *bmp = malloc(image->size);
  Assert(bmp, "malloc failed");

  size_t offset = (size_t)header->offset;
  size_t w = (size_t)(((24 * info->width + 31) >> 5) << 2);

  void *ptr = bmp;
  memcpy(ptr, clus, sz);
  ptr += sz;
  clus += sz;

  // uint8_t *data = (uint8_t *)bmp + offset;
  if (sz < offset)
  {
    return;
  }

  size_t delta = sz - offset;
  size_t x = delta % w;
  size_t y = delta / w;

  size_t cnt = (image->size - 1) / sz;
  for (size_t t = 0; t < cnt; ++t)
  {
    while (clus < disk->tail && get_cluster_type(clus, nr) != TYPE_BMP)
      clus += sz;
    if (clus >= disk->tail)
      break;
    if (t == cnt - 1)
    {
      memcpy(ptr, clus, image->size - sz * cnt);
    }
    else
    {
      memcpy(ptr, clus, sz);
      ptr += sz;
      clus += sz;
      x += sz;
      while (x >= w)
        x -= w, y += 1;
    }
  }

  if (saving_files)
  {
    char save_name[256] = "";
    sprintf(save_name, FOLDER "/%s", image->name);
    image->file = fopen(save_name, "w+");
    Assert(image->file, "fopen failed for image %s", image->name);
    fwrite(bmp, image->size, 1, image->file);
    fclose(image->file);
  }

  int fd0[2] = {};
  int fd1[2] = {};
  int wstatus = 0;
  Assert(pipe(fd0) != -1, "pipe failed");
  Assert(pipe(fd1) != -1, "pipe failed");
  pid_t pid = fork();
  Assert(pid != -1, "fork failed");

  if (pid == 0)
  {
    // child process，计算图像的 SHA1 哈希值
    close(fd0[1]);
    close(fd1[0]);
    dup2(fd0[0], 0);
    dup2(fd1[1], 1);

    char *const args[] = {"sha1sum", "-b", NULL};
    execvp(args[0], args);
    Panic("execvp returned (failed)");
  }
  else
  {
    // parent process
    close(fd0[0]);
    close(fd1[1]);

    write(fd0[1], bmp, image->size);
    close(fd0[1]);
    wait(&wstatus);

    char buf[256] = "";
    read(fd1[0], buf, sizeof(buf));
    close(fd1[0]);
    sscanf(buf, "%s", image->sha1);
    printf("%s  %s\n", image->sha1, image->name);
  }

  free(bmp);
}
