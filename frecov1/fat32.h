#ifndef __FAT32_H__
#define __FAT32_H__

#include "frecov.h"
#include <stdint.h>
#include <stdio.h>

struct MBR
{
  // common BPB structure
  char BS_jmpBoot[3];      // 0x00 - 0x02 跳转指令
  char BS_OEMName[8];      // 0x03 - 0x0A OEM 名称
  uint16_t BPB_BytsPerSec; // 0x0B - 0x0C 每扇区字节数
  uint8_t BPB_SecPerClus;  // 0x0D 每簇扇区数
  uint16_t BPB_RsvdSecCnt; // 0x0E - 0x0F 保留扇区数
  uint8_t BPB_NumFATs;     // 0x10 FAT 表数量
  uint16_t BPB_RootEntCnt; // 0x11 - 0x12 根目录项数
  uint16_t BPB_TotSec16;   // 0x13 - 0x14 总扇区数（小于 65536）
  uint8_t BPB_Media;       // 0x15 媒体类型
  uint16_t BPB_FATSz16;    // 0x16 - 0x17 每个 FAT 表的扇区数
  uint16_t BPB_SecPerTrk;  // 0x18 - 0x19 每磁道扇区数
  uint16_t BPB_NumHeads;   // 0x1A - 0x1B 磁头数
  uint32_t BPB_HiddSec;    // 0x1C - 0x1F 隐藏扇区数
  uint32_t BPB_TotSec32;   // 0x20 - 0x23 总扇区数（如果大于 65536）

  // extended BPB structure for FAT32
  uint32_t BPB_FATSz32;   // 0x24 - 0x27 FAT32 表的扇区数
  uint16_t BPB_ExtFlags;  // 0x28 - 0x29 扩展标志
  uint16_t BPB_FSVer;     // 0x2A - 0x2B 文件系统版本
  uint32_t BPB_RootClus;  // 0x2C - 0x2F 根目录起始簇号
  uint16_t BPB_FSInfo;    // 0x30 - 0x31 FSInfo 结构的扇区号
  uint16_t BPB_BkBootSec; // 0x32 - 0x33 备份引导扇区的位置
  char BPB_Reserved[12];  // 0x34 - 0x3F 保留字段
  uint8_t BS_DrvNum;      // 0x40 驱动器号
  uint8_t BS_Reserved1;   // 0x41 保留
  uint8_t BS_BootSig;     // 0x42 扩展引导标志
  uint32_t BS_VolID;      // 0x43 - 0x46 卷序列号
  char BS_VolLab[11];     // 0x47 - 0x51 卷标
  char BS_FilSysType[8];  // 0x52 - 0x59 文件系统类型字符串
  char EMPTY[420];        // 0x5A 填充字段，确保引导扇区总大小为 512 字节
  uint16_t SignatureWord; // 0x1FE 结束标志（0xAA55）
} __attribute__((packed));
// 确保结构体内的成员不会被编译器填充，以保证结构体大小与实际数据大小一致。

/*
这个结构体表示 FAT 表的一个条目。
FAT 表条目可以表示簇的状态或者指向下一个簇的地址。
使用联合体（union）允许同一块内存区域有两种解释方式：
mark 表示原始字节数组，next 表示下一个簇号。
 */
struct FAT
{
  union
  {
    uint8_t mark[4];
    uint32_t next;
  };
} __attribute__((packed));

struct FSInfo
{
  uint32_t FSI_LeadSig;    // 0x000 文件系统信息的签名
  char FSI_Reserved1[480]; // 0x004 保留字段
  uint32_t FSI_StrucSig;   // 0x1E4 结构签名
  uint32_t FSI_Free_Count; // 0x1E8 空闲簇的数量
  uint32_t FSI_Nxt_Free;   // 0x1EC 下一个可用簇
  char FSI_Reserved2[12];  // 0x1F0 保留字段
  uint32_t FSI_TrailSig;   // 0x1FE 尾部签名
} __attribute__((packed));

#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_LONG_NAME 0x0f
#define ATTR_LONG_NAME_MASK 0x3f
#define LAST_LONG_ENTRY 0x40

struct FDT
{
  union
  {
    struct
    {
      union
      {
        uint8_t state; // 状态字节
        char name[11]; // 短文件名
      };
      uint8_t attr;         // 文件属性
      uint8_t reserved;     // 保留字段
      uint8_t crt_time_10;  // 创建时间的 10 毫秒位
      uint16_t crt_time;    // 创建时间
      uint16_t crt_date;    // 创建日期
      uint16_t acc_date;    // 最后访问日期
      uint16_t fst_clus_HI; // 起始簇号的高 16 位
      uint16_t wrt_time;    // 最后写入时间
      uint16_t wrt_date;    // 最后写入日期
      uint16_t fst_clus_LO; // 起始簇号的低 16 位
      uint32_t file_size;   // 文件大小
    };
    struct
    {
      uint8_t order;     // 长文件名条目的顺序号
      char name1[10];    // 长文件名的一部分
      uint8_t attr_long; // 文件属性（长文件名）
      uint8_t type;      // 长文件名条目类型
      uint8_t chk_sum;   // 短文件名的校验和
      char name2[12];    // 长文件名的另一部分
      uint16_t fst_clus; // 簇号
      char name3[4];     // 长文件名的最后一部分
    };
  };
} __attribute__((packed));

struct Disk
{
  void *head;            // 指向磁盘开始的指针
  void *tail;            // 指向磁盘结束的指针
  struct MBR *mbr;       // 指向 MBR 的指针
  struct FSInfo *fsinfo; // 指向 FSInfo 的指针
  struct FAT *fat[2];    // 指向两个 FAT 表的指针数组
  void *data;            // 指向数据区的指针
};
struct Disk *disk_load_fat(const char *);
void disk_get_sections(struct Disk *);
unsigned char check_sum(unsigned char *);

#endif
