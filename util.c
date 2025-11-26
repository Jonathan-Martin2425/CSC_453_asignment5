
#include "util.h"

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
        print_superblock(super);
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
off_t get_inode_table_start(struct superblock *super, off_t disk_start){
    int16_t block_size = super->blocksize;
    off_t inode_offset;

    /* inode table is calculated from start of disk + boot sector
       plus the blocks allocated by the superblock, inode bitmap
       and zone bitmap found in the superblock*/
    inode_offset = disk_start + 
                   block_size * (FIRST_BLOCKS + super->i_blocks + super->z_blocks);

    return inode_offset;
}

int find_file(char *path, FILE *image, off_t disk_start, int isV){
    struct superblock *super;
    off_t inode_offset;
    struct inode *inode_table;

    /* get superblock from start of disk*/
    super = get_superblock(image, disk_start, isV);
    if(super == NULL){
        return EXIT_FAILURE;
    }

    /* get inode table offset from superblock
       then read it to get it as an array of
       inodes */
    inode_offset = get_inode_table_start(super, disk_start);
    inode_table = malloc(sizeof(struct inode) * super->ninodes);
    if(inode_table == NULL){
        perror(MALLOCERR);
        return NULL;
    }
    if(fseek(image, inode_offset, SEEK_SET) < 0){
        perror(FILEERR);
        return NULL;
    }
    if(fread((void*)inode_table, sizeof(struct inode), super->ninodes, image) <= 0){
        perror(READERR);
        return NULL;
    }

    /* read the 1st inode(root), verify it is a DIR 
       and start recursivley searching for the file
       by parsing the path */
    if(isV){
        print_inode(inode_table[0]);
    }    

    free(super);
    free(inode_table);
}

void print_superblock(struct superblock *super){
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

void print_inode(struct inode node){
    char buf[MODE_PRINT_SIZE];
    int i;

    fprintf(stderr, INODE_NAME);
    strmode((mode_t)node.mode, buf);
    fprintf(stderr, MODE_ATTRIBUTE_PRINT, node.mode, buf);
    fprintf(stderr, NUM_ATTRIBUTE_PRINT, "uint16_t links", node.links);
    fprintf(stderr, NUM_ATTRIBUTE_PRINT, "uint16_t uid", node.uid);
    fprintf(stderr, NUM_ATTRIBUTE_PRINT, "uint16_t gid", node.gid);
    fprintf(stderr, NUM_ATTRIBUTE_PRINT, "uint32_t size", node.size);
    fprintf(stderr, TIME_ATTRIBUTE_PRINT, "uint32_t atime", node.atime, ctime((time_t*)&node.atime));
    fprintf(stderr, TIME_ATTRIBUTE_PRINT, "uint32_t mtime", node.mtime, ctime((time_t*)&node.mtime));
    fprintf(stderr, TIME_ATTRIBUTE_PRINT, "uint32_t ctime", node.ctime, ctime((time_t*)&node.ctime));

    /*zones */
    fprintf(stderr, ZONE_NAME, "Direct");
    for(i = 0; i < DIRECT_ZONES; i++){
        fprintf(stderr, ZONE_ATTRIBUTE_PRINT, i, node.zone[i]);
    }

    fprintf(stderr, NUM_ATTRIBUTE_PRINT, "uint32_t indirect", node.indirect);
    fprintf(stderr, NUM_ATTRIBUTE_PRINT, "uint32_t two_indirect", node.two_indirect);
}