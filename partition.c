#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include "util.h"

#define MBR_SIZE 512
#define PART_TABLE_OFFSET 0x1BE
#define PART_TABLE_SIG_OFFSET 510
#define VALID_PART_BYTE_ONE 0x55
#define VALID_PART_BYTE_TWO 0xAA
#define PART_ENTRY_SIZE 16
#define PART_TYPE_OFFSET 4
#define PART_LFIRST_OFFSET 8
#define PART_SIZE_OFFSET 12
#define SECTOR_SIZE 512
#define MINIX_TYPE 0x81

/* makes one 32 bit number out of four 8 bit chunks */
uint32_t uint32_convert(uint8_t *p) {
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

/* given the pararmeters of minls and minget
   tries to find the start of the partitioned disk.
   Then populated the offset and p_size parameters
   with the start of disk in sectors and part size
   respectivley */
int partition_finder(char *img, int part_num, int sub_part, 
                     uint32_t *offset, uint32_t *p_size, int isV) {
    uint8_t mbr[MBR_SIZE];
    uint8_t sub_mbr[MBR_SIZE];
    uint8_t part_type, sub_type;
    uint32_t first_sec, psize, sub_first_sec, sub_psize;
    int fd, r;
    size_t base;
    off_t location;

    /* opens disk image, returns if 
       image path is invalid */
    fd = open(img, O_RDONLY);
    if (fd < 0) {
        return EXIT_FAILURE;
    }
    
    /* read 1st 512 bytes into buffer */
    r = read(fd, mbr, MBR_SIZE);
    if (r != MBR_SIZE) {
        close(fd);
        fprintf(stderr, READ_ERR);
        return EXIT_FAILURE;
    }

    /* after reading, try to print out table*/
    if(isV){
        print_part_table(fd, PART_TABLE_OFFSET, part_num, NO_PART);
    }


    /* check if partition table has valid signature */
    if (mbr[PART_TABLE_SIG_OFFSET] != VALID_PART_BYTE_ONE || 
        mbr[PART_TABLE_SIG_OFFSET + 1] != VALID_PART_BYTE_TWO) {
        close(fd);
        fprintf(stderr, PSIG_INVAL);
        return EXIT_FAILURE;
    }

    /* check validity of partition number
       parameter */
    if (part_num < 0 || part_num > 3) {
        close(fd);
        fprintf(stderr, PNUM_INVAL);
        return EXIT_FAILURE;
    }
    
    /* find base of given partition */
    base = PART_TABLE_OFFSET + (part_num * PART_ENTRY_SIZE);
    /* this finds type field in partition struct */
    part_type = mbr[base + PART_TYPE_OFFSET];
    
    if (part_type != MINIX_TYPE) {
        close(fd);
        fprintf(stderr, TYPE_INVAL);
        return EXIT_FAILURE;
    }
    
    /* converting 32 bit ints from partition struct 
       returns with error if part size is 0 */ 
    first_sec = uint32_convert(&mbr[base + PART_LFIRST_OFFSET]);
    psize = uint32_convert(&mbr[base + PART_SIZE_OFFSET]);

    if (psize == 0) {
        close(fd);
        fprintf(stderr, SIZE_INVAL);
        return EXIT_FAILURE;
    }

    /* checks if a subpartition was given, then trys to
       search for subpartition too */
    if (sub_part != NO_PART) {

        /* check if subpartition number given
           is valid, since a MINIX file system
           is only partitioned in 4 partitions*/
        if (sub_part < 0 || sub_part > 3) {
            close(fd);
            fprintf(stderr, SPNUM_INVAL);
            return EXIT_FAILURE;
        }
        
        /* seek to subpartition's partition table
           from found partition table */ 
        location = (off_t)first_sec * SECTOR_SIZE;
        r = lseek(fd, location, SEEK_SET);
        
        if (r == (off_t)-1) {
            close(fd);
            fprintf(stderr, SP_TABLE_ERR);
            return EXIT_FAILURE;
        }
        
        /* read subpartition table */
        r = read(fd, sub_mbr, MBR_SIZE);
        if (r != MBR_SIZE) {
            close(fd);
            fprintf(stderr, READ_ERR);
            return EXIT_FAILURE;
        }

        /* after reading MBR, print potential subpartition table */
        if(isV){
            print_part_table(fd, location + PART_TABLE_OFFSET,
                             sub_part, part_num);
        }
        
        /* repeat steps to check validity of subpartition
           table and find start of disk and part size*/
        if (sub_mbr[PART_TABLE_SIG_OFFSET] != VALID_PART_BYTE_ONE || 
            sub_mbr[PART_TABLE_SIG_OFFSET + 1] != VALID_PART_BYTE_TWO) {
            close(fd);
            fprintf(stderr, SUB_MBR_INVAL);
            return EXIT_FAILURE;
        }

        base = PART_TABLE_OFFSET + sub_part * PART_ENTRY_SIZE;

        sub_type = sub_mbr[base + PART_TYPE_OFFSET];
        sub_first_sec = uint32_convert(&sub_mbr[base + PART_LFIRST_OFFSET]);
        sub_psize = uint32_convert(&sub_mbr[base + PART_SIZE_OFFSET]);
        
        if (sub_type != MINIX_TYPE) {
            close(fd);
            fprintf(stderr, TYPE_INVAL);
            return EXIT_FAILURE;
        }

        if (sub_psize == 0) {
            close(fd);
            fprintf(stderr, SIZE_INVAL);
            return EXIT_FAILURE;
        }

        /* return subpartition lfirst as start of disk */
        *offset = sub_first_sec;
        *p_size = sub_psize;


        close(fd);
        return EXIT_SUCCESS;
    }

    /* return partition lfirst as start of disk */
    *offset = first_sec;
    *p_size = psize;

    close(fd); 
    return EXIT_SUCCESS;
}

/* prints partition table or subpartition table
   depending on values of "part" and "subPart" */
void print_part_table(int fd, off_t table_offset, int part, int subPart){
    struct partition_entry buf[NUM_PART];
    int i;

    /* seek to table_offset given and read
       whole partition table */
    if(lseek(fd, table_offset, SEEK_SET) < 0){
        fprintf(stderr, FILEERR);
        return;
    }
    if(read(fd, &buf, sizeof(struct partition_entry) * NUM_PART) < 0){
        fprintf(stderr, FILEERR);
        return;
    }

    /* check if a subpartition value is given 
       to indicate whether to print partition
       as a regular or sub partition */
    if(subPart != NO_PART){
        fprintf(stderr, SUB_PART_PRINT, subPart);
    }else{
        fprintf(stderr, PART_PRINT);
    }

    /* print whole partition table, with an indicator for which
       partition was actually found/used */
    for(i = 0; i < NUM_PART; i++){
        if(i == part){
            fprintf(stderr, ACTUAL_PART_ENTRY, i + 1);
        }else{
            fprintf(stderr, PART_ENTRY_PRINT, i + 1);
        }
        fprintf(stderr, NUM_ATTRIBUTE_PRINT, "bootind", buf[i].bootind);
        fprintf(stderr, NUM_ATTRIBUTE_PRINT, "start_head", buf[i].start_head);
        fprintf(stderr, NUM_ATTRIBUTE_PRINT, "start_sec", buf[i].start_sec);
        fprintf(stderr, NUM_ATTRIBUTE_PRINT, "start_cyl", buf[i].start_cyl);
        fprintf(stderr, NUM_ATTRIBUTE_PRINT, "type", buf[i].type);
        fprintf(stderr, NUM_ATTRIBUTE_PRINT, "end_head", buf[i].end_head);
        fprintf(stderr, NUM_ATTRIBUTE_PRINT, "end_sec", buf[i].end_sec);
        fprintf(stderr, NUM_ATTRIBUTE_PRINT, "end_cyl", buf[i].end_cyl);
        fprintf(stderr, NUM_ATTRIBUTE_PRINT, "lFirst", buf[i].lFirst);
        fprintf(stderr, NUM_ATTRIBUTE_PRINT, "size", buf[i].size);
        fprintf(stderr, NEW_LINE);
    }
}
