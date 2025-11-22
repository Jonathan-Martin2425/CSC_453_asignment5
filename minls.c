#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include "util.c"

#define OPTSTR "vp:s:"
#define USAGE "Usage: [ -v ] [ -p part [ -s subpart ] ] imagefile [ path ]\n"
#define PARTERR "partition must be between 0-3"
#define SUBPARTERR "subpartition must be between 0-3"
#define NO_IMG "an image file must be provided"
#define MALLOCERR "Malloc error"
#define OPENERR "open error"
#define PRTVAR "%s: %s\n"
#define NO_PART -1
#define MAX_PART 4

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

int main(int argc, char *argv[]){
    int option, path_len;
    extern int optind;
    extern char *optarg;
    int isV = FALSE, part = NO_PART, sub_part = NO_PART;
    char *image = NULL, *min_path = NULL;
    FILE *image_file;
    uint32_t disk_start;
    struct superblock *superblock;

    /* parses all options using getopt and
       if they are invalid in anyway, it errors
       and returns appropriately*/
    while((option = getopt(argc, argv, OPTSTR)) != EOF){
        switch (option)
        {
        case 'v':
            printf("option -%c wit arg %s\n", option, optarg);
            isV = TRUE;
            break;
        case 'p':
            printf("option -%c wit arg %s\n", option, optarg);
            part = strtol(optarg, NULL, 10);
            if(part < 0 || part >= MAX_PART){
                perror(PARTERR);
                return EXIT_FAILURE;
            }
            break;
        case 's':
            printf("option -%c wit arg %s\n", option, optarg);
            part = strtol(optarg, NULL, 10);
            if(part < 0 || part >= MAX_PART){
                perror(SUBPARTERR);
                return EXIT_FAILURE;
            }
            break;
        case '?':
            perror(USAGE);
            return EXIT_FAILURE;
            break;
        default:
            printf(USAGE);
            return 0;
            break;
        }
    }

    /* gets other two arguments if they exist
       and exiting if no image file is provided */
    if(argc > optind){
        image = argv[optind];
        optind++;
    }else{
        perror(NO_IMG);
        printf(USAGE);
        return EXIT_FAILURE;
    }

    if(argc > optind){
        /* allocates memory to copy path
           string to new path string we
           will use strtok on */
        path_len = strlen(argv[optind]);
        if((min_path = (char*)malloc(path_len + 1)) < 0){
            perror(MALLOCERR);
            return EXIT_FAILURE;
        }
        memset(min_path, 0, path_len + 1);
        strncpy(min_path, argv[optind], path_len);
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

    /* search and verify for super block 
       and search starting from root by parsing
       path given */
       superblock = get_superblock(image_file, disk_start);
       if(superblock == NULL) return NULL;

    /* MINLS SPECIFIC 
       get type of file and other information about it.
       if it is a directory, print information about each file in it,
       if it isn't print out information of file */



    /* prints out image and min_path variables if they exist
      for testing purposes */
    if(image){
        printf(PRTVAR, "image", image);
    }

    if(min_path){
        printf(PRTVAR, "min_path", min_path);
    }

    free(min_path);
    return 0;
}