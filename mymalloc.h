#ifndef MYMALLOC_H
#define MYMALLOC_H

//commonly used values
#define MAGICAL_NUMBER 2109999L //random key, long (4 bytes)
#define FREE 0
#define ALLOC 1
#define HEADER_SIZE 3

//define a static array of size 4096 to allocate from
static char myBlock[4096];

//macro that replaces all malloc(x) with mymalloc(x) and all free(x) with myfree(x)
#define malloc(x) mymalloc(x, __FILE__, __LINE__)
#define free(x) myfree(x, __FILE__, __LINE__)

//function signature for mymalloc(x)
void* mymalloc(int size, char *var1, size_t l1);
void myfree(void* index, char *var2, size_t l2);

#endif
