#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>

/* Structures implemented from Tutorial 8 slides.
 * "__attribute__((__packed__))" is required to ensure
 * compiler does not  optimize for byte alignment.
 */

// Supper Block Struct
struct __attribute__((__packed__)) superblock_t {
    uint8_t fs_id[8];
    uint16_t block_size;
    uint32_t file_system_block_count;
    uint32_t fat_start_block;
    uint32_t fat_block_count;
    uint32_t root_dir_start_block;
    uint32_t root_dir_block_count;
};


// Time and Date Entry Struct
struct __attribute__((__packed__)) dir_entry_timedate_t {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

// Directory Entry Struct
struct __attribute__((__packed__)) dir_entry_t {
    uint8_t status;
    uint32_t starting_block;
    uint32_t block_count;
    uint32_t size;
    struct dir_entry_timedate_t create_time;
    struct dir_entry_timedate_t modify_time;
    uint8_t filename[31];
    uint8_t unused[6];
};


/*Function Prototypes*/
void diskinfo();
void printBlockInfo();
int calculateStartBlock();
int calculateEndBlock();
int StartPlusEnd();

int main(int argc, char* argv[]) {
	/*Logic implemented from Tutorial 8 slides*/
    	#if defined(PART1)
        	diskinfo(argc, argv);
  /*  #elif defined(PART2)
        disklist(argc, argv);
    #elif defined(PART3)
        diskget(argc, argv);
    #eliif defined(PART4)
        diskput(argc, argv);
    #elif defined(PART5)
        diskfix(argc, argv);*/
    #else
    #   error "PART[12345] must be defined"
    #endif
        return 0;
}


void diskinfo(int argc, char** argv) {
    	if (argc < 2) {
        	printf("Error");
        	exit(1);
    	}

	int fd = open(argv[1], O_RDWR);
	struct stat fileStats;

	fstat(fd, &fileStats);

	char* data = mmap(NULL, fileStats.st_size, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);
	struct superblock_t* superBlock = (struct superblock_t*) data;
	
	int FATcount[3] = {0, 0, 0};
	for (int i = calculateStartBlock(superBlock); i < StartPlusEnd(superBlock); i += 4) {
        	int temp = 0;
        	memcpy(&temp, data + i, 4);
		htonl(temp) == 0 ? FATcount[0]++ : htonl(temp) == 1 ? 
			FATcount[1]++ : FATcount[2]++;
    	}

    	printBlockInfo(superBlock, FATcount);

   
	close(fd);
}

int calculateStartBlock(struct superblock_t* superBlock) {
	return ntohl(superBlock->fat_start_block) * htons(superBlock->block_size);
}

int calculateEndBlock(struct superblock_t* superBlock) {
	return ntohl(superBlock->fat_block_count) * htons(superBlock->block_size);
}

int StartPlusEnd(struct superblock_t* superBlock) {
	return calculateStartBlock(superBlock) + calculateEndBlock(superBlock);
}
void printBlockInfo(struct superblock_t* superBlock, int FATcount[]) {
    	printf("Super block information:\n");
    	printf("Block size: %d\n", htons(superBlock->block_size));
  	printf("Block count: %d\n", ntohl(superBlock->file_system_block_count));
    	printf("FAT starts: %d\n", ntohl(superBlock->fat_start_block));
    	printf("FAT blocks: %d\n", ntohl(superBlock->fat_block_count));
    	printf("Root directory start: %d\n", ntohl(superBlock->root_dir_start_block));
    	printf("Root directory blocks: %d\n\n", ntohl(superBlock->root_dir_block_count));
	printf("FAT information:\n");
   	printf("Free Blocks: %d\n", FATcount[0]);
    	printf("Reserved Blocks: %d\n", FATcount[1]);
    	printf("Allocated Blocks: %d\n", FATcount[2]);

}

