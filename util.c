
#include "util.h"

/* */
void *read_file(FILE *image, 
                struct inode *node, 
                struct superblock *super, 
                off_t disk_start){
    uint32_t zone_size = super->blocksize << super->log_zone_size;
    int num_zones, i;
    intptr_t res_offset;
    void *res;

    /* calculates zones allocated to file from given size
       by doing floor division on zone_size and rounding up*/
    num_zones = (node->size + zone_size - 1) / zone_size;

    /* allocates returning pointer with full 
       potential file size allocated */
    res = malloc((size_t)zone_size * num_zones);
    if(res == NULL){
        perror(MALLOCERR);
        return NULL;
    }

    /* checks if only direct zones need to be read*/
    if(num_zones <= DIRECT_ZONES){

        /* iterates through each direct zone, fseeks to it
           then reads it into the resulting buffer, never assuming
           each zone is sequencial */
        for(i = 0; i < num_zones; i++){
            if(fseek(image, disk_start + (zone_size * node->zone[i]), SEEK_SET) < 0){
                perror(FILEERR);
                free(res);
                return NULL;
            }

            res_offset = (intptr_t)res + zone_size * i;

            /* check for 0 zone, and still write full zone of 0's
                   to buffer instead */
            if(node->zone[i] == 0){
                memset((void*)res_offset, 0, zone_size);
                continue;
            }

            if(fread((void*)res_offset, zone_size, 1, image) <= 0){
                perror(READERR);
                free(res);
                return NULL;
            }
        }
    }else{
        /* read indirect zone and maybe double indirect too*/
    }

    return res;
}

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
        perror(SUPERERR);
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
    struct inode *inode_table, *cur_inode;
    uint32_t zone_size;
    void *file_zones;
    int potential_dir_entries, i, cur_dir_inode;

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

    cur_inode = inode_table + 0; /* redundant, but shows it is root*/

    file_zones = read_file(image, cur_inode, super, disk_start);
    if(file_zones == NULL){
        return EXIT_FAILURE;
    }

    /* temporary verbose option, goes through root DIR, assuming it is a dir,
       and prints each inode in the DIR along with its name */
    if(isV){
        potential_dir_entries = (cur_inode->size + 
                                    sizeof(struct dir_entry) - 1 ) /  
                                    sizeof(struct dir_entry);

        for(i = 0; i < potential_dir_entries; i++){
            cur_dir_inode = ((struct dir_entry*)file_zones)[i].inode;
            if(cur_dir_inode != 0 && cur_dir_inode <= super->ninodes){
                fprintf(stderr, STR_ATTRIBUTE_PRINT,
                     "name", 
                     ((struct dir_entry*)file_zones)[i].name);
                print_inode(inode_table[cur_dir_inode]);
            }
        }
    }

    free(file_zones);
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