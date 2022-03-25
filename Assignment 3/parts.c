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

int diskget();

int main(int argc, char* argv[]) {
	/*Logic implemented from Tutorial 8 slides*/
    	#if defined(PART1)
        	diskinfo(argc, argv);
    #elif defined(PART2)
        disklist(argc, argv);
    #elif defined(PART3)
        diskget(argc, argv);
/*    #elif defined(PART4)
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
	if (fd < 0) {
		printf("Error reading disk image\n");
	}

    struct stat buffer;
    fstat(fd, &buffer);
    char* data = mmap(NULL, buffer.st_size, PROT_WRITE|PROT_READ, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
	printf("Colud not mmap\n");
	exit(1);
    }
    struct superblock_t* superBlock = (struct superblock_t*) data;

    int blockSize, rootStart = 0;
    blockSize = htons(superBlock->block_size);
    rootStart = ntohl(superBlock->root_dir_start_block) * blockSize;

    struct dir_entry_t* root_block;

    int curr = 0;
    	if (argc == 3 && strcmp(dirName, "/")) {
		for (int i = 0; i < dirs; i++) {
            		for (int j = rootStart; j < rootStart + blockSize; j += 64) {
             			root_block = (struct dir_entry_t*) (data+j);
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


struct superblock_t sb;
/*Part 3*/

int diskget(int argc, char* argv[]) {
	char* directory = argv[2];
	char* tokens[128];
	char* args = strtok(directory, "/");
	int i = 0;
	while (args) {
		tokens[i++] = args;
		args = strtok(NULL, "/");
	}
	tokens[i] = NULL;
//	printf("%s", tokens[0]);
//	printf("%s", tokens[1]);

if (argc != 4) {
        printf("Error: Incorrect usage. Needs 4 arguments\n");
        return 0;
    }
    char *file_name;
    char *file_placement;
    file_name = tokens[i-1];
    file_placement = argv[3];

    int fd = open(argv[1], O_RDWR);
    struct stat buffer;
    if (fstat(fd, &buffer) < 0) {
        perror("FSTAT ERROR");
        return 1;
    }
    char *address = mmap(NULL, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    int block_size = 0, root_start = 0, root_blocks = 0, fat_start = 0, fat_blocks = 0;
    memcpy(&block_size, address + 8, 2);
    block_size = htons(block_size);
    memcpy(&root_start, address + 24, 2);
    root_start = htons(root_start);
    memcpy(&root_blocks, address + 28, 2);
    root_blocks = htons(root_blocks);
    memcpy(&fat_start, address + 16, 2);
    fat_start = htons(fat_start);
    memcpy(&fat_blocks, address + 20, 2);
    fat_blocks = htons(fat_blocks);

    int root_block = root_start * block_size;

    while(1){
        int i;
        for (i = root_block; i < root_block + block_size; i += 64) {
           int curr = 0;
           memcpy(&curr, address + i, 1);

           // if we found a file entry
           if (curr == 3) {
               char name[31];
               memcpy(&name, address + i + 27, 31);

               // compare names, if the same, the file has been found
               if (!strcmp(name, file_name)) {
                   FILE *fp;
                   fp = fopen(file_placement, "wb");

                   int current_block = 0, num_blocks = 0, fat_address = 0;

                   // find how many blocks it needs
                   memcpy(&num_blocks, address + i + 5, 4);
                   num_blocks = htonl(num_blocks);

                   // printf("Current block address: %x\n", i);
                   memcpy(&current_block, address + i + 1, 4);
                   current_block = htonl(current_block);

                   int file_size;
                   memcpy(&file_size, address+i+9, 4);
                   file_size = htonl(file_size);
                   int j;
                   // for each needed block -1, copy block contents in block_size intervals
                   for (j = 0; j < num_blocks-1; j++) {
                       fwrite(address + (block_size * current_block), 1, block_size, fp);
                       fat_address = fat_start * block_size + current_block * 4;

                       memcpy(&curr, address + fat_address, 4);
                       curr = htonl(curr);
                       current_block = curr;
                   }

                   // determine remaining size left to copy
                   int remaining = file_size % block_size;

                   // if the remaining is 0, then it's a full block_size multiple
                   if (remaining == 0) {
                      // to copy the last contents
                      remaining = block_size;
                   }

                   // write remaining information
                   fwrite(address + (block_size * current_block), 1, remaining, fp);
                   fat_address = fat_start * block_size + current_block * 4;
                   memcpy(&curr, address + fat_address, 4);

                   munmap(address, buffer.st_size);
                   close(fd);
                   printf("Getting %s from %s and writing as %s.\n", file_name, argv[1], file_placement);
                   return 0;
               }
           }
        }
        int fat_dir_location = (root_block/block_size)*4 + (fat_start * block_size);
        memcpy(&root_block, address + fat_dir_location, 4);
        root_block = htonl(root_block);
        if (root_block == -1){ break; }
        root_block = root_block * block_size;
    }

    printf("File not found.\n");
    munmap(address, buffer.st_size);
    close(fd);
    return 0;	
}
