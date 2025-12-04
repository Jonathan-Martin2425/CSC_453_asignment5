#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include "util.h"

#define OPTSTR "vp:s:"
#define USAGE "Usage: [ -v ] [ -p part [ -s subpart ] ] imagefile [ path ]\n"
#define DIR_PRINT "%s:\n"
#define PARTERR "partition must be between 0-3\n"
#define SUBPARTERR "subpartition must be between 0-3\n"
#define NO_IMG "an image file must be provided\n"
#define MALLOCERR "Malloc error\n"
#define OPENERR "open error\n"
#define INITIALDISK 0
#define MAX_PART 4
#define DEF_PATH "/"
#define SLASH '/'
#define PERM_STRING 12
#define SIZE_STRING 10

/* defineing all functions used in main before
   writing later for style purposes*/
void print_reg_file(struct inode *, char *);
void print_dir(struct dir_entry *, struct inode *, off_t, char *);
int canonicalizer(char *);

int main(int argc, char *argv[]) {
    int option, path_len;
    extern int optind;
    extern char *optarg;
    int isV = FALSE, part = NO_PART, sub_part = NO_PART;
    char *image = NULL, *min_path = NULL, *path_name;
    FILE *image_file;
    uint32_t disk_start, part_size;
    struct superblock *super;
    struct inode found_file, *inode_table;
    off_t inode_table_offset, possible_num_entries;
    struct dir_entry *dir_data;

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
                fprintf(stderr, PARTERR);
                return EXIT_FAILURE;
            }
            break;
        case 's':
            sub_part = strtol(optarg, NULL, 10);
            if (sub_part < 0 || sub_part >= MAX_PART) {
                fprintf(stderr, SUBPARTERR);
                return EXIT_FAILURE;
            }
            break;
        default:
            fprintf(stderr, USAGE);
            return EXIT_FAILURE;
            break;
        }
    }

    /* gets image file, exiting if not provided*/
    if (argc > optind) {
        image = argv[optind];
        optind++;
    } else {
        fprintf(stderr, USAGE);
        return EXIT_FAILURE;
    }

    /* gets path to list if given as
       a copy of the string for the 
       "find_file" function to use
       strtok on */
    if (argc > optind) {
        /* allocates memory to copy path
           string to new path string we
           will use strtok on */
        path_len = strlen(argv[optind]);
        if ((intptr_t)(min_path = (char*)malloc(path_len + 1)) < 0) {
            fprintf(stderr, MALLOCERR);
            return EXIT_FAILURE;
        }
        /* Making two copies to have one for printing with */
        if ((intptr_t)(path_name = (char*)malloc(path_len + 1)) < 0) {
            fprintf(stderr, MALLOCERR);
            return EXIT_FAILURE;
        }
        memset(min_path, 0, path_len + 1);
        memset(path_name, 0, path_len + 1);
        strncpy(min_path, argv[optind], path_len);
        strncpy(path_name, argv[optind], path_len);
        if(canonicalizer(path_name) == EXIT_FAILURE){
            return EXIT_FAILURE;
        }
    } else {
        min_path = DEF_PATH;
        path_name = DEF_PATH;
    }

    /* open image file, therefore checking if it
       is valid */
    if ((image_file = fopen(image, "r")) == NULL) {
        fprintf(stderr, OPENERR);
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
            fprintf(stderr, USAGE);
            return EXIT_FAILURE;
        }
    }

    /*
       search and verify super block, then
       search starting from root by parsing
       path given to get inode of file
    */
    if (find_file(min_path, 
                 image_file,  
                 disk_start * SECTOR_SIZE, 
                 &found_file, 
                 isV) == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }

    /* MINLS SPECIFIC 
       check type of file.
       if it is a directory, print information about each file in it,
       if it isn't print out information of file */
    if ((found_file.mode & FILE_TYPE_MASK) == DIR_MASK) {
        /* directory */

        /* get superblock to read file data and inode table*/
        super = get_superblock(image_file, disk_start * SECTOR_SIZE, FALSE);

        /* read dir entries using "read_file" function*/
        dir_data = (struct dir_entry*)read_file(image_file, 
                                                &found_file, 
                                                super,
                                                disk_start * SECTOR_SIZE);
        if(dir_data == NULL){
            return EXIT_FAILURE;
        }

        /* calculates the possible ammount of entires in
           the directory from the file inode size */
        possible_num_entries = (found_file.size + 
                                sizeof(struct dir_entry) - 1 ) /  
                                sizeof(struct dir_entry);

        /* get inode table offset from superblock,
           allocate space for it and read it */
        inode_table_offset = get_inode_table_start(super, 
                                                   disk_start * SECTOR_SIZE);
        inode_table = malloc(sizeof(struct inode) * super->ninodes);
        if(inode_table == NULL){
            fprintf(stderr, MALLOCERR);
            return EXIT_FAILURE;
        }
        if(fseek(image_file, inode_table_offset, SEEK_SET) < 0){
            fprintf(stderr, FILEERR);
            return EXIT_FAILURE;
        }
        if(fread((void*)inode_table, 
                sizeof(struct inode), 
                super->ninodes, image_file) <= 0){
            fprintf(stderr, READERR);
            return EXIT_FAILURE;
        }

        /* pass directory entries and inode table 
           to print out the inode of each file in
           the directory */
        print_dir(dir_data, inode_table, possible_num_entries, path_name);
    } else if ((found_file.mode & FILE_TYPE_MASK) == REG_MASK) {
        /* regular file */
        print_reg_file(&found_file, path_name);
    } else {
        fprintf(stderr, LS_TYPE_INVAL);
        return EXIT_FAILURE;
    }

    return 0;
}

