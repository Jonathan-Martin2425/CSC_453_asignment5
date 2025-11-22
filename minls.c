#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define OPTSTR "vp:s:"
#define USAGE "Usage: [ -v ] [ -p part [ -s subpart ] ] imagefile [ path ]\n"
#define PARTERR "partition must be between 0-3"
#define SUBPARTERR "subpartition must be between 0-3"
#define NO_IMG "an image file must be provided"
#define MALLOCERR "Malloc error"
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

    if(image){
        printf(PRTVAR, "image", image);
    }

    if(min_path){
        printf(PRTVAR, "min_path", min_path);
    }

    free(min_path);
    return 0;
}