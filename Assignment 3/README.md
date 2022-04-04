# CSc 360: Programming Assignment 3
In `parts.c` we implement utilities that perform operations on a file system similar to Microsoft’s FAT file system with additional improvements. Note that the requirments of this assingments have been changed to no longer require sub-directory support. Nonetheless, this program supports sub-directories for diskinfo and disklist.


## Design
For each part in `parts.c`, `mmap()` is used to create a new mapping in the virtual address space of the calling process. This is done to deter page faults, and to ensure that reading and writing to a memory-mapped file does not incur any system call or context switch overhead [1].

### Part I: diskinfo.c
Displays information about the file system. The program reads from the file syper block and uses the information to read the FAT.

### Part II: disklist.c
Displays the contents of the root directory with the following content:
- Indication of file or directory:
  - `F` for regular files
  - `D` for directories 

- File size
- File name
- File creation date in `<year/month/day> <hour:min:sec>` format

### Part III: diskget.c
Copies a file from the file system to the current local directory in Linux. Only works for the root directory. The following design is implemented:
1) Convert the given filename to upper case then search the directory entries in the foot folder for the filename.
2) If filname found -> get the file size and first logical cluster.
3) Use FAT entries to copy.
4) If the last cluster of file and file size value is reached, stop the copy.
5) Print "File not found." if the file is not in the root directory, then exit.

### Part IV: diskput.c
Copies a file from the current local directory in Linux into the file system. Only works for the root directory. the following design is implemented:
1) Check if the file is within the current local Linux directory.
2) Check if the specified directory exits within the FAT image.
3) check the available space
4) Find unused sectors in the disk.
5) Copy the file to the unused sectors.
6) Update the file size of directory entry and first logical cluster number.
7) Update FAT entries.

### Part V: diskfix.c (Not completed)
Iterate through the disk image according to the file system specification and find inconsistent information and fix the issues.

## Run the program
Compile by running `make`

Run any of the commands one-by-one: </br>
`./diskinfo test.img` </br>
`./disklist test.img /` </br>
`./diskget test.img /foo.txt foo2.txt` </br>
`./diskput test.img foo.txt /foo2.txt` </br>

### References: </br>
https://stackoverflow.com/questions/258091/when-should-i-use-mmap-for-file-access </br>
Tutorial 8,9,10 slides