/* Prints the given file in "[permissions] [size] [filename]" format. 
 * Assumes filename is null terminated */
void print_reg_file(struct inode *file, char *name) {
    char perms[PERM_STRING];
    char size[SIZE_STRING];
    char name_buf[NAME_SIZE];
    uint32_t file_size;
    perms_print(file->mode, perms);
    memset((void*)size, 0, SIZE_STRING);

    file_size = file->size;
    sprintf(size, "%9u", file_size);

    /* only copies the 1st 60 characters of name 
       since filenames are not NULL terminated
       sometimes */
    strncpy(name_buf, name, NAME_SIZE);
    if(name_buf[0] == SLASH){
        printf(REG_FILE_PRINT, perms, size, &(name[1]));
    }else{
        printf(REG_FILE_PRINT, perms, size, name);
    }
}

/* given the inode table and all entries in a directory,
   prints out the information of each inode in the directory*/
void print_dir(struct dir_entry *dir_data, 
               struct inode *inode_table,
               off_t num_entries, 
               char *name){
    int i;
    struct dir_entry *cur_entry;
    struct inode *cur_inode;
    printf(DIR_PRINT, name);

    /* iterates through DIR entries to get inode number,
       finds the inode in the inode table,
       then prints out the information if
       it is a regular file or directory */
    for(i = 0; i < num_entries; i++){
        
        /* get inode number*/
        cur_entry = (struct dir_entry*)((intptr_t)dir_data +
                     (sizeof(struct dir_entry) * i));

        /* deleted file */
        if(cur_entry->inode == 0) continue;

        /* get inode from inode table */
        cur_inode = (struct inode*)( (intptr_t)inode_table + 
                     sizeof(struct inode) * (cur_entry->inode - 1));
        
        /* print out information only if it is a regular
           file or directory, as we aren't printing out
           all entries in subdirectories */
        if(((cur_inode->mode & FILE_TYPE_MASK) == REG_MASK) ||
           ((cur_inode->mode & FILE_TYPE_MASK) == DIR_MASK)){
            print_reg_file(cur_inode, (char*)cur_entry->name);
        }
    }
}

/* Removes duplicate slashes, adds slash at 
 * beginning and removes slash from end */
int canonicalizer(char *original) {
    int i = 0, j = 0, length;
    char *copy, prev;
    length = strlen(original);
    copy = (char*)calloc(length + 2, sizeof(char));
    if(copy == NULL){
        fprintf(stderr, MALLOCERR);
        return EXIT_FAILURE;
    }
    
    /* ensure copy starts with a slash
       and both strings start at a point
       without a slash */
    copy[j++] = SLASH;
    if (original[i] == SLASH) {
        i++;
    } 
    
    /* copy one char at a time, dont copy if its a repeated slash */ 
    prev = SLASH;
    while (i < length) {
        /* only copies character if neither the previous
           or current character equal slash, therefore 
           only adding one slash per series of slashes*/
        if(original[i] != SLASH || prev != SLASH){
            prev = copy[j++] = original[i];
        }
        i++;
    }
    
    /* Remove slash if there, and add null term. */
    if (j > 1 && copy[j - 1] == SLASH) {
        j--;
    }
    copy[j] = '\0';

    strncpy(original, copy, j + 1);
    free(copy);
    return EXIT_SUCCESS;
}

