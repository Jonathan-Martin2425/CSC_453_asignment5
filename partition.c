#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include "partition.h"
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

int partition_finder(char *img, int part_num, int sub_part, 
                     uint32_t *offset, uint32_t *p_size, int isV) {
    uint8_t mbr[MBR_SIZE];
    uint8_t sub_mbr[MBR_SIZE];
    uint8_t part_type, sub_type;
    uint32_t first_sec, psize, sub_first_sec, sub_psize;
    int fd, r;
    size_t base;
    off_t location;

    fd = open(img, O_RDONLY);
    if (fd < 0) {
        return EXIT_FAILURE;
    }
    
    /* read 512 bytes into buffer */
    r = read(fd, mbr, MBR_SIZE);
    if (r != MBR_SIZE) {
        close(fd);
        perror("Error reading from file");
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
        perror("Partition table does not have valid signature");
        return EXIT_FAILURE;
    }

    /* check validity of partition number */
    if (part_num < 0 || part_num > 3) {
        close(fd);
        perror("Invalid partition number");
        return EXIT_FAILURE;
    }
    
    /* find base of given partition */
    base = PART_TABLE_OFFSET + (part_num * PART_ENTRY_SIZE);
    /* this finds type field in partition struct */
    part_type = mbr[base + PART_TYPE_OFFSET];
    
    if (part_type != MINIX_TYPE) {
        close(fd);
        perror("Partition is not of type MINIX");
        return EXIT_FAILURE;
    }
    
    /* getting 32 bit ints from partition struct */ 
    first_sec = uint32_convert(&mbr[base + PART_LFIRST_OFFSET]);
    psize = uint32_convert(&mbr[base + PART_SIZE_OFFSET]);

    if (psize == 0) {
        close(fd);
        perror("Partition size is too small");
        return EXIT_FAILURE;
    }

    /* -1 == no subpartitions, 0-3 == looking for subpartitions */
    if (sub_part != -1) {
        if (sub_part < 0 || sub_part > 3) {
            close(fd);
            perror("Invalid subpartition number");
            return EXIT_FAILURE;
        }
        
        /* traverse to subpartition's partition table */ 
        if(isV){
            printf("\nfirst_sec: %d\n\n", first_sec);
        }
        location = (off_t)first_sec * SECTOR_SIZE;
        r = lseek(fd, location, SEEK_SET);
        
        if (r == (off_t)-1) {
            close(fd);
            perror("Error finding subpartition table");
            return EXIT_FAILURE;
        }
        
        /* read subpartition table */
        r = read(fd, sub_mbr, MBR_SIZE);
        if (r != MBR_SIZE) {
            close(fd);
            perror("Error reading from file");
            return EXIT_FAILURE;
        }

        /* after reading MBR, print potential subpartition table */
        if(isV){
            print_part_table(fd, location + PART_TABLE_OFFSET, sub_part, part_num);
        }
        
        if (sub_mbr[PART_TABLE_SIG_OFFSET] != VALID_PART_BYTE_ONE || 
            sub_mbr[PART_TABLE_SIG_OFFSET + 1] != VALID_PART_BYTE_TWO) {
            close(fd);
            perror("Invalid SUB MBR signature");
            return EXIT_FAILURE;
        }

        base = PART_TABLE_OFFSET + sub_part * PART_ENTRY_SIZE;

        sub_type = sub_mbr[base + PART_TYPE_OFFSET];
        sub_first_sec = uint32_convert(&sub_mbr[base + PART_LFIRST_OFFSET]);
        sub_psize = uint32_convert(&sub_mbr[base + PART_SIZE_OFFSET]);
        
        if (sub_type != MINIX_TYPE) {
            close(fd);
            perror("Subpartition is not of type MINIX");
            return EXIT_FAILURE;
        }

        if (sub_psize == 0) {
            close(fd);
            perror("Subpartition size is invalid size");
            return EXIT_FAILURE;
        }

        *offset = first_sec + sub_first_sec;
        *p_size = sub_psize;

        close(fd);
        return EXIT_SUCCESS;
    }

    *offset = first_sec;
    *p_size = psize;

    close(fd); 
    return EXIT_SUCCESS;
}
