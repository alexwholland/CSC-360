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
void checkArguments();

void diskput();

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
    	#elif defined(PART4)
		struct stat buf;
		int f = open(argv[2], O_RDONLY);
		if (fstat(f, &buf) < 0) {
			printf("File not found.\n");
			exit(EXIT_FAILURE);
		}
        	diskput(argc, argv, addr, buf);
		close(f);
/*    #elif defined(PART5)
        diskfix(argc, argv);
    #else
    #   error "PART[12345] must be defined";*/
    #endif
	close(fd);
        return 0;
}


void stripDirectory(char* argv[], int pos, char* tokens[]) {
	char* args = strtok(argv[pos], "/");
	int i = 0;
	while (args) {
		tokens[i++] = args;
		args = strtok(NULL, "/");
	}	
	tokens[i] = NULL;
}

void checkArguments(int argc, int argCount) {
	if (argc != argCount) {
		printf("Error: %d arguments were used, but %d were expected\n", argc, argCount);
		exit(EXIT_FAILURE);
	}	
}


/* Part 1: diskinfo
 * Purpose: 	Read the file system super block and use the 
 * 		information to read the FAT.
 * Parameters: 	int argc - argument count
 * 		char** argv - argument values
 * Returns:	void
 */
void diskinfo(int argc, char** argv, char* addr) {
	checkArguments(argc, 2);    	
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

int rootStartBlock(struct superblock_t* sb) {
	return ntohl(sb->root_dir_start_block) * htons(sb->block_size);
}

int newLocation(struct superblock_t* sb, int i) {
	return ((i - calculateStartBlock(sb)) / 4);
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
    	char* tokens[128];
    	int dirs = 0;
		
	checkArguments(argc, 3);
	stripDirectory(argv, 2, tokens);
	while (tokens[dirs] != NULL) {
		dirs++;
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
            		if (ntohl(root_block->size) == 0) {
		 		continue;      
			}
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




/*Part 3*/

void diskget(int argc, char* argv[], char* addr, struct stat buffer) {
	checkArguments(argc, 4);
	char* tokens[128];
  	stripDirectory(argv, 2, tokens);

    int block_size = 0, root_start = 0, fat_start = 0;
    memcpy(&block_size, addr + 8, 2);
    block_size = htons(block_size);
    memcpy(&root_start, addr + 24, 2);
    root_start = htons(root_start);
    memcpy(&fat_start, addr + 16, 2);
    fat_start = htons(fat_start);

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
               if (!strcmp(name, tokens[0])) {
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


void updateTime(void* address, int i, struct stat buf) {
			struct tm *tm;
            			char buff[10];
            			tm = localtime(&buf.st_mtime);
            			int k;

            			// Create time and modify time the same
			      	strftime(buff, sizeof(buff), "%Y", tm);
            			sscanf(buff, "%d", &k);
            			k = htons(k);
            			memcpy(address+i+20, &k, 2);
            			memcpy(address+i+13, &k, 2);

            			strftime(buff, sizeof(buff), "%m", tm);
            			sscanf(buff, "%d", &k);
            			memcpy(address+i+22, &k, 1);
            			memcpy(address+i+15, &k, 1);

            			strftime(buff, sizeof(buff), "%d", tm);
            			sscanf(buff, "%d", &k);
            			memcpy(address+i+23, &k, 1);
            			memcpy(address+i+16, &k, 1);

            			strftime(buff, sizeof(buff), "%H", tm);
            			sscanf(buff, "%d", &k);
            			memcpy(address+i+24, &k, 1);
            			memcpy(address+i+17, &k, 1);

            			strftime(buff, sizeof(buff), "%M", tm);

            			sscanf(buff, "%d", &k);
            			memcpy(address+i+25, &k, 1);
			        memcpy(address+i+18, &k, 1);

		            	strftime(buff, sizeof(buff), "%S", tm);
	            		sscanf(buff, "%d", &k);
	            		memcpy(address+i+26, &k, 1);
	            		memcpy(address+i+19, &k, 1);
	 


}

int put(int root_block, int block_size, void* address, struct stat buf, char* file_placement, int file_size, int needed_blocks, int starting_block, int fat_start_block, int fat_dir_location) {
   	int stat = 3;
  	int i;
   	int endf = 0xFFFFFFFF;
   	int endff = 0xFFFF;
   	while(1){
      		for (i = root_block; i < root_block + block_size; i += 64) {
         		int curr = 0;
         		memcpy(&curr, address + i, 1);
         		if (curr == 0){
            			memcpy(address+i, &stat, 1); // status

    			        starting_block = ntohl(starting_block);
        			memcpy(address+i+1, &starting_block, 4);

            			needed_blocks = htonl(needed_blocks);
            			memcpy(address+i+5, &needed_blocks, 4);

            			file_size = htonl(file_size);
            			memcpy(address+i+9, &file_size, 4);
 				
				updateTime(address, i, buf);
            		    
		    		strncpy(address+i+27, file_placement, 31);
	
	            		memcpy(address+i+58, &endf, 4);
	            		memcpy(address+i+62, &endff, 2);

            			return 0;
          	}
      }
      fat_dir_location = (root_block/block_size)*4 + (fat_start_block);
      memcpy(&root_block, address + fat_dir_location, 4);
      root_block = htonl(root_block);
      if (root_block == -1){ break; }
      root_block = root_block * block_size;
   }

   // return nonzero value if no entry was added, which is the case where the
   // directory is full
   return fat_dir_location;
}


void fullDirectory(struct superblock_t* sb, int fat_dir_location, char* address) {
	int end = 0xFFFFFFFF;
	for (int i = fat_dir_location; i < StartPlusEnd(sb); i += 4) {
        	int curr = -1;
        	memcpy(&curr, address + i, 4);
        	curr = htonl(curr);
        	if (curr == 0) {
           		int new = htonl(newLocation(sb, i));
           		memcpy(address + fat_dir_location, &new, 4);

           		memcpy(address + i, &end, 4);

           		int root_blocks = ntohl(ntohl(sb->root_dir_block_count) + 1);
           		memcpy(address+26, &root_blocks, 4);
           		break;
        	}
    	}
}


int allocateBlocks(int bufSize, int blockSize) {
	return bufSize % blockSize != 0 ? 
		(bufSize / blockSize) + 1 : 
		(bufSize / blockSize);
}

void diskput(int argc, char *argv[], char* address, struct stat buf) {
    	checkArguments(argc, 4);
	char* tokens[128];
	stripDirectory(argv, 3, tokens);
    	FILE* fp = fopen(argv[2], "rb");
	if (fp == NULL) {
		return;
	}

    struct superblock_t* sb;
    sb = (struct superblock_t*) address;

//    	int block_size = htons(sb->block_size);


    int blocks_used = 0;
    int end = 0xFFFFFFFF;
    int mem_location = 0, last_FAT = 0, starting_block = 0;
    char file_buffer[512];
    for (int i = calculateStartBlock(sb); i < StartPlusEnd(sb); i += 4) {
        int curr = -1;
        memcpy(&curr, address + i, 4);
        curr = htonl(curr);
        // found a free block
        if (curr == 0) {
           if (blocks_used == 0) {
               starting_block = newLocation(sb, i);
               last_FAT = i;
               blocks_used++;

               fread(file_buffer, htons(sb->block_size), 1, fp);
               memcpy(address+(starting_block*htons(sb->block_size)), &file_buffer, htons(sb->block_size));
               continue;
           }
           mem_location = newLocation(sb, i);
           fread(file_buffer, htons(sb->block_size), 1, fp);
           memcpy(address+(mem_location*htons(sb->block_size)), &file_buffer, htons(sb->block_size));

           int loc = htonl(mem_location);
           memcpy(address+last_FAT, &loc, 4);
           last_FAT = i;
           blocks_used++;

           if (blocks_used == allocateBlocks(buf.st_size, htons(sb->block_size))){
               memcpy(address+i, &end, 4);
               break;
           }
        }
    }


    // go to root dir to find open spot to add entry
    int fat_dir_location = 0;
    fat_dir_location = put(rootStartBlock(sb), htons(sb->block_size), address, buf, tokens[0], buf.st_size, allocateBlocks(buf.st_size, htons(sb->block_size)), starting_block, calculateStartBlock(sb), fat_dir_location);

    // on successful retrun, exit
    	if (!fat_dir_location) { 
		return; 
	}

	fullDirectory(sb, fat_dir_location, address);
    
    	put(rootStartBlock(sb), htons(sb->block_size), address, buf, tokens[0], buf.st_size, allocateBlocks(buf.st_size, htons(sb->block_size)), starting_block, calculateStartBlock(sb), fat_dir_location);

    	return;
}
