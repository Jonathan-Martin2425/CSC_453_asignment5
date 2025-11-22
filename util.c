#include <super.h>
#include <stdio.h>

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
        return super
    }else{
        free(super);
        return NULL;
    }
}

/* given the image file, start of disk and superblock,
   returns the offset to the inode table by calculating
   it from the superblock's blocksize, imap number
   of blocks and zmap number of blocks */
off_t get_inode_table(FILE *image, struct superblock *super, off_t disk_start){
    
}