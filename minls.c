#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include "util.h"

#define OPTSTR "vp:s:"
#define USAGE "Usage: [ -v ] [ -p part [ -s subpart ] ] imagefile [ path ]\n"
#define PARTERR "partition must be between 0-3"
#define SUBPARTERR "subpartition must be between 0-3"
#define NO_IMG "an image file must be provided"
#define MALLOCERR "Malloc error"
#define OPENERR "open error"
#define INITIALDISK 0
#define MAX_PART 4
#define DEF_PATH "/"

int main(int argc, char *argv[]){
    int option, path_len;
    extern int optind;
    extern char *optarg;
    int isV = FALSE, part = NO_PART, sub_part = NO_PART;
    char *image = NULL, *min_path = NULL;
    FILE *image_file;
    uint32_t disk_start, part_size;
    struct superblock *superblock;
    struct inode found_file;
    off_t inode_table_offset;

    /* parses all options using getopt and
       if they are invalid in anyway, it errors
       and returns appropriately*/
    while((option = getopt(argc, argv, OPTSTR)) != EOF){
        switch (option)
        {
        case 'v':
            isV = TRUE;
            break;
        case 'p':
            part = strtol(optarg, NULL, 10);
            if(part < 0 || part >= MAX_PART){
                perror(PARTERR);
                return EXIT_FAILURE;
            }
            break;
        case 's':
            sub_part = strtol(optarg, NULL, 10);
            if(sub_part < 0 || sub_part >= MAX_PART){
                perror(SUBPARTERR);
                return EXIT_FAILURE;
            }
            break;
        default:
            printf(USAGE);
            return EXIT_FAILURE;
            break;
        }
    }

    /* gets other two arguments if they exist
       and exiting if no image file is provided */
    if(argc > optind){
        image = argv[optind];
        optind++;
    }else{
        printf(USAGE);
        return EXIT_FAILURE;
    }

    if(argc > optind){
        /* allocates memory to copy path
           string to new path string we
           will use strtok on */
        path_len = strlen(argv[optind]);
        if((int)(min_path = (char*)malloc(path_len + 1)) < 0){
            perror(MALLOCERR);
            return EXIT_FAILURE;
        }
        memset(min_path, 0, path_len + 1);
        strncpy(min_path, argv[optind], path_len);
    }else{
        min_path = DEF_PATH;
    }

    /* open image file, therefore checking if it
       is valid */
    if((image_file = fopen(image, "r+w")) == NULL){
        perror(OPENERR);
        return EXIT_FAILURE;
    }

    /* check if partitioning is required,
       if it is then find and verify if it
       exists and set the start of disk to that,
       otherwise set the start of disk as the start
       of the opened image file */
    if(part != NO_PART){
        if(partition_finder(image, part, sub_part, &disk_start, &part_size, isV) == EXIT_FAILURE){
            return EXIT_FAILURE;
        }
    }else{
        if(sub_part == NO_PART){
            disk_start = INITIALDISK;
            part_size = INITIALDISK;
        }else{
            printf(USAGE);
            return EXIT_FAILURE;
        }
    }

    /***  
       search and verify super block, then
       search starting from root by parsing
       path given to get inode of file
    ***/
    if(find_file(min_path, 
                 image_file,  
                 disk_start * SECTOR_SIZE, 
                 &found_file, 
                 isV) == EXIT_FAILURE){
        return EXIT_FAILURE;
    }

    /* MINLS SPECIFIC 
       get type of file and other information about it.
       if it is a directory, print information about each file in it,
       if it isn't print out information of file */
    if((found_file.mode & FILE_TYPE_MASK) == DIR_MASK){
        print_dir(found_file);
    }else{
        print_reg_file(found_file);
    }

    free(min_path);
    return 0;
}

void print_reg_file(struct inode file){

}

void print_dir(struct inode file){
    
}