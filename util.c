
#include "util.h"

int read_zone(FILE *image, 
              off_t disk_start, 
              uint32_t zone_size, 
              uint32_t zone, 
              void *buf){

    /* check for 0 zone, and still write full zone of 0's
       to buffer instead */
    if(zone == 0){
        memset((void*)buf, 0, zone_size);
        return EXIT_SUCCESS;
    }

    /* seek to correct zone from given information*/
    if(fseek(image, disk_start + (zone_size * zone), SEEK_SET) < 0){
        perror(FILEERR);
        return EXIT_FAILURE;
    }

    /* read into the buffer given assuming 
       it has at least zone_size space in it*/
    if(fread((void*)buf, zone_size, 1, image) <= 0){
        perror(READERR);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/* given an inode and neccessary information to traverse
   the MINIX file system, it creates and returns a pointer
   to an allocated block of all data in said file.
   On error returns NULL */
void *read_file(FILE *image, 
                struct inode *node, 
                struct superblock *super, 
                off_t disk_start){
    int num_zones, i, cur_zone, indirect_num_zones, double_table_index;
    intptr_t res_offset;
    void *res;
    uint32_t *indirect_zone_table = NULL, *double_indirect_table = NULL;
    uint32_t zone_size = super->blocksize << super->log_zone_size;

    /* checks if file has a valid size before trying to read it*/
    if(node->size > super->max_file){
        perror(TOOBIG);
        return NULL;
    }

    /* calculates how many zone numbers an indirect zone block
       holds from the blocksize, rounding down */
    indirect_num_zones = super->blocksize / sizeof(uint32_t);

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

    for(i = 0; i < num_zones; i++){
        /* check for which zone number to take from, being either
           from the direct, indirect or double_indirect zones*/
        if(i < DIRECT_ZONES){
            cur_zone = node->zone[i];
        }else if(i < indirect_num_zones + DIRECT_ZONES){

            /* checks if the indirect table zone has been read yet
               then reads it if it hasn't before continuing*/
            if(indirect_zone_table == NULL){
                indirect_zone_table = malloc(zone_size);
                if(indirect_zone_table == NULL){
                    free(res);
                    perror(MALLOCERR);
                    return NULL;
                }

                if(read_zone(image,
                        disk_start, 
                        zone_size,
                        node->indirect, 
                        indirect_zone_table) == EXIT_FAILURE){
                    free(res);
                    free(indirect_zone_table);
                    return NULL;
                }
            }

            cur_zone = indirect_zone_table[i - DIRECT_ZONES];
        }else{

            /* if double_indirect table hasn't been read,
               read it and initialize index */
            if(double_indirect_table == NULL){
                double_indirect_table = malloc(zone_size);
                if(double_indirect_table == NULL){
                    free(res);
                    free(indirect_zone_table);
                    perror(MALLOCERR);
                    return NULL;
                }

                if(read_zone(image,
                        disk_start, 
                        zone_size,
                        node->two_indirect, 
                        double_indirect_table) == EXIT_FAILURE){
                    free(res);
                    free(indirect_zone_table);
                    free(double_indirect_table);
                    return NULL;
                }

                double_table_index = 0;
            }

            /* if needed, read new table from zone in double indirect table*/
            if((i - DIRECT_ZONES) % indirect_num_zones == 0){
                /* free previous table, then check if the index is too large*/
                free(indirect_zone_table);
                if(double_table_index >= indirect_num_zones){
                    /* should realistically never happen
                       due to the max_file in the superblock*/
                    perror(TOOBIG);
                    free(res);
                    free(double_indirect_table);
                    return NULL;
                }

                /* allocate zone to read double indirect table 
                   into then read it using the value in
                   the double indirect table index */
                indirect_zone_table = malloc(zone_size);
                if(indirect_zone_table == NULL){
                    free(res);
                    free(double_indirect_table);
                    perror(MALLOCERR);
                    return NULL;
                }
                if(read_zone(image,
                        disk_start, 
                        zone_size,
                        double_indirect_table[double_table_index], 
                        indirect_zone_table) == EXIT_FAILURE){
                    free(res);
                    free(indirect_zone_table);
                    free(double_indirect_table);
                    return NULL;
                }

                /* increment double indirect table index for next iteration */
                double_table_index++;
            }

            cur_zone = indirect_zone_table[(i - DIRECT_ZONES) %
                                            indirect_num_zones];
        }

        /* calculate next to zone to read into result
           then read zone */
        res_offset = (intptr_t)res + zone_size * i;
        if(read_zone(image,
                     disk_start, 
                     zone_size,
                     cur_zone, 
                     (void*)res_offset) == EXIT_FAILURE){
            free(res);
            free(indirect_zone_table);
            free(double_indirect_table);
            return NULL;
        }
    }

    free(indirect_zone_table);
    free(double_indirect_table);
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
                   block_size * (FIRST_BLOCKS + 
                                 super->i_blocks + super->z_blocks);

    return inode_offset;
}

int find_file(char *path, 
              FILE *image, 
              off_t disk_start,
              struct inode *res, 
              int isV){
    struct superblock *super;
    struct inode *inode_table, *cur_inode;
    struct dir_entry *entry;
    off_t inode_offset;
    uint32_t zone_size;
    void *file_zones;
    int potential_dir_entries, i, found_entry;
    char *token;

    /* invalid usage */
    if(res == NULL) return EXIT_FAILURE;

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
        free(super);
        return EXIT_FAILURE;
    }
    if(fseek(image, inode_offset, SEEK_SET) < 0){
        perror(FILEERR);
        free(super);
        return EXIT_FAILURE;
    }
    if(fread((void*)inode_table, 
             sizeof(struct inode), 
             super->ninodes, image) <= 0){
        perror(READERR);
        free(super);
        return EXIT_FAILURE;
    }

    cur_inode = inode_table; /* setting cur_inode to root*/

    /* search through each token/dir in path
       until the last token is found and erroring
       otherwise */
    token = strtok(path, PATH_DELIM);
    while(token != NULL){
        /* extra slashes case, so go onto next token*/
        if(strlen(token) == 0){
            token = strtok(NULL, PATH_DELIM);
            continue;
        }

        /* check if cur_inode is a DIR to continue search */
        if((cur_inode->mode & FILE_TYPE_MASK) != DIR_MASK){
            perror(DIRERR);
            free(super);
            free(inode_table);
            return EXIT_FAILURE;
        }

        /* read cur_inode datazones to start DIR search*/
        file_zones = read_file(image, cur_inode, super, disk_start);
        if(file_zones == NULL){
            free(super);
            free(inode_table);
            return EXIT_FAILURE;
        }

        /* start searching through each DIR entry and check if the name
           is the same as the token*/
        potential_dir_entries = (cur_inode->size + 
                                    sizeof(struct dir_entry) - 1 ) /  
                                    sizeof(struct dir_entry);
        found_entry = FALSE;
        for(i = 0; i < potential_dir_entries; i++){
            entry = (struct dir_entry*)((intptr_t)file_zones +
                     (sizeof(struct dir_entry) * i));

            /* deleted file*/
            if(entry->inode == 0){
                continue;
            }
            
            /* inode that is too large */
            if(entry->inode > super->ninodes){
                perror(INODEERR);
                free(file_zones);
                free(super);
                free(inode_table);
                return EXIT_FAILURE;

            }

            /* inode with same name as token */
            if (strncmp(token, (char*)entry->name, NAME_SIZE) == 0) {
                cur_inode = (struct inode*)((intptr_t)inode_table + 
                        sizeof(struct inode) * (entry->inode - 1));
                found_entry = TRUE;
                free(file_zones);
                break;
            }
        }

        /* check if the file is found before continuing
           to the next file */
        if(!found_entry){
            perror(FILENOTFOUNDERR);
            free(file_zones);
            free(super);
            free(inode_table);
            return EXIT_FAILURE;
        }
        token = strtok(NULL, PATH_DELIM);
    }

    /* sets result to res and print out verbose option*/
    if(isV){
        print_inode(*cur_inode);
    }
    *res = *cur_inode;

    free(super);
    free(inode_table);
}

void print_superblock(struct superblock *super){
    fprintf(stderr, SUP_NAME);
    fprintf(stderr, NUM_ATTRIBUTE_PRINT, "ninodes", super->ninodes);
    fprintf(stderr, NUM_ATTRIBUTE_PRINT, "i_blocks", super->i_blocks);
    fprintf(stderr, NUM_ATTRIBUTE_PRINT, "z_blocks", super->z_blocks);
    fprintf(stderr, NUM_ATTRIBUTE_PRINT, "firstdata", super->firstdata);
    fprintf(stderr, NUM_ATTRIBUTE_PRINT, 
            "log_zone_size", super->log_zone_size);
    fprintf(stderr, NUM_ATTRIBUTE_PRINT, "max_file", super->max_file);
    fprintf(stderr, HEX_ATTRIBUTE_PRINT, "magic", super->magic);
    fprintf(stderr, NUM_ATTRIBUTE_PRINT, "zones", super->zones);
    fprintf(stderr, NUM_ATTRIBUTE_PRINT, "blocksize", super->blocksize);
    fprintf(stderr, NUM_ATTRIBUTE_PRINT, "subversion", super->subversion);
    fprintf(stderr, NEW_LINE);   
}

void print_inode(struct inode node){
    char buf[MODE_PRINT_SIZE];
    int i;

    fprintf(stderr, INODE_NAME);
    perms_print(node.mode, buf);
    fprintf(stderr, MODE_ATTRIBUTE_PRINT, node.mode, buf);
    fprintf(stderr, NUM_ATTRIBUTE_PRINT, "uint16_t links", node.links);
    fprintf(stderr, NUM_ATTRIBUTE_PRINT, "uint16_t uid", node.uid);
    fprintf(stderr, NUM_ATTRIBUTE_PRINT, "uint16_t gid", node.gid);
    fprintf(stderr, NUM_ATTRIBUTE_PRINT, "uint32_t size", node.size);
    fprintf(stderr, TIME_ATTRIBUTE_PRINT, "uint32_t atime",
         node.atime, ctime((time_t*)&node.atime));
    fprintf(stderr, TIME_ATTRIBUTE_PRINT, "uint32_t mtime",
         node.mtime, ctime((time_t*)&node.mtime));
    fprintf(stderr, TIME_ATTRIBUTE_PRINT, "uint32_t ctime",
         node.ctime, ctime((time_t*)&node.ctime));

    /* zones */
    fprintf(stderr, ZONE_NAME, "Direct");
    for(i = 0; i < DIRECT_ZONES; i++){
        fprintf(stderr, ZONE_ATTRIBUTE_PRINT, i, node.zone[i]);
    }

    fprintf(stderr, NUM_ATTRIBUTE_PRINT,
         "uint32_t indirect", node.indirect);
    fprintf(stderr, NUM_ATTRIBUTE_PRINT, 
        "uint32_t two_indirect", node.two_indirect);
    fprintf(stderr, NEW_LINE);
}

void perms_print(uint16_t mode, char *buf) {

    buf[0] = ((mode & FILE_TYPE_MASK) == DIR_MASK) ? 'd' : '-';

    buf[1] = (mode & OWNER_READ)  ? 'r' : '-';

    buf[2] = (mode & OWNER_WRITE) ? 'w' : '-';

    buf[3] = (mode & OWNER_EXEC)  ? 'x' : '-';

    buf[4] = (mode & GROUP_READ)  ? 'r' : '-';

    buf[5] = (mode & GROUP_WRITE) ? 'w' : '-';

    buf[6] = (mode & GROUP_EXEC)  ? 'x' : '-';

    buf[7] = (mode & OTHER_READ)  ? 'r' : '-';

    buf[8] = (mode & OTHER_WRITE) ? 'w' : '-';

    buf[9] = (mode & OTHER_EXEC)  ? 'x' : '-';

    buf[10] = '\0';

}
