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

#define SECTORSIZE 512

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
void fileNotFound();

/*Part 3*/
void diskget();
void checkArguments();

/*Part 4*/
void diskput();

/*Part 5*/
void diskfix();


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
			fileNotFound(addr, buf);
			exit(EXIT_FAILURE);
		}
        	diskput(argc, argv, addr, buf);
		close(f);
    #elif defined(PART5)
        diskfix(argc, argv);
    #else
    #   error "PART[12345] must be defined";
    #endif
	close(fd);
        return 0;
}


/* Purpose: 	Strip the forward slash '/' from the input
 * Parameters: 	char* argv[] - the input
 *		int pos - the argument number
 *		char* tokens[] - tokenized arguments
 * Returns:	void 
 */
void stripDirectory(char* argv[], int pos, char* tokens[]) {
	char* args = strtok(argv[pos], "/");
	int i = 0;
	while (args) {
		tokens[i++] = args;
		args = strtok(NULL, "/");
	}	
	tokens[i] = NULL;
}


/* Purpose:	Checks if the correct number of arguments were used
 * Parameters: 	int argc - the provided number of arguments
 * 		int argCount - the expected number of arguments
 * Returns: 	void
 */
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


/*Calculate the blocks*/
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

/*Calculate the location/addresses*/
int newLocation(struct superblock_t* sb, int i) {
	return ((i - calculateStartBlock(sb)) / 4);
}

