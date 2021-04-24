//
// Created by pjs on 2021/4/20.
//

#ifndef QUARKOS_FS_FAT_H
#define QUARKOS_FS_FAT_H

#include "types.h"
#include "buf.h"

#define ROOT_INUM    2 // 根目录的 inode 索引, inode 索引从 1 开始

typedef struct superBlock {
#define EXT2_SIGNATURE 0xef53
    uint32_t inodeCnt;        // inode 数
    uint32_t blockCnt;
    uint32_t rBlockCnt;       // 为超级用户保留的块
    uint32_t freeBlockCnt;    // 没有被分配的块数
    uint32_t freeInodeCnt;
    uint32_t firstDataBlock;  // 包含超级块的块数量

    uint32_t logBlockSize;    // 值为: log2(block size) - 10
    uint32_t logFragmentSize; // log2(fragment size) - 10

    uint32_t blockPerGroup;
    uint32_t fragmentsPerGroup;
    uint32_t inodePerGroup;

    uint32_t mountTime;
    uint32_t writtenTime;

    uint16_t mountCnt;
    uint16_t maxMountCnt;

    uint16_t magic;

    uint16_t state;        // 文件系统状态
#define FS_STATE_ERROR 2
#define FS_STATE_CLEAN 1

    uint16_t error;        // 处理错误的方法
#define FS_ERR_IGNORE            1
#define FS_ERR_REMOUNT_AS_READ_ONLY 2
#define FS_ERR_KERNEL_PANIC      3

    uint16_t fsVersionMinor;

    uint32_t fsckTime;         // 上次 fsck 检查(POSIX)时间
    uint32_t fsckInterval;     // 强制 fsck 检查时间间隔

    uint32_t creatorOS;
#define OS_LINUX    0
#define OS_GUN_HURD 1
#define OS_MASIX    2
#define OS_FREE_BSD 3
#define OS_OTHER    4


    uint32_t fsVersionMajor;

    uint16_t uid; //可以使用保留块的用户id
    uint16_t gid; //可以使用保留块的用户组 id

//fs_version_major >= 1 时, 以下字段有效
    uint32_t firstInode;  // 版本<1则 = 11, 第一个非保留 inode
    uint16_t inodeSize;   // 版本<1则 = 128
    uint16_t blockGroup;  // 当前超级块所属的块组

    uint32_t optionalFeature;    // 可选功能
#define PREALLOCATE_FOR_DIR 1    // 创建目录时预分配一些数据块
#define AFS_SERVER_INODES   2
#define JOURNAL             4    // 支持日志(ext3)
#define INODE_EXT           8    // inode 可以有扩展属性
#define FS_RESIZE           0x10 // fs 可以调整大小
#define DIR_HASH_INDEX      0x20 // 目录使用哈希索引
    uint32_t requiredFeature;
#define COMPRESS            1    // 压缩
#define DIR_TYPE_FIELD      2    // 目录有类型字段
#define REPLAY_JOURNAL      4
#define JOURNAL_DEV         8
    uint32_t optionalFeatureRo; //如果不支持这些功能则只读挂载
#define SS_GROUP_DESC       1   // Sparse superblocks and group descriptor tables
#define FS_64BIT_SIZE       2
#define DIR_ARE_BIN_TREE    4   // 目录以二叉树的形式存储

    uint8_t fsId[16];

    uint8_t volumeName[16];      // 以 0 结束的字符串
    uint8_t pathLastMount[64];

    uint32_t algorithmsBitmap;   // 位图使用的压缩算法
    uint8_t preallocateBlock;    // 为文件预分配的块数
    uint8_t preallocateBlockDir; // 为目录预分配的块数

    uint16_t unused;

    uint8_t journalId[16];
    uint32_t journalInode;
    uint32_t journalDevice;

    uint32_t inodeListHead;          // start of list of inodes to delete

    uint32_t hashSeed[4];            // HTREE hash seed
    uint8_t defHashVersion;          // Default hash version to use
    uint8_t reservedCharPad;
    uint16_t reservedWordPad;
    uint32_t defaultMountOpts;
    uint32_t firstMetaBg;            // First metablock block group

    uint8_t unused2[760];
} superBlock_t;

//Block Group Descriptor Table, 紧接超级块后
typedef struct blockGroupDescriptor {
    uint32_t blockBitmapAddr;
    uint32_t inodeBitmapAddr;
    uint32_t inodeTableAddr;
    uint16_t freeBlockCnt;     // 组中没有分配的块数
    uint16_t freeInodesCnt;
    uint16_t dirNum;           // 组中目录数
    uint8_t  unused[14];
} groupDesc_t;

typedef struct inode {
    uint16_t mode;             // 权限与类型
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

    uint16_t uid;
/*
 * In revision 1 and later revisions, and only for regular files,
 * this represents the lower 32-bit of the file size;
 * the upper 32-bit is located in the i_dir_acl.
 */
    uint32_t size;

    uint32_t accessTime;
    uint32_t createTime;
    uint32_t modTime;   // 修改时间
    uint32_t delTime;

    uint16_t groupId;

    uint16_t linkCnt;    // 链接数量

    uint32_t cntSectors; // 磁盘扇区数(不包括inode与链接)

    uint32_t flags;
#define SYNC_UPDATE         0x8   //新数据立即写入磁盘
#define IMM_FILE            0x10  // 文件内容不可变
#define APPEND_ONLY         0x20
#define HASH_INDEX_DIR      0x10000
#define AFS_DIR             0x20000
#define JOURNAL_FILE_DATA   0x40000
    uint32_t osSpec;

    uint32_t directPtr[12]; // 直接指针
    uint32_t siPtr;         // 间接指针
    uint32_t diPtr;         // 双重间接指针
    uint32_t tiPtr;         // 三重间接指针

    uint32_t generationNum;
    uint32_t acl;          // 版本>=1 有效
    uint32_t dirAcl;       // 版本>=1 有效
    uint32_t fragmentAddr;
    uint32_t osSpec2[3];
} inode_t;

struct directory_entry {
    uint32_t inode;
    uint16_t entrySize; // 目录项总大小
    uint8_t nameLengthLow;
    uint8_t typeIndicator;
    //目录项指向的文件类型
#define DIR_UNKNOWN            0
#define DIR_REGULAR_FILE       1
#define DIR_DIR                2
#define DIR_CHAR               3
#define DIR_BLOCK              4
#define DIR_FIFO               5
#define DIR_SOCKET             6
#define DIR_SOFT_LINK          7
    uint8_t name[1];
};



#endif //QUARKOS_FS_FAT_H
