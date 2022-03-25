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

/*Part 1*/
void diskinfo();
void printBlockInfo();
int calculateStartBlock();
int calculateEndBlock();
int StartPlusEnd();

/*Part 2*/
void disklist();
void printDirectory();

int main(int argc, char* argv[]) {
	/*Logic implemented from Tutorial 8 slides*/
    	#if defined(PART1)
        	diskinfo(argc, argv);
    #elif defined(PART2)
        disklist(argc, argv);
/*    #elif defined(PART3)
        diskget(argc, argv);
    #elif defined(PART4)
        diskput(argc, argv);
    #elif defined(PART5)
        diskfix(argc, argv);
    #else
    #   error "PART[12345] must be defined";*/
    #endif
        return 0;
}


/* Part 1: diskinfo
 * Purpose: 	Read the file system super block and use the 
 * 		information to read the FAT.
 * Parameters: 	int argc - argument count
 * 		char** argv - argument values
 * Returns:	void
 */
void diskinfo(int argc, char** argv) {
    	if (argc < 2) {
        	printf("No disk image found\n");
        	exit(1);
    	}

	int fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		printf("Error reading disk image\n");
	}

	struct stat buffer;
	fstat(fd, &buffer);
	char* data = mmap(NULL, buffer.st_size, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);
	if (data == MAP_FAILED) {
		printf("Could not mmap\n");
		exit(1);
	}
	
	struct superblock_t* sb = (struct superblock_t*) data;
	int FATcount[3] = {0};
	for (int i = calculateStartBlock(sb); i < StartPlusEnd(sb); i += 4) {
        	int temp = 0;
        	memcpy(&temp, data + i, 4);
		htonl(temp) == 0 ? FATcount[0]++ : htonl(temp) == 1 ? 
			FATcount[1]++ : FATcount[2]++;
    	}

    	printBlockInfo(sb, FATcount);

	close(fd);
}

int calculateStartBlock(struct superblock_t* sb) {
	return ntohl(sb->fat_start_block) * htons(sb->block_size);
}

int calculateEndBlock(struct superblock_t* sb) {
	return ntohl(sb->fat_block_count) * htons(sb->block_size);
}

int StartPlusEnd(struct superblock_t* sb) {
	return calculateStartBlock(sb) + calculateEndBlock(sb);
}

/* Purpose: 	Print the formatted output.
 * Parameters:	struct superblock_t* sb - the superblock
 * 		int FATcount[] - tracks the blocks in the FAT
 * 				index 0 - amount of free blocks
 * 				index 1 - amount of reserved blocks
 * 				index 2 - amount of allocated blocks
 * Returns:	void
 */
void printBlockInfo(struct superblock_t* sb, int FATcount[]) {
    	printf("Super block information:\n");
    	printf("Block size: %d\n", htons(sb->block_size));
  	printf("Block count: %d\n", ntohl(sb->file_system_block_count));
    	printf("FAT starts: %d\n", ntohl(sb->fat_start_block));
    	printf("FAT blocks: %d\n", ntohl(sb->fat_block_count));
    	printf("Root directory start: %d\n", ntohl(sb->root_dir_start_block));
    	printf("Root directory blocks: %d\n\n", ntohl(sb->root_dir_block_count));
	printf("FAT information:\n");
   	printf("Free Blocks: %d\n", FATcount[0]);
    	printf("Reserved Blocks: %d\n", FATcount[1]);
    	printf("Allocated Blocks: %d\n", FATcount[2]);

}

/*Part 2*/
void disklist(int argc, char** argv) {
	char* dirName = argv[2];
    
    // Tokenize the input directory
    	char* tokens[128];
    	char* directory = argv[2];
    	int dirs = 0;
    
  	if (argc == 3) {
		char* args = strtok(directory, "/");
		int i = 0;
		while (args) {
			tokens[i++] = args;
			args = strtok(NULL, "/");
			dirs++;	
		}
		tokens[i] = NULL;
    }
    
    // Open and assemble the disk image
    int fd = open(argv[1], O_RDWR);
    struct stat fileStats;

    fstat(fd, &fileStats);

    char* data = mmap(NULL, fileStats.st_size, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);
    struct superblock_t* superBlock = (struct superblock_t*) data;

    int blockSize, rootStart = 0;
    blockSize = htons(superBlock->block_size);
    rootStart = ntohl(superBlock->root_dir_start_block) * blockSize;

    struct dir_entry_t* root_block;

    int offSet = rootStart;
    int curr = 0;
    if (argc == 3 && strcmp(dirName, "/")) {
        while(1) {
            for (int i = offSet; i < offSet + blockSize; i += 64) {
                root_block = (struct dir_entry_t*) (data+i);
                const char* name = (const char*)root_block->filename;
                if (!strcmp(name, tokens[curr])) {
                    curr++;
                    offSet = ntohl(root_block->starting_block) * blockSize;
                    if (curr == dirs || !strcmp(dirName, "/")) {
                        for (int j = offSet; j < offSet + blockSize; j += 64) {
                            root_block = (struct dir_entry_t*) (data + j);
                            if (ntohl(root_block->size) == 0) continue;
                          	printDirectory(root_block);
                        }
                        return;
                    }
                    break;
                }
            }
        }
   }
    
    if (argc == 2 || curr == dirs || !strcmp(dirName, "/")) {
        for (int i = rootStart; i <= rootStart+blockSize; i += 64) {
            root_block = (struct dir_entry_t*) (data+i);
            if (ntohl(root_block->size) == 0) continue;
    		printDirectory(root_block);
        }
    }    
}




void printDirectory(struct dir_entry_t* root_block) {
            printf("%s %10d %30s %4d/%02d/%02d %02d:%02d:%02d\n",
		root_block->status == 5 ? "D" : "F",  
		ntohl(root_block->size),
		root_block->filename,	    
		htons(root_block->modify_time.year),
		root_block->modify_time.month,
		root_block->modify_time.day,
		root_block->modify_time.hour,
		root_block->modify_time.minute,
		root_block->modify_time.second); 
}
