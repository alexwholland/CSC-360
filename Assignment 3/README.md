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
Copies a file from the file system to the current local directory in Linux. Only works for the root directory.

### Part IV: diskput.c
Copies a file from the current local directory in Linux into the file system. Only works for the root directory.

### Part V: diskfix.c (Not completed)

## Run the program
Compile by running `make`

Run any of the commands one-by-one: </br>
`./diskinfo test.img` </br>
`./disklist test.img /` </br>
`./diskget test.img /foo.txt foo2.txt` </br>
`./diskput test.img foo.txt /foo2.txt` </br>

### References: </br>
https://stackoverflow.com/questions/258091/when-should-i-use-mmap-for-file-access
