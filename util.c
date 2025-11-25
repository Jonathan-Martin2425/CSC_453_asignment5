
/* located in /usr/src/servers/mfs */
#include <super.h>
#include <stdio.h>
#include <stdint.h>
#include "util.h"

#define SUPMAGIC 0x4D5A
#define SUPEROFF 1024
#define NUM_SUP 1
#define MALLOCERR "Malloc error"
#define READERR "read error"
#define FILEERR "FILE error"

/* given the start position of the disk and the opened image file,
   returns an allocated struct of the superblock if it is valid*/
struct superblock *get_superblock(FILE *image, off_t disk_start){
    struct superblock *super;

    /* set file ptr to start of disk plus the offset to
       the superblock to get ready to read it */  
    if(fseek(image, disk_start + SUPEROFF, SEEK_SET) < 0){
        perror(FILEERR);
        return NULL;
    }

    /* allocate struct for superblock */
    super = (struct superblock*) malloc(sizeof(struct superblock));
    if(super == NULL){
        perror(MALLOCERR);
        return NULL;
    }

    /* read superblock from image */
    if(fread((void*)super, sizeof(struct superblock), NUM_SUP, image) <= 0){
        perror(READERR);
        return NULL;
    }

    /*check if superblock magic number is valid,
      if it isn't return NULL, otherwise return superblock */
    if(super->magic == SUPMAGIC){
        return super;
    }else{
        free(super);
        return NULL;
    }
}

/* given the start of disk and superblock,
   returns the offset to the inode table  */
off_t get_inode_table(struct superblock *super, off_t disk_start){
    int16_t block_size = super->blocksize;
    off_t inode_offset;

    /* inode table is calculated from start of disk + boot sector
       plus the blocks allocated by the superblock, inode bitmap
       and zone bitmap found in the superblock*/
    inode_offset = disk_start + SUPEROFF +
                   block_size * (1 + super->i_blocks + super->z_blocks);

    return inode_offset;
}