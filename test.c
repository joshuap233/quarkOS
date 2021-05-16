#include "stdio.h"
#include "stdint.h"

typedef uint64_t u64_t;
typedef uint32_t u32_t;
typedef uint16_t u16_t;
typedef uint8_t u8_t;

typedef struct {
     int counter;
} atomic_t;

struct foo {
    u8_t flags;
};

static void atomic_set(atomic_t *v, int i) {
    v->counter = i;
}

typedef struct inode {
    u16_t mode;             // 权限与类型
// 类型
#define MOD_FIFO            0x1000
#define MOD_CHARACTER_DEV   0x2000
#define MOD_DIR             0x4000
#define MOD_BLOCK           0x6000
#define MOD_REGULAR_FILE    0x8000
#define MOD_SYMBOLIC_LINK   0xA000
#define MOD_UNIX_SOCKET     0xC000
// 权限
#define MOD_X               0x001
#define MOD_W               0x002
#define MOD_R               0x004

#define MOD_OTHER           0
#define MOD_GROUP           3
#define MOD_USER            6
#define GET_MODE(who, permit)  ((permit) << (who))

#define USER_MODE           GET_MODE(MOD_USER,MOD_X|MOD_W|MOD_R)

    u16_t uid;
/*
 * In revision 1 and later revisions, and only for regular files,
 * this represents the lower 32-bit of the file size;
 * the upper 32-bit is located in the i_dir_acl.
 */
    u32_t size;

    u32_t accessTime;
    u32_t createTime;
    u32_t modTime;   // 修改时间
    u32_t delTime;

    u16_t groupId;

    u16_t linkCnt;    // 链接数量

    u32_t cntSectors; // 磁盘扇区数(不包括inode与链接)

    u32_t flags;
#define SYNC_UPDATE         0x8   //新数据立即写入磁盘
#define IMM_FILE            0x10  // 文件内容不可变
#define APPEND_ONLY         0x20
#define HASH_INDEX_DIR      0x10000
#define AFS_DIR             0x20000
#define JOURNAL_FILE_DATA   0x40000
    u32_t osSpec;

    u32_t directPtr[12]; // 直接指针
    u32_t siPtr;         // 间接指针
    u32_t diPtr;         // 双重间接指针
    u32_t tiPtr;         // 三重间接指针

    u32_t generationNum;
    u32_t filAcl;         // 版本>=1 有效
    u32_t dirAcl;         // 版本>=1 有效
    u32_t fragmentAddr;
    u32_t osSpec2[3];
} inode_t;

typedef struct blockGroupDescriptor {
    u32_t blockBitmapAddr;
    u32_t inodeBitmapAddr;
    u32_t inodeTableAddr;
    u16_t freeBlockCnt;     // 组中没有分配的块数
    u16_t freeInodesCnt;
    u16_t dirNum;           // 组中目录数
    u8_t unused[14];
} groupDesc_t;

void test(u32_t foo){
    printf("%d\n",foo);
}

int main() {
//    atomic_t v;
//    atomic_set(&v, 1);
    u32_t foo = 0;
    test(foo++);
}