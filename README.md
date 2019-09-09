# mymalloc
An implementation of the C library function malloc() which aims to minimize the amount of metadata used.
### Design
Due to the size of the static array, 4096 bytes, the maximum size of an accepted mymalloc() request can not use more than 2 bytes of memory. Therefore by bitshifting the requested size, we can elliminate 2 unused bytes of memory per allocated block. The header of each block will contain a 1 byte free/allocated flag followed by the size of the allocated block which has been bitshifted into 2 bytes. Therefore the total size of metadata is 3 bytes for every block, plus a 4 byte "magic number" at the beginning of the static array which is used to determine if the array has been initialized. 
