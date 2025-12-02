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
#define PERM_STRING 11
#define SIZE_STRING 10

int main(int argc, char *argv[]) {
    int option, path_len;
    extern int optind;
    extern char *optarg;
    int isV = FALSE, part = NO_PART, sub_part = NO_PART;
    char *image = NULL, *min_path = NULL, *path_name;
    FILE *image_file;
    uint32_t disk_start, part_size;
    struct superblock *superblock;
    struct inode found_file;
    off_t inode_table_offset;

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

    /* gets other two arguments if they exist
       and exiting if no image file is provided */
    if (argc > optind) {
        image = argv[optind];
        optind++;
    } else {
        printf(USAGE);
        return EXIT_FAILURE;
    }

    if (argc > optind) {
        /* allocates memory to copy path
           string to new path string we
           will use strtok on */
        path_len = strlen(argv[optind]);
        if ((int)(min_path = (char*)malloc(path_len + 1)) < 0) {
            perror(MALLOCERR);
            return EXIT_FAILURE;
        }
        /* Making two copies to have one for printing with */
        if ((int)(path_name = (char*)malloc(path_len + 1)) < 0) {
            perror(MALLOCERR);
            return EXIT_FAILURE;
        }
        memset(min_path, 0, path_len + 1);
        memset(path_name, 0, path_len + 1);
        strncpy(min_path, argv[optind], path_len);
        strncpy(path_name, argv[optind], path_len);
        canonicalizer(path_name);
    } else {
        min_path = DEF_PATH;
        path_name = DEF_PATH;
    }

    /* open image file, therefore checking if it
       is valid */
    if ((image_file = fopen(image, "r+w")) == NULL) {
        perror(OPENERR);
        return EXIT_FAILURE;
    }

    /* check if partitioning is required,
       if it is then find and verify if it
       exists and set the start of disk to that,
       otherwise set the start of disk as the start
       of the opened image file */
    if (part != NO_PART) {
        if (partition_finder(image, part, sub_part, 
            &disk_start, &part_size, isV) == EXIT_FAILURE) {
            return EXIT_FAILURE;
        }
    } else {
        if (sub_part == NO_PART) {
            disk_start = INITIALDISK;
            part_size = INITIALDISK;
        } else {
            printf(USAGE);
            return EXIT_FAILURE;
        }
    }

    /***  
       search and verify super block, then
       search starting from root by parsing
       path given to get inode of file
    ***/
    if (find_file(min_path, 
                 image_file,  
                 disk_start * SECTOR_SIZE, 
                 &found_file, 
                 isV) == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }

    /* MINLS SPECIFIC 
       get type of file and other information about it.
       if it is a directory, print information about each file in it,
       if it isn't print out information of file */
    if ((found_file.mode & FILE_TYPE_MASK) == DIR_MASK) {
        print_dir(found_file, path_name);
    } else if ((found_file.mode & FILE_TYPE_MASK) == REG_MASK) {
        print_reg_file(found_file, path_name);
    } else {
        perror(LS_TYPE_INVAL);
        return EXIT_FAILURE;
    }

    free(min_path);
    return 0;
}

/* Prints the given file in "[permissions] [size] [filename]" format. 
 * Assumes filename is null terminated */
void print_reg_file(struct inode file, char *name) {
    char perms[PERM_STRING];
    char size[SIZE_STRING];
    uint32_t file_size;
    /* MAKE SURE YOU GET RID OF THE LEADING SLASH IF PRINTING A REGULAR FILE
 *
 *
 *
 *
 *
 *
 * MAKE SURE YOU GET RID OF THE LEADING SLASH IF PRINTING A REGULAR FILE
 *
 * */
    perms_print(file.mode, perms);

    file_size = file.size;
    sprintf(size, "%9u", file_size);
    size[9] = '\0';

    printf(REG_FILE_PRINT, perms, size, name);
}

void print_dir(struct inode file, char *name) {
    
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

/* Removes duplicate slashes, adds slash at 
 * beginning and removes slash from end */
void canonicalizer(char *original) {
    int i = 0, j = 0, length;
    char *copy, prev;
    length = strlen(original);
    copy = calloc(length + 2, sizeof(char));
    
    /* ensure both strings start with a slash */
    if (original[i] != '/') {
        copy[j++] = '/';
    } else {
        i++;
        j++;
    }
   
    /* copy one char at a time, dont copy if its a repeated slash */ 
    prev = '/';
    while (i < length) {
        if (!(original[i] == '/' && prev == '/')) {
            prev = copy[j++] = original[i];
        }
        i++;
    }
    
    /* Remove slash if there, and add null term. */
    if (j > 1 && copy[j - 1] == '/') {
        j--;
    }
    copy[j] = '\0';

    strncpy(original, copy, j + 1);
    free(copy);
}

