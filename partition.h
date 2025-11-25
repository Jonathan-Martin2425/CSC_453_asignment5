#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#define MBR_SIZE 512
#define PART_TABLE_OFFSET 0x1BE
#define PART_ENTRY_SIZE 16
#define SECTOR_SIZE 512
#define MINIX_TYPE 0x81

uint32_t uint32_convert(uint8_t *);
int partition_finder(char *, int, int, uint32_t *, uint32_t *);