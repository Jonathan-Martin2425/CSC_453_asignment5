#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include "util.h"

#define OPTSTR "vp:s:"
#define USAGE "Usage: [ -v ] [ -p part [ -s subpart ] ] imagefile srcpath [ dstpath ]\n"
#define PARTERR "partition must be between 0-3"
#define SUBPARTERR "subpartition must be between 0-3"
#define NO_IMG "an image file must be provided"
#define MALLOCERR "Malloc error"
#define OPENERR "open error"
#define INITIALDISK 0
#define MAX_PART 4
#define DEF_PATH "/"
#define PERM_STRING 11
#define SIZE_STRING 10

int main(int argc, char *argv[]) {
    int option, path_len;
    extern int optind;
    extern char *optarg;
    int isV = FALSE, part = NO_PART, sub_part = NO_PART;
    char *image = NULL, *src = NULL, *dest_path = NULL;
    FILE *image_file, *dest, *test;
    uint32_t disk_start, part_size;
    struct superblock *superblock;
    struct inode found_file;
    off_t inode_table_offset;
    void *file_data;
    size_t i;

    /* parses all options using getopt and returns appropriately,
     * erroring if they are invalid in anyway */
    while ((option = getopt(argc, argv, OPTSTR)) != EOF) {
        switch (option)
        {
        case 'v':
            isV = TRUE;
            break;
        case 'p':
            part = strtol(optarg, NULL, 10);
            if (part < 0 || part >= MAX_PART) {
                perror(PARTERR);
                return EXIT_FAILURE;
            }
            break;
        case 's':
            sub_part = strtol(optarg, NULL, 10);
            if (sub_part < 0 || sub_part >= MAX_PART) {
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

    /* gets required image and srcpath arguments if they exist
       and exiting otherwise */
    if (argc > optind) {
        image = argv[optind];
        optind++;
    } else {
        printf(USAGE);
        return EXIT_FAILURE;
    }

    if (argc > optind) {
        /* allocates memory to copy src path
           string to new path string we
           will use strtok on in the
           "find_file" function */
        path_len = strlen(argv[optind]);
        if ((int)(src = (char*)malloc(path_len + 1)) < 0) {
            perror(MALLOCERR);
            return EXIT_FAILURE;
        }

        memset(src, 0, path_len + 1);
        strncpy(src, argv[optind], path_len);
        optind++;
    } else {
        printf(USAGE);
        return EXIT_FAILURE;
    }

    if(argc > optind){
        dest_path = argv[optind];
    }

    /* open image file and destination file 
       if neccesary, therefore checking if
       they are valid */
    if ((image_file = fopen(image, "r+w")) == NULL) {
        perror(OPENERR);
        return EXIT_FAILURE;
    }

    if(dest_path != NULL){
        if ((dest = fopen(dest_path, "w+")) == NULL) {
            fclose(image_file);
            perror(OPENERR);
            return EXIT_FAILURE;
        }
    }else{
        dest = stdout;
    }

    /* check if partitioning is required,
       if it is then find and verify if it
       exists and set the start of disk to that,
       otherwise set the start of disk as the start
       of the opened image file */
    if (part != NO_PART) {
        if (partition_finder(image, part, sub_part, 
            &disk_start, &part_size, isV) == EXIT_FAILURE) {
            fclose(image_file);
            fclose(dest);
            return EXIT_FAILURE;
        }
    } else {
        if (sub_part == NO_PART) {
            disk_start = INITIALDISK;
            part_size = INITIALDISK;
        } else {
            fclose(image_file);
            fclose(dest);
            printf(USAGE);
            return EXIT_FAILURE;
        }
    }

    /***  
       search and verify super block, then
       search starting from root by parsing
       path given to get inode of file
    ***/
    if (find_file(src, 
                 image_file,  
                 disk_start * SECTOR_SIZE, 
                 &found_file, 
                 isV) == EXIT_FAILURE) {
        fclose(image_file);
        fclose(dest);
        return EXIT_FAILURE;
    }

    /* MINGET specific
       reads file from found file inode
       then writes contents to destination path */

    /* check if file is a regular file before writing */
    if((found_file.mode & FILE_TYPE_MASK) != REG_MASK){
        fclose(image_file);
        fclose(dest);
        return EXIT_FAILURE;
    }
    
    if((file_data = read_file(image_file, 
                 &found_file, 
                 get_superblock(image_file, disk_start * SECTOR_SIZE, FALSE),
                 disk_start * SECTOR_SIZE)) == NULL){
        fclose(image_file);
        fclose(dest);
        return EXIT_FAILURE;
    }



    if(fwrite(file_data, 1, found_file.size, dest) != found_file.size){
        free(file_data);
        fclose(image_file);
        fclose(dest);
        return EXIT_FAILURE;
    }

    free(file_data);
    fclose(image_file);
    fclose(dest);
}