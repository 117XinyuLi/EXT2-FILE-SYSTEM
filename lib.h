#ifndef EXT2_INIT_H
#define EXT2_INIT_H


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "main.h"

#define VOLUME_NAME         "EXT2FS"    //卷名
#define BLOCK_SIZE          512         //块大小
#define NUM_BLOCK           4611        //磁盘总块数

#define DISK_START          0           //磁盘开始地址

#define GDT_START           0           //块组描述符起始地址
#define GD_SIZE             32          //块组描述符大小是32B

#define BLOCK_BITMAP        512         //块位图起始地址
#define INODE_BITMAP        1024        //inode 位图起始地址

#define INODE_TABLE         1536        //索引节点表起始地址
#define INODE_SIZE          64          //每个inode的大小是 64B
#define INODE_TABLE_COUNTS  4096        //inode数

#define DATA_BLOCK          263680      //数据块起始地址
#define DATA_BLOCK_COUNTS   4096        //数据块数

#define BLOCKS_PER_GROUP    4611        //每组中的块数

#define USER_MAX            3           //用户个数
#define FOPEN_TABLE_MAX     16          //文件打开表大小

#define MAX_FILE_SIZE     (2*1024*1024) //文件最大大小

// 组描述符
struct group_desc { // 32 B
    char bg_volume_name[16]; //文件系统名
    unsigned short bg_block_bitmap; //块位图的起始块号
    unsigned short bg_inode_bitmap; //索引结点位图的起始块号
    unsigned short bg_inode_table; //索引结点表的起始块号
    unsigned short bg_free_blocks_count; //本组空闲块的个数
    unsigned short bg_free_inodes_count; //本组空闲索引结点的个数
    unsigned short bg_used_dirs_count; //组中分配给目录的结点数
    char bg_pad[4]; //填充(0xff)
};

// 索引结点
struct inode { // 64 B 
    unsigned short i_mode;   //文件类型及访问权限
    unsigned short i_blocks; //文件所占的数据块个数(0~7), 最大为7
    unsigned short i_uid;    //文件拥有者标识号
    unsigned short i_gid;    //文件的用户组标识符
    unsigned short i_links_count; //文件的硬链接计数
    unsigned short i_flags;  //打开文件的方式
    unsigned long i_size;    //文件或目录大小(单位 byte)
    unsigned long i_atime;   //访问时间
    unsigned long i_ctime;   //创建时间
    unsigned long i_mtime;   //修改时间
    unsigned long i_dtime;   //删除时间
    unsigned short i_block[8]; //直接索引方式 指向数据块号
    char i_pad[16];           //填充(0xff)
};

// 只有指针的索引结点
struct inode_ptr { // 16 B
    unsigned short i_block[8]; //直接索引方式 指向数据块号
};

// 目录项入口
struct dir_entry { // 16 B
    unsigned short inode; //索引节点号
    unsigned short rec_len; //目录项长度
    unsigned short name_len; //文件名长度
    char file_type; //文件类型(1 普通文件 2 目录.. )
    char name[9]; //文件名
};

// 用户信息
struct user {
    char username[10];
    char password[10];
    char disk_name[10];
    unsigned short u_uid;   //用户标识号
    unsigned short u_gid;
}User[USER_MAX];

static unsigned short last_alloc_inode; // 最近分配的i节点号
static unsigned short last_alloc_block; // 最近分配的数据块号
static unsigned short current_dir;   // 当前目录的节点号

static unsigned short current_dirlen; // 当前路径长度

static short fopen_table[FOPEN_TABLE_MAX]; // 文件打开表

static struct group_desc group_desc_buf;    // 组描述符缓冲区
static struct inode inode_buf;  // inode缓冲区
static unsigned char bitbuf[512] = {0}; // block位图缓冲区
static unsigned char ibuf[512] = {0};  // inode位图缓冲区
static struct dir_entry dir[32] = {0};   // 目录项缓冲区，一个块可以存放32个目录项
static char Buffer[BLOCK_SIZE] = {0};  // 针对数据块的缓冲区
static struct inode_ptr inode_ptr_buf[64]={0}; // 指针inode缓冲区
static struct inode_ptr inode_ptr_buf_level_2[64]={0}; // 指针inode缓冲区2
static char tempbuf[MAX_FILE_SIZE] = {0};// 文件写入缓冲区
static FILE *fp;    // 虚拟磁盘指针


char current_path[256];    // 当前路径名
char current_user[10];     // 当前用户名
char current_disk[10];     // 当前磁盘名

int user_num;   // 用户个数
int user_index;   // 当前用户id

static void update_group_desc();    //更新组描述符内容
static void reload_group_desc();    //加载组描述符内容
static void update_inode_entry(unsigned short i); //更新indoe表
static void reload_inode_entry(unsigned short i); //加载inode表
static void update_block_bitmap();  //更新块位图
static void reload_block_bitmap();  //加载块位图
static void update_inode_bitmap();  //更新inode位图
static void reload_inode_bitmap();  //加载inode位图
static void update_dir(unsigned short i);//更新目录
static void reload_dir(unsigned short i);//加载目录
static void update_block(unsigned short i);//更新数据块
static void reload_block(unsigned short i);//加载数据库
static int alloc_block();//分配数据块
static int get_inode(); //得到inode节点
static unsigned short research_file(char tmp[100], int file_type, unsigned short *inode_num, unsigned short *block_num,
                                   unsigned short *dir_num);//查找文件
static void dir_prepare(unsigned short tmp, unsigned short len, int type, int mode);//目录准备

static void remove_block(unsigned short del_num);//删除数据块
static void remove_inode(unsigned short del_num);//删除inode节点
static  unsigned short search_file(unsigned short Ino);//在打开文件表中查找是否已打开文件
static void initialize_disk();//初始化磁盘

#endif //EXT2_INIT_H
