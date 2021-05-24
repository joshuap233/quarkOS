//
// Created by pjs on 2021/4/20.
//

#ifndef QUARKOS_FS_EXT2_H
#define QUARKOS_FS_EXT2_H

#include "types.h"
#include "fs/vfs.h"

#define ROOT_INUM            2 // 根目录的 inode 索引, inode 索引从 1 开始

#define BLOCK_SIZE           4096
#define INODE_SIZE           256
#define INODE_PER_BLOCK      (BLOCK_SIZE/INODE_SIZE)
#define SUPER_BLOCK_NO       2
#define BLOCK_PER_GROUP      (BLOCK_SIZE*8)
#define DESCRIPTOR_PER_BLOCK (BLOCK_SIZE/sizeof(groupDesc_t))
#define N_DIRECT_BLOCK       12                         // 直接指针数量
#define N_PTR                (BLOCK_SIZE/sizeof(u32_t)) // 块内指针数量
#define SECTOR_PER_BLOCK     (BLOCK_SIZE/SECTOR_SIZE)
#define N_BLOCKS             (N_DIRECT_BLOCK+3)


#define SEPARATOR            '/'

typedef struct ext2_superBlock {
#define EXT2_SIGNATURE 0xef53
    u32_t inodeCnt;        // inode 数
    u32_t blockCnt;
    u32_t rBlockCnt;       // 为超级用户保留的块
    u32_t freeBlockCnt;    // 没有被分配的块数
    u32_t freeInodeCnt;
    u32_t firstDataBlock;  // 包含超级块的块数量

    u32_t logBlockSize;    // 值为: log2(block size) - 10
    u32_t logFragmentSize; // log2(fragment size) - 10

    u32_t blockPerGroup;
    u32_t fragmentsPerGroup;
    u32_t inodePerGroup;

    u32_t mountTime;
    u32_t writtenTime;

    u16_t mountCnt;
    u16_t maxMountCnt;

    u16_t magic;

    u16_t state;                  // 文件系统状态

    u16_t error;                  // 处理错误的方法

    u16_t fsVersionMinor;

    u32_t fsckTime;               // 上次 fsck 检查(POSIX)时间
    u32_t fsckInterval;           // 强制 fsck 检查时间间隔

    u32_t creatorOS;

    u32_t fsVersionMajor;

    u16_t uid;                   //可以使用保留块的用户id
    u16_t gid;                   //可以使用保留块的用户组 id


/* -- fs_version_major >= 1 时, 以下字段有效 --*/
    u32_t firstInode;            // 版本<1则 = 11, 第一个非保留 inode
    u16_t inodeSize;             // 版本<1则 = 128
    u16_t blockGroup;            // 当前超级块所属的块组

    u32_t optionalFeature;       // 可选功能
    u32_t requiredFeature;
    u32_t optionalFeatureRo;     // 如果不支持这些功能则只读挂载

    u8_t uuid[16];
    u8_t volumeName[16];         // 以 0 结束的字符串
    u8_t pathLastMount[64];
    u32_t algorithmsBitmap;      // 位图使用的压缩算法

/* ----- 性能 ----- */
    u8_t preallocateBlock;       // 为文件预分配的块数
    u8_t preallocateBlockDir;    // 为目录预分配的块数
    u16_t unused;

/* ----- 日志 ----- */
    u8_t journalId[16];
    u32_t journalInode;
    u32_t journalDevice;
    u32_t lastOrphan;


/* ----- 目录索引 ----- */
    u32_t hashSeed[4];            // HTREE hash seed
    u8_t defHashVersion;          // Default hash version to use
    u8_t padding[3];

/* ----- 其他选项 ----- */
    u32_t defaultMountOpts;
    u32_t firstMetaBg;            // First metablock block group

    u8_t unused2[760];
} ext2_sb_t;

/* ----- stat ----- */
#define EXT2_VALID_FS          2
#define EXT2_ERROR_FS          1

/* ----- stat ----- */
#define EXT2_ERRORS_CONTINUE   1
#define EXT2_ERRORS_RO         2
#define EXT2_ERRORS_PANIC      3


/* ----- creatorOS ----- */
#define OS_LINUX    0
#define OS_GUN_HURD 1
#define OS_MASIX    2
#define OS_FREE_BSD 3
#define OS_OTHER    4

/* ----- optionalFeature ----- */
#define PREALLOCATE_FOR_DIR 1    // 创建目录时预分配一些数据块
#define AFS_SERVER_INODES   2
#define JOURNAL             4    // 支持日志(ext3)
#define INODE_EXT           8    // inode 可以有扩展属性
#define FS_RESIZE           0x10 // fs 可以调整大小
#define DIR_HASH_INDEX      0x20 // 目录使用哈希索引

/* ----- requiredFeature ----- */
#define COMPRESS            1    // 压缩
#define DIR_TYPE_FIELD      2    // 目录有类型字段
#define REPLAY_JOURNAL      4
#define JOURNAL_DEV         8

/* ----- optionalFeatureRo ----- */
#define SS_GROUP_DESC       1    // Sparse superblocks and group descriptor tables
#define FS_64BIT_SIZE       2
#define DIR_ARE_BIN_TREE    4    // 目录以二叉树的形式存储