int calculateFATaddr(int FAT_s, int blockSize, int curBlock) {
	return FAT_s * blockSize + curBlock * 4;
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

/* Purpose: 	Display the cont of the directory
 * Parameters:	int argc - # of arguments
 * 		int** argv - the inputted arguments
 * Returns:	void
 */
void disklist(int argc, char* argv[], char* addr) {
	char* dirName = argv[2];
    	char* tokens[128];
    	int dirs = 0;
		
	checkArguments(argc, 3);
	stripDirectory(argv, 2, tokens);
	while (tokens[dirs] != NULL) {
		dirs++;
	}

    	struct superblock_t* sb = (struct superblock_t*) addr;
	int rootStart = rootStartBlock(sb);
    	struct dir_entry_t* root_block;
    	int count = 0;
    	if (argc == 3 && strcmp(dirName, "/")) {
		for (int i = 0; i < dirs; i++) {
            		for (int j = rootStart; j < rootStart + htons(sb->block_size); j += 64) {
             			root_block = (struct dir_entry_t*) (addr+j);
                		const char* name = (const char*)root_block->filename;
                		if (!strcmp(name, tokens[count])) {
                    			count++;
                    			rootStart = ntohl(root_block->starting_block) 
						* htons(sb->block_size);
                                	break;
                		}
     			}	
		}
   	}
    
    	if (argc == 2 || count == dirs || !strcmp(dirName, "/")) {
        	for (int i = rootStart; i <= rootStart + htons(sb->block_size); i += 64) {
            		root_block = (struct dir_entry_t*) (addr+i);
            		if (ntohl(root_block->size) == 0) {
		 		continue;      
			}
    			printDirectory(root_block);
        	}			
    	}    
}


/* Purpose: 	Print the status of the directory
 * Parameters: 	struct dir_entry_t* root_block - the information
 * Returns:	void
 */
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


/* Purpose: Copy the block contents to a new location
 * Parameters:
 * Returns int current_block
 */
int copyBlockContents(int block_size, int num_blocks, void* addr, int current_block, FILE* fp, int fat_start, int curr) {
	int fat_address = 0;
	for (int i = 0; i < num_blocks-1; i++) {
        	fwrite(addr + (block_size * current_block), 1, block_size, fp);
		fat_address = calculateFATaddr(fat_start, block_size, current_block);
                memcpy(&curr, addr + fat_address, 4);
                curr = htonl(curr);
                current_block = curr;
      	}
	return current_block;
}


/*Indicate that the file has not been found and deletes the mappings*/
void fileNotFound(void* addr, struct stat buffer) {
	printf("File not found.\n");
	munmap(addr, buffer.st_size);
}


int getHtonsSize(int i, void* addr, int size) {
	int block = 0;
	memcpy(&block, addr + i, size);
	return htons(block);
}

int getHtonlSize(int i, void* addr, int size) {
	int block = 0;
	memcpy(&block, addr + i, size);
	return htonl(block);
}


/* Purpose: 	Writes content to the new file 
 * Parameters: 	void* addr - the adress
 *		int i - location
 *		FILE* fp - the file
 *		int bSize = the block size
 *		int sFAT - start of the FAT block 
 *		int pos - the current location to be copied too
 * Returns: 	void	
 */
void fileFound(void* addr, int i, FILE* fp, int bSize, int sFAT, int pos) {
  	int fat_address = 0;
	int num_blocks = getHtonlSize(i+5, addr, 4);
	int current_block = getHtonlSize(i+1, addr, 4);
        int file_size = getHtonlSize(i+9, addr, 4);

	current_block = copyBlockContents(bSize, num_blocks, 
	addr, current_block, fp, sFAT, pos);
                  
        fwrite(addr + (bSize * current_block), 
		1, file_size % bSize == 0 ? 
		bSize : file_size % bSize, fp);
	fat_address = calculateFATaddr(sFAT, bSize, current_block);
        memcpy(&pos, addr + fat_address, 4);
}


/* Purpose: 	Copies from the addr into the root block.
 * Parameters:	int* root - the root block to be copied to	
 *		int bSize - the block size
 *		int sFAT - the FAT starting block
 *		void* addr - the address
 *		int mode - determines which function is calling copyInfo	
 * Returns: 	0 - No file/contents found
 * 		1 - File/contents found
 */
int copyInfo(int* root, int bSize, int sFAT, void* addr, int mode) {
	if (mode == 1) {
      		memcpy(&root, addr + ((*root/bSize)*4) + (sFAT*bSize), 4);
	} 
      *root = htonl(*root);
      if (*root == -1){ 
	      return 0; 
      }
      *root = *root * bSize;
      return 1;

}

/*Part 3*/

/* Purpose: 	Copies a file form the file ystem to the current local directory.
 * Parameters: 	int argc - argument count
 * 		char* argv[] - user inputted arguments
 * 		char* addr - the address
 * 		struct stat buffer - fstat() information from the disk image
 * Returns: 	void 
 */
void diskget(int argc, char* argv[], char* addr, struct stat buffer) {
	checkArguments(argc, 4);
	char* tokens[128];
  	stripDirectory(argv, 2, tokens);

	int root_block = getHtonsSize(24, addr, 2) * getHtonsSize(8, addr, 2);

    	while(1){
        	for (int i = root_block; i < root_block + getHtonsSize(8, addr, 2); i += 64) {
           		int count = 0;
           		memcpy(&count, addr + i, 1);
           		if (count == 3) {
               			char name[31];
               			memcpy(&name, addr + i + 27, 31);
               			if (!strcmp(name, tokens[0])) {
                   			FILE* fp = fopen(argv[3], "wb");
					if (fp == NULL) {
						return;
					}
					fileFound(addr, i, fp, getHtonsSize(8, addr, 2), 
							getHtonsSize(16, addr, 2), count);
                   			munmap(addr, buffer.st_size);
                   			fclose(fp);
                   			return;
               			}
           		}
        	}
		if (copyInfo(&root_block, getHtonsSize(8, addr, 2), 
					getHtonsSize(16, addr, 2), addr, 1) == 0) {
			break;
		}	
    	}
	fileNotFound(addr, buffer);
    	return;
}


/*updateTime() helper function, copies created and modified time*/
void date(void* addr, int i, int k, char buff[]) {
	sscanf(buff, "%d", &k);
	memcpy(addr + i, &k, 1);
	memcpy(addr + (i-7), &k, 1);
}


/* Purpose: 	Update the created and modified time. 
 * Parameters: 	void* addr - the address
 * 		int i - block location	
 * 		struct stat buf - time information
 * Returs:	void 
 */	
void updateTime(void* addr, int i, struct stat buf) {
    	char dSize[10];
	
	strftime(dSize, sizeof(dSize), "%Y", localtime(&buf.st_mtime));
	int src;
        sscanf(dSize, "%d", &src);
        src = htons(src);
        memcpy(addr+i+20, &src, 2);
        memcpy(addr+i+13, &src, 2);

        strftime(dSize, sizeof(dSize), "%m", localtime(&buf.st_mtime));
	date(addr, i+22, src, dSize);

        strftime(dSize, sizeof(dSize), "%d", localtime(&buf.st_mtime));
        date(addr, i+23, src, dSize);

        strftime(dSize, sizeof(dSize), "%H", localtime(&buf.st_mtime));
        date(addr, i+24, src, dSize);

        strftime(dSize, sizeof(dSize), "%M", localtime(&buf.st_mtime));
        date(addr, i+25, src, dSize);

	strftime(dSize, sizeof(dSize), "%S", localtime(&buf.st_mtime));
	date(addr, i+26, src, dSize);
}


void getSize(int block, int i, void* addr, int size) {
	block = ntohl(block);
	memcpy(addr + i, &block, size);
}


void newBlock(void* addr, int i, int size) {
	int hex = 0xFFFFFFFF;
	memcpy(addr + i, &hex, size);
}


void endBlock(void* addr, int i, int size) {
	int hex = 0xFFFF;
	memcpy(addr + i, &hex, size);
}	


/* Purpose: 	Puts the contents from the current directory into the file system
 * Returns: 	0 - success
 *		1 - No space
 */
int putInfo(int block, int bSize, void* addr, struct stat buf, char* fPos, int fSize, int bCount, int firstB, int firstFATB, int locFAT) {
	int jump = 3;
   	while(1){
      		for (int i = block; i < block + bSize; i += 64) {
         		int count = 0;
         		memcpy(&count, addr + i, 1);
         		if (count == 0){
            			memcpy(addr+i, &jump, 1);
				getSize(firstB, i+1, addr, 4);
				getSize(bCount, i+5, addr, 4);
				getSize(fSize, i+9, addr, 4);
				updateTime(addr, i, buf);
		    		strncpy(addr+i+27, fPos, 31);
				newBlock(addr, i+58, 4);
				endBlock(addr, i + 62, 2);

            			return 0;
          	}
      }
	locFAT = (block/bSize)*4 + (firstFATB);
	memcpy(&block, addr + locFAT, 4);
	if (copyInfo(&block, bSize, locFAT, addr, 0) == 0) {
		break;
	}
   }
   return locFAT;
}


/* Purpose: 	Find new sectors in the disk (Sometimes buggy and has caused seg faults)
 * Parameters:	struct superBlock_t* sb - super block information
 *		int locFAT - FAT location
 *		char* addr - the address
 * Returns:	void
 */
void fullDirectory(struct superblock_t* sb, int locFAT, char* addr) {
	for (int i = locFAT; i < StartPlusEnd(sb); i += 4) {
        	int count = -1;
        	memcpy(&count, addr + i, 4);
        	count = htonl(count);
        	if (count == 0) {
           		int allocate = htonl(newLocation(sb, i));
           		memcpy(addr + locFAT, &allocate, 4);
			newBlock(addr, i, 4);
           		int root = ntohl(ntohl(sb->root_dir_block_count) + 1);
           		memcpy(addr+26, &root, 4);
           		break;
        	}
    	}
}


/*Determines how many blocks to allocate*/
int allocateBlocks(int bufSize, int blockSize) {
	return bufSize % blockSize != 0 ? 
		(bufSize / blockSize) + 1 : 
		(bufSize / blockSize);
}


/* Purpose: 	Determines whether a file can be placed in a directory or if it's full
 * Returns:	void
 */
void addFile(int root_block, int block_size, void* address, struct stat buf, char* file_placement, int file_size, int needed_blocks, int starting_block, int fat_start_block, struct superblock_t* sb, char* tokens[]) {
	int flag = 0;
	int newSpot = 0;
	while (flag < 2) {
		if (flag == 1) {
			fullDirectory(sb, newSpot, address);
		}
	
		if (!putInfo(rootStartBlock(sb), htons(sb->block_size), address, buf, tokens[0], 
				buf.st_size, allocateBlocks(buf.st_size, htons(sb->block_size)), 
				starting_block, calculateStartBlock(sb), newSpot)) {
			return;	
		}
		flag++;
	}
	exit(EXIT_FAILURE);
}


/* Purpose: 	Find the location to copy the file too
 * Parameters:	struct superblock_t* sb - the super block information
 * 		void* addr - the address
 * 		FILE* fp - the file
 * 		struct stat buf - buffer containing file information collected using fstat()
 * Returns: 	int - the starting block
 */
int findPutLocation(struct superblock_t* sb, void* addr, FILE* fp, struct stat buf) {
	int blockCount = 0;
    	int last_FAT = 0, starting_block = 0;
    	char buffer[SECTORSIZE];
    	for (int i = calculateStartBlock(sb); i < StartPlusEnd(sb); i += 4) {
        	int count = -1;
        	memcpy(&count, addr + i, 4);
        	count = htonl(count);
        	if (count == 0) {
           		if (blockCount == 0) {
               			starting_block = newLocation(sb, i);
               			last_FAT = i;
               			blockCount++;
               			fread(buffer, htons(sb->block_size), 1, fp);
               			memcpy(addr+(starting_block*htons(sb->block_size)), 
						&buffer, htons(sb->block_size));
               			continue;
           		}
           		int mem_location = newLocation(sb, i);
           		fread(buffer, htons(sb->block_size), 1, fp);
           		memcpy(addr+(mem_location*htons(sb->block_size)), 
					&buffer, htons(sb->block_size));

           		int loc = htonl(mem_location);
           		memcpy(addr+last_FAT, &loc, 4);
           		last_FAT = i;
           		blockCount++;
           		if (blockCount == allocateBlocks(buf.st_size, htons(sb->block_size))){
				newBlock(addr, i, 4);
               			break;
           		}
        	}
    	}
 	return starting_block;	
}


/* Purpose: 	Copies a file from the current directory into the file system.
 * Parameters: 	int argc - argument count
 * 		char* argv[] - the users input
 * 		char* addr - the address
 * 		struct stat buf - the file fstat() information	
 * Returns:	void
 */
void diskput(int argc, char* argv[], char* addr, struct stat buf) {
    	checkArguments(argc, 4);
	char* tokens[128];
	stripDirectory(argv, 3, tokens);
    	FILE* fp = fopen(argv[2], "rb");
	if (fp == NULL) {
		return;
	}
    	struct superblock_t* sb = (struct superblock_t*) addr;
	addFile(rootStartBlock(sb), htons(sb->block_size), addr, buf, tokens[0], 
			buf.st_size, allocateBlocks(buf.st_size, htons(sb->block_size)), 
			findPutLocation(sb, addr, fp, buf), calculateStartBlock(sb), sb, tokens);
    	return;
}


void diskfix(int argc, char* argv[]) {}
