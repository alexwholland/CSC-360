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

void diskget();

int main(int argc, char* argv[]) {
	int fd = open(argv[1], O_RDWR);
	struct stat buffer;

	if (fd == -1) {
		printf("Error\n");
		exit(EXIT_FAILURE);
	}
	if (fstat(fd, &buffer) == -1) {
		printf("Error\n");
		exit(EXIT_FAILURE);
	}	

	char* addr = mmap(NULL, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if (addr == MAP_FAILED) {
		printf("Error");
		exit(EXIT_FAILURE);
	}
	/*Logic implemented from Tutorial 8 slides*/
    	#if defined(PART1)
        	diskinfo(argc, argv, addr);
    	#elif defined(PART2)
        	disklist(argc, argv, addr);
    	#elif defined(PART3)
       	 	diskget(argc, argv, addr, buffer);
/*    #elif defined(PART4)
        diskput(argc, argv);
    #elif defined(PART5)
        diskfix(argc, argv);
    #else
    #   error "PART[12345] must be defined";*/
    #endif
	close(fd);
        return 0;
}


/* Part 1: diskinfo
 * Purpose: 	Read the file system super block and use the 
 * 		information to read the FAT.
 * Parameters: 	int argc - argument count
 * 		char** argv - argument values
 * Returns:	void
 */
void diskinfo(int argc, char** argv, char* addr) {
    	
	struct superblock_t* sb = (struct superblock_t*) addr;
	int FATcount[3] = {0};
	for (int i = calculateStartBlock(sb); i < StartPlusEnd(sb); i += 4) {
        	int temp = 0;
        	memcpy(&temp, addr + i, 4);
		htonl(temp) == 0 ? FATcount[0]++ : htonl(temp) == 1 ? 
			FATcount[1]++ : FATcount[2]++;
    	}

    	printBlockInfo(sb, FATcount);
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
void disklist(int argc, char** argv, char* addr) {
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
    
    struct superblock_t* superBlock = (struct superblock_t*) addr;

    int blockSize, rootStart = 0;
    blockSize = htons(superBlock->block_size);
    rootStart = ntohl(superBlock->root_dir_start_block) * blockSize;

    struct dir_entry_t* root_block;

    int curr = 0;
    	if (argc == 3 && strcmp(dirName, "/")) {
		for (int i = 0; i < dirs; i++) {
            		for (int j = rootStart; j < rootStart + blockSize; j += 64) {
             			root_block = (struct dir_entry_t*) (addr+j);
                		const char* name = (const char*)root_block->filename;
                		if (!strcmp(name, tokens[curr])) {
                    			curr++;
                    			rootStart = ntohl(root_block->starting_block) * blockSize;
                                	break;
                		}
     			}	
		}
   }
    
    if (argc == 2 || curr == dirs || !strcmp(dirName, "/")) {
        for (int i = rootStart; i <= rootStart+blockSize; i += 64) {
            root_block = (struct dir_entry_t*) (addr+i);
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

/*
void copyInfo(int* block_size, int* root_start, int* root_blocks, int* fat_start, int* fat_blocks) {
	memcpy(&block_size, addr + 8, 2);
  	block_size = htons(block_size);
	memcpy(&root_start, addr + 24, 2);
	root_start = htons(root_start);
	memcpy(&root_blocks, addr + 28, 2);
	root_blocks = htons(root_blocks);
	memcpy(&fat_start, addr + 16, 2);
    	fat_start = htons(fat_start);
    	memcpy(&fat_blocks, addr + 20, 2);
    	fat_blocks = htons(fat_blocks);


}*/

/*Part 3*/

void diskget(int argc, char* argv[], char* addr, struct stat buffer) {
if (argc != 4) {
        printf("Error: Incorrect usage. Needs 4 arguments\n");
        return;
    }
   
    int block_size = 0, root_start = 0, root_blocks = 0, fat_start = 0;
    memcpy(&block_size, addr + 8, 2);
    block_size = htons(block_size);
    memcpy(&root_start, addr + 24, 2);
    root_start = htons(root_start);
    memcpy(&root_blocks, addr + 28, 2);
    root_blocks = htons(root_blocks);
    memcpy(&fat_start, addr + 16, 2);
    fat_start = htons(fat_start);
  //  memcpy(&fat_blocks, addr + 20, 2);
   // fat_blocks = htons(fat_blocks);

    int root_block = root_start * block_size;

    while(1){
        for (int i = root_start * block_size; i < (root_start*block_size) + block_size; i += 64) {
           int curr = 0;
           memcpy(&curr, addr + i, 1);

           // if we found a file entry
           if (curr == 3) {
               char name[31];
               memcpy(&name, addr + i + 27, 31);

               // compare names, if the same, the file has been found
               if (!strcmp(name, argv[2])) {
                   FILE* fp = fopen(argv[3], "wb");

                   int current_block = 0, num_blocks = 0, fat_address = 0;

                   // find how many blocks it needs
                   memcpy(&num_blocks, addr + i + 5, 4);
                   num_blocks = htonl(num_blocks);

                  
		   // printf("Current block address: %x\n", i);
                   memcpy(&current_block, addr + i + 1, 4);
                   current_block = htonl(current_block);

                   int file_size;
                   memcpy(&file_size, addr+i+9, 4);
                   file_size = htonl(file_size);
                   // for each needed block -1, copy block contents in block_size intervals
                   for (int j = 0; j < num_blocks-1; j++) {
                       fwrite(addr + (block_size * current_block), 1, block_size, fp);
                       fat_address = fat_start * block_size + current_block * 4;

                       memcpy(&curr, addr + fat_address, 4);
                       curr = htonl(curr);
                       current_block = curr;
                   }

                   // determine remaining size left to copy
                   int remaining = file_size % block_size;

                   // write remaining information
                   fwrite(addr + (block_size * current_block), 1, remaining == 0 ? block_size : remaining, fp);
                   fat_address = fat_start * block_size + current_block * 4;
                   memcpy(&curr, addr + fat_address, 4);

                   munmap(addr, buffer.st_size);
                   fclose(fp);
                   printf("Getting %s from %s and writing as %s.\n", argv[2], argv[1], argv[3]);
                   return;
               }
           }
        }
        memcpy(&root_block, addr + ((root_block/block_size)*4 + (fat_start * block_size)), 4);
        root_block = htonl(root_block);
        if (root_block == -1){ break; }
        root_block = root_block * block_size;
    }

    printf("File not found.\n");
    munmap(addr, buffer.st_size);
    return;
} 
