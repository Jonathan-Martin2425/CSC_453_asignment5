
#include <stdio.h>
#include <stdint.h>

#define SUPMAGIC 0x4D5A
#define SUPEROFF 1024
#define NUM_SUP 1
#define MALLOCERR "Malloc error"
#define READERR "read error"
#define FILEERR "FILE error"
#define NO_PART -1

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

struct superblock *get_superblock(FILE *, off_t, int);
off_t get_inode_table(struct superblock *, off_t);
void print_part_table(int, off_t, int, int);