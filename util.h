
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include "partition.h"

#define SUPMAGIC 0x4D5A
#define SUPEROFF 1024
#define NUM_SUP 1
#define NUM_PERMS 3
#define NUM_PART 4
#define DIRECT_ZONES 7
#define MODE_PRINT_SIZE 12
#define NAME_SIZE 60
#define FIRST_BLOCKS 2
#define MALLOCERR "Malloc error"
#define READERR "read error"
#define FILEERR "FILE error"
#define SUPERERR "Superblock magic number invalid. Not a MINIX file system"
#define INODEERR "Invalid inode, inode number is greater than total ammount of inodes"
#define DIRERR "File in path is not a DIR"
#define TOOBIG "File has a size greater than the max possible size"
#define FILENOTFOUNDERR "File couldn't be found"
#define NO_PART -1

#define SUB_PART_PRINT "Subparition table %d: \n"
#define PART_PRINT "partion table:\n"
#define PART_ENTRY_PRINT "\tpartition entry: %d\n"
#define ACTUAL_PART_ENTRY "\tpartition entry: %d (selected partiton)\n"
#define STR_ATTRIBUTE_PRINT "%s: %s\n"
#define NUM_ATTRIBUTE_PRINT "\t\t%s: %d\n"
#define HEX_ATTRIBUTE_PRINT "\t\t%s: 0x%X\n"
#define MODE_ATTRIBUTE_PRINT "\t\tuint16_t mode: 0x%X (%s)\n"
#define TIME_ATTRIBUTE_PRINT "\t\t%s: %d --- %s"
#define ZONE_ATTRIBUTE_PRINT "\t\tzone[%d] = %d\n"
#define SUP_NAME "Superblock Contents:\nStored Fields:\n"
#define ZONE_NAME "%s zones:\n"
#define INODE_NAME "File inode:\n"
#define PATH_DELIM "/"
#define NEW_LINE "\n"

#define FILE_TYPE_MASK 0xF000
#define REG_MASK 0x8000
#define DIR_MASK 0x4000

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

struct superblock { /* Minix Version 3 Superblock
                    * this structure found in fs/super.h
                    * in minix 3.1.1
                    */
    /* on disk. These fields and orientation are non–negotiable */
    uint32_t ninodes; /* number of inodes in this filesystem */
    uint16_t pad1; /* make things line up properly */
    int16_t i_blocks; /* # of blocks used by inode bit map */
    int16_t z_blocks; /* # of blocks used by zone bit map */
    uint16_t firstdata; /* number of first data zone */
    int16_t log_zone_size; /* log2 of blocks per zone */
    int16_t pad2; /* make things line up again */
    uint32_t max_file; /* maximum file size */
    uint32_t zones; /* number of zones on disk */
    int16_t magic; /* magic number */
    int16_t pad3; /* make things line up again */
    uint16_t blocksize; /* block size in bytes */
    uint8_t subversion; /* filesystem sub–version */
};

struct inode {
    uint16_t mode; /* mode */
    uint16_t links; /* number or links */
    uint16_t uid;
    uint16_t gid;
    uint32_t size;
    int32_t atime;
    int32_t mtime;
    int32_t ctime;
    uint32_t zone[DIRECT_ZONES];
    uint32_t indirect;
    uint32_t two_indirect;
    uint32_t unused;
};

struct dir_entry {
    uint32_t inode;
    unsigned char name[60];
};

int read_zone(FILE *, off_t, uint32_t, uint32_t, void*);
void *read_file(FILE *, struct inode *, struct superblock *, off_t);
struct superblock *get_superblock(FILE *, off_t, int);
off_t get_inode_table(struct superblock *, off_t);
int find_file(char *, FILE *, off_t , struct inode *, int);
void print_superblock(struct superblock *);
void print_inode(struct inode);