//Block Group Descriptor Table, 紧接超级块后
typedef struct ext2_group_desc {
    u32_t blockBitmapAddr;
    u32_t inodeBitmapAddr;
    u32_t inodeTableAddr;
    u16_t freeBlockCnt;         // 组中没有分配的块数
    u16_t freeInodesCnt;
    u16_t dirNum;               // 组中目录数
    u8_t unused[14];
} groupDesc_t;

typedef struct ext2_inode {
    u16_t mode;                 // 权限与类型
    u16_t uid;

/*
 * 如果目录,则 size 为分配的块大小,
 * 如果是文件,则 size 为实际存储的数据大小,比如存储 'aa'(ascii),则文件大小为 3,
 * 如果版本大于 1 ,且为常规文件, size 为 文件大小低 32 位, 高 32 位在 dirAcl 字段
 */
    u32_t size;

    u32_t accessTime;
    u32_t createTime;
    u32_t modTime;                 // 修改时间
    u32_t delTime;

    u16_t groupId;

    u16_t linkCnt;                // 链接数量(目录链接数量为 dir entry 数, . 与 .. 也算)

    u32_t cntSectors;             // 磁盘扇区数(不包括inode与链接),间接指针块本身也算一块(分配间接块 cntSectors + 8)

    u32_t flags;
    u32_t osSpec;

    u32_t blocks[N_BLOCKS]; // 0-12 为直接指针

    u32_t generationNum;
    u32_t filAcl;                     // 版本>=1 有效
    u32_t dirAcl;                     // 版本>=1 有效
    u32_t fragmentAddr;
    u32_t osSpec2[3];
} ext2_inode_t;


/* ---------- mode ----------- */
/*    -- 文件类型  -- */
#define EXT2_IFSOCK   0xC000    //socket
#define EXT2_IFLNK    0xA000    //symbolic link
#define EXT2_IFREG    0x8000    //regular file
#define EXT2_IFBLK    0x6000    //block device
#define EXT2_IFDIR    0x4000    //directory
#define EXT2_IFCHR    0x2000    //character device
#define EXT2_IFIFO    0x1000    //fifo

/* -- process execution user/group override -- */
#define EXT2_ISUID    0x0800    //Set process User ID
#define EXT2_ISGID    0x0400    //Set process Group ID
#define EXT2_ISVTX    0x0200    //sticky bit

/* -- 访问控制 -- */
#define EXT2_IRUSR    0x0100    //user read
#define EXT2_IWUSR    0x0080    //user write
#define EXT2_IXUSR    0x0040    //user execute
#define EXT2_IRGRP    0x0020    //group read
#define EXT2_IWGRP    0x0010    //group write
#define EXT2_IXGRP    0x0008    //group execute
#define EXT2_IROTH    0x0004    //others read
#define EXT2_IWOTH    0x0002    //others write
#define EXT2_IXOTH    0x0001    //others execute

#define EXT2_ALL_RWX  (EXT2_IRUSR|EXT2_IWUSR|EXT2_IXUSR|EXT2_IRGRP|EXT2_IWGRP|EXT2_IXGRP|EXT2_IROTH|EXT2_IXOTH)
/* ------------- end mode --------- */


/* ---------- flags ----------- */
#define SYNC_UPDATE         0x8   //新数据立即写入磁盘
#define IMM_FILE            0x10  // 文件内容不可变
#define APPEND_ONLY         0x20
#define HASH_INDEX_DIR      0x10000
#define AFS_DIR             0x20000
#define JOURNAL_FILE_DATA   0x40000

typedef struct ext2_directory_entry {
    u32_t inode;
    u16_t entrySize; // 目录项总大小
    u8_t nameLen;
    u8_t type;
    char name[0];
} PACKED ext2_dir_t;

/* ---------- type ----------- */
//目录项指向的文件类型
#define EXT2_FT_UNKNOWN            0
#define EXT2_FT_REG_FILE           1
#define EXT2_FT_DIR                2
#define EXT2_FT_CHRDEV             3
#define EXT2_FT_BLKDEV             4
#define EXT2_FT_FIFO               5
#define EXT2_FT_SOCK               6
#define EXT2_FT_SYMLINK            7


typedef struct ext2_sb_info {
    super_block_t sb;
    u32_t inodePerGroup;
    u32_t blockPerGroup;
    u32_t version;
    u32_t groupCnt;
    struct info_descriptor {
        buf_t *blockBitmap;
        buf_t *inodeBitmap;
        u32_t inodeTableAddr;
        u16_t freeBlockCnt;
        u16_t freeInodesCnt;
        u16_t dirNum;               // 组中目录数
    } *desc;
} ext2_sb_info_t;


typedef struct ext2_inode_info {
    inode_t inode;

    u32_t blockCnt;                 // 实际分配的磁盘块数
    u32_t blocks[N_BLOCKS];
} ext2_inode_info_t;

typedef directory_t ext2_dir_info_t;

#define ext2_i(ptr) container_of(ptr, ext2_inode_info_t, inode)
#define ext2_s(ptr) container_of(ptr, ext2_sb_info_t, sb)

#define EXT2_BLOCK2LBA(block_no)     (BLOCK_SIZE / SECTOR_SIZE * (block_no))

#define EXT2_INODE2GROUP(ino, sb)    (((ino)-1) / (sb)->inodePerGroup)

#define ext2_block_read(blk_no)      page_read_no(EXT2_BLOCK2LBA(blk_no))

#define ext2_block_write(buf)        page_write(buf)

#endif //QUARKOS_FS_EXT2_H
