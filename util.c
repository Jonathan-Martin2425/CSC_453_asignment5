

#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include "util.h"
#include "partition.h"

#define SUPMAGIC 0x4D5A
#define SUPEROFF 1024
#define NUM_SUP 1
#define NUM_PART 4
#define MALLOCERR "Malloc error"
#define READERR "read error"
#define FILEERR "FILE error"
#define NO_PART -1

#define SUB_PART_PRINT "Subparition table %d: \n"
#define PART_PRINT "partion table:\n"
#define PART_ENTRY_PRINT "\tpartition entry: %d\n"
#define ACTUAL_PART_ENTRY "\tpartition entry: %d (selected partiton)\n"
#define NUM_ATTRIBUTE_PRINT "\t\t%s: %d\n"
#define HEX_ATTRIBUTE_PRINT "\t\t%s: 0x%X\n"
#define SUP_NAME "Superblock Contents:\nStored Fields:\n"

/* given the start position of the disk and the opened image file,
   returns an allocated struct of the superblock if it is valid*/
struct superblock *get_superblock(FILE *image, off_t disk_start, int isV){
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

    if(isV){
        fprintf(stderr, SUP_NAME);
        fprintf(stderr, NUM_ATTRIBUTE_PRINT, "ninodes", super->ninodes);
        fprintf(stderr, NUM_ATTRIBUTE_PRINT, "i_blocks", super->i_blocks);
        fprintf(stderr, NUM_ATTRIBUTE_PRINT, "z_blocks", super->z_blocks);
        fprintf(stderr, NUM_ATTRIBUTE_PRINT, "firstdata", super->firstdata);
        fprintf(stderr, NUM_ATTRIBUTE_PRINT, "log_zone_size", super->log_zone_size);
        fprintf(stderr, NUM_ATTRIBUTE_PRINT, "max_file", super->max_file);
        fprintf(stderr, HEX_ATTRIBUTE_PRINT, "magic", super->magic);
        fprintf(stderr, NUM_ATTRIBUTE_PRINT, "zones", super->zones);
        fprintf(stderr, NUM_ATTRIBUTE_PRINT, "blocksize", super->blocksize);
        fprintf(stderr, NUM_ATTRIBUTE_PRINT, "subversion", super->subversion);        
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

void print_part_table(int fd, off_t table_offset, int part, int subPart){
    struct partition_entry buf[NUM_PART];
    int i;

    if(lseek(fd, table_offset, SEEK_SET) < 0){
        perror(FILEERR);
        return;
    }

    if(read(fd, &buf, sizeof(struct partition_entry) * NUM_PART) < 0){
        perror(FILEERR);
        return;
    }

    if(subPart != NO_PART){
        fprintf(stderr, SUB_PART_PRINT, subPart);
    }else{
        fprintf(stderr, PART_PRINT);
    }

    for(i = 0; i < NUM_PART; i++){
        if(i == part){
            fprintf(stderr, ACTUAL_PART_ENTRY, i + 1);
        }else{
            fprintf(stderr, PART_ENTRY_PRINT, i + 1);
        }
        fprintf(stderr, NUM_ATTRIBUTE_PRINT, "bootind", buf[i].bootind);
        fprintf(stderr, NUM_ATTRIBUTE_PRINT, "start_head", buf[i].start_head);
        fprintf(stderr, NUM_ATTRIBUTE_PRINT, "start_sec", buf[i].start_sec);
        fprintf(stderr, NUM_ATTRIBUTE_PRINT, "start_cyl", buf[i].start_cyl);
        fprintf(stderr, NUM_ATTRIBUTE_PRINT, "type", buf[i].type);
        fprintf(stderr, NUM_ATTRIBUTE_PRINT, "end_head", buf[i].end_head);
        fprintf(stderr, NUM_ATTRIBUTE_PRINT, "end_sec", buf[i].end_sec);
        fprintf(stderr, NUM_ATTRIBUTE_PRINT, "end_cyl", buf[i].end_cyl);
        fprintf(stderr, NUM_ATTRIBUTE_PRINT, "lFirst", buf[i].lFirst);
        fprintf(stderr, NUM_ATTRIBUTE_PRINT, "size", buf[i].size);
    }
}