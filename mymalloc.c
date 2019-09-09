#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mymalloc.h"

int getSize(int headerIndex)
{//reverses the 4byte->2byte conversion of the size stored in the header at headerIndex
	//printf("in get size\n");
	unsigned char check1 = *(unsigned char*)&myBlock[headerIndex+1];
	unsigned char check2 = *(unsigned char*)&myBlock[headerIndex];
	int added = (check2<<8)+check1;
	//printf("size is: %d\n", added);
	return added;
}
int convertInt(int size, int index)
{//convert the 4byte size integer to 2bytes (`•ω•´๑)
	//printf("in convert int\n");
	myBlock[index] = 0; myBlock[index+1]=0; //init data to avoid error
	//printf("Origional int size: %d\n", size);
	unsigned char b1 = (unsigned)(size&0xF); //this is the lowest order
	unsigned char b2 = (unsigned)((size>>4)&0xF); //second lowest
	unsigned char b3 = (unsigned)((size>>8)&0xF); //third lowest
	unsigned char b4 = (unsigned)(((size>>16)&0xF)<<4); //highest
	//find the bits of size with values--this is for precaution
	if( ((size&0xFF)!=0)||(((size>>8)&0xFF)!=0) )
	{ //access the first two bytes of size and save them in the header
		//for clarity: start with the most significant bits (16-11, 12-9), cast them as unsigned to avoid two's complement, and store them in the address after the allocFlag
		unsigned char result1 = (unsigned)(b2<<4) + (unsigned)b1;
		unsigned char result2 = (unsigned)(b4<<4) + (unsigned)b3;
		myBlock[index] = *(unsigned char*)&result2;
		//do the same with bits 8-5 and 4-1
		myBlock[index+1] = *(unsigned char*)&result1;
	}
	//this should never happen, bitwise shifting should disregard endianness
	else //( (((size>>16)& 0xFF)!=0)||((size>>24)& 0xFF)!=0)
	{
		myBlock[index] = (unsigned char)((size>>16)& 0xFF);
		myBlock[index+1] = (unsigned char)((size>>24)& 0xFF);
	}
	//printf("1st byte: %u 2nd byte: %u\n", *(unsigned char*)&myBlock[index], *(unsigned char*)&myBlock[index+1]);
	if(getSize(index) == size) { return 1; }
	else { return -1; }
}
int freeblockRepurpose(int payload, int size, int index)
{//size is the requested num bytes for alloc, payload is the available bytes in the free block (header has already been accounted for), index is the location of the block
	//printf("in freeblock repurpose\n");
	//change flag to ALLOC
	myBlock[index] = (char)ALLOC;
	//printf("set flag to %d\n", (int)myBlock[index]);
	int check = 0;
	if((payload-HEADER_SIZE) == size) { return 1; } //size is the same so we just change the flag and we are done
//***********************************************COALESCE HERE******************************************************************
	else 
	{//payload > size, will need to split the block into allocated and free
		//printf("size of the free block is larger than the requested size\n");
		check = convertInt(size, (index+1));
		if(check==-1)
		{
			printf("error in convertInt()\n");
			return check;
		}
		//printf("convert int returns %d\n", check);
		//check if the next block is F, if yes send to a coalesce function
		//else: compute the size of the remaining free block, if we have enough bytes make a free block otherwise add the bytes as FREE padding
		int remainder = ((payload-HEADER_SIZE)-size);
		//printf("remainder is: %d", remainder);
		if(remainder < 4)
		{//we don't have enough bytes to make another free block, add to the padding of the allocated block
			//printf("remainder is too small for a new block\n");
			//update the header (requested size + padding)
			size = size + remainder;
			check = convertInt(remainder, index+1);
			//printf("new size is %d and convert int returned %d\n", size, check);
			if(check==-1)
			{
				printf("error in convertInt()\n");
				return check;
			}
			return check;
		}
		else //make a new free block with the remaining bytes
		{
			//printf("we can make a new free block. we were at index %d\n", index);
			index = (((index+HEADER_SIZE)+size)+1);
			myBlock[index] = (char)FREE;
			//printf("not at index %d set the flag to %d\n", index, (int)myBlock[index]);
			check = convertInt((remainder-HEADER_SIZE), index+1);
			//printf("convert int returns: %d\n", check);
			if(check==-1)
			{
				printf("error in convertInt()\n");
				return check;
			}
			return check;
		}
	}	
}
int freeblockInit(int size, int index, char flag)
{//create a header for free blocks
	//printf("in freeblock init\n");
	//this should not happen
	if(flag == ALLOC) { return -1; }
	int payload = size-(HEADER_SIZE); //bytes remaining after metadata allocation. we already checked if size is < 4 in mymalloc()
	//printf("free block payload is: %d\n", payload);
	//initialize the header
	myBlock[index] = flag;
	//printf("set the flag: %d\n", (int)myBlock[index]);
	int check = convertInt(payload, index+1);
	//printf("check returns: %d\n", check);
	if(check==-1)
	{
		printf("error in convertInt()\n");
		return index;
	}
	return ((index+payload)+2);
}
int headerInit(int size, int index)
{//create the header for allocated blocks
	//printf("in header init\n");
	//set the flag
	myBlock[index] = (char)ALLOC;
	//printf("flag is set to %d at index %d\n", (int)myBlock[index], index);
	//check the size to see if there will be padding
	int position = ((index+size)+HEADER_SIZE);
	//printf("end of the block will be at %d\n", position);
	int check = 0;
	//not enough space to make this block
	if(position > 4095) { return index; }
	else if(position >= 4092)
	{//add the extra bytes as padding (max 3). this is good bc we can avoid boundary error during coalescing
		//printf("add padding to this block\n");
		int dif = (4095 - position);
		size = size + dif;
	}

	//use convertInt() to store the size data
	check = convertInt(size, index+1);
	//printf("convert int returns %d\n", check);
	if(check==-1)
	{
		//printf("error in convertInt()\n");
		return (index+1);
	}
	//return the address of the end of the block
	return position;
}
int mallocInit(int size)
{//used to initialize an empty static array
	//printf("in Malloc Init\n");
	int check = 0;
	//create the 4byte prologue w/magicc number
	long fourthM = (MAGICAL_NUMBER>>24&0xFF);
	long thirdM = (MAGICAL_NUMBER>>16&0xFF);
	long secondM = (MAGICAL_NUMBER>>8&0xFF);
	long firstM = (MAGICAL_NUMBER&0xFF);
	myBlock[0] = firstM;
	myBlock[1] = secondM;
	myBlock[2] = thirdM;
	myBlock[3] = fourthM;
	//create the header and return the index of the end of the block
	check = headerInit(size, 4);
	return check;
}
int magicCheck()
{//check for stack initialization with (∩^o^)⊃━☆ﾟ.*･｡ﾟmagic number•☆ﾟ.*･｡
	//printf("in Magic Check\n");
	long firstB = (long)myBlock[0];
	long secondB = (long)myBlock[1];
	long thirdB = (long)myBlock[2];
	long fourthB = (long)myBlock[3];
	long fourthM = (MAGICAL_NUMBER>>24&0xFF);
	long thirdM = (MAGICAL_NUMBER>>16&0xFF);
	long secondM = (MAGICAL_NUMBER>>8&0xFF);
	long firstM = (MAGICAL_NUMBER&0xFF);

	if( (firstB==firstM)&&(secondB==secondM)&&(thirdB==thirdM)&&(fourthB==fourthM) ){ return ALLOC; }
	return FREE;
}
int checkAdjacent(int index, int size)
{//go to the next block and check the flag. if FREE, return the size of the adjacent free block, otherwise return -1
	if(myBlock[index+size] == (char)FREE)
	{
		int nextBlock = getSize((index+size)+1);
		if((nextBlock < 0)||(nextBlock > 4088)) { return -1; }
		return nextBlock;
	}
	return -1;
}
/**void deferredCoalesce()
{//deferred coalescing policy, therefore we will traverse the entire array coalescing as we go
CASES FOR COALESCING THE CURRENT BLOCK
	I. both adjacent blocks are ALLOC
	II. both adjacent blocks are FREE
	III. prev is ALLOC and next is FREE
	IV. prev is FREE and next is ALLOC

	int curIndex = 4, ogIndex = 4, count = 0, size = 0, check = 0;
	
}**/
void* mymalloc(int size, int __LINE__, char* __FILE__)
{//mymalloc returns the address of the beginning of the payload, or NULL 
	//printf("Start!\n");
	//min/max size values
	if((size>4088)||(size<=0)) { return NULL; }
	if(magicCheck()==FREE)
	{//initialize the static array with the magic number & first blocks
		//printf("Magic Check is false\n");
		//initialize the first allocated block, store the index of the end of the block
		int endIndex = mallocInit(size);
		//printf("malloc init returns %d\n", endIndex);
		//if 4095 is reached we are at the end of the list and there is no adjacent free block to initalize
		if((endIndex)==4095){ return &myBlock[endIndex-size]; }
		else if(endIndex==4)
		{ 
			//printf("error somewhere :O\n");
			return NULL;
		}
		//size of the *first* free block's payload = (total indicies - position in array) - (size of header+footer)
		int freePayload = (4095-endIndex);
		//printf("freepayload: %d\n", freePayload);
		if(freePayload < 4) { return &myBlock[endIndex-size]; } //padding was set in headerInit(), no freeblock to make
		//initialize the free block
		int check = freeblockInit(freePayload, endIndex+1, (char)FREE);
		//printf("freeblockInit returns: %d\n", check);
		if(check==endIndex+1)
		{
			printf("error in freeblockInit()\n");
			return NULL;
		}
		//printf("inital alloc and free blocks are done!\n");
		return &myBlock[7]; //return the address of the beginning of the payload
	}
//TODO:::add a check for the "array is full" special key ????
	//already initialized the block (unless our key value wasn't random enough)
	else
	{//prologue ends at index 4, start to search for a free block with enough space
		//printf("Magic check is false\n");
		//using first fit as the block placement policy
		int index = 4, curPayload = 0, freePayload = 0, curblockSize = 0, loop = 0, check = 0;
		do { //traverse the array to find a free block that we can allocate memory from
			//printf("loop number %d\n", loop);
			curPayload = getSize(index+1); //get the size of the current block
			//printf("current block size: %d\n", curPayload);
			curblockSize = curPayload + HEADER_SIZE;
			//printf("total block size: %d\n", curblockSize);
			if(myBlock[index] == (char)FREE)
			{//if the current block is free and the size is big enough
				//printf("found a free block!\n");
				if( (curPayload+HEADER_SIZE) >= size) //when converting from free to alloc, there is an extra 3 bytes
				{//need to repurpose this block
					//printf("this block is %d which is large enough to hold %d\n", (curPayload+HEADER_SIZE), size);
					check = freeblockRepurpose(curblockSize, size, index);
					//printf("freeblock repurpose returns: %d\n", check);
					//printf("index: %d\n", index);
					//printf("return index: %d\n", (index+HEADER_SIZE));
					return &myBlock[index+HEADER_SIZE];
				}
			}
			//printf("did not find a free block\n");
			//go to the header of the next block 
			index = ((index + curblockSize)+ 1);
			//printf("go to index %d\n", index);
			loop = loop+1;
		} while(index < (4095 - HEADER_SIZE));
//if we get here, there was no free block found
//TODO::::coalesce to see if we can create more space
	}
	return NULL;
}

int main(int argc, char **argv)
{ //testing
	*myBlock = 507000;
	char *ptr = malloc(500);
	ptr = "this is";
	char* ptr2 = malloc(12);
	ptr2 = "why not";
	char* ptr3 = malloc(7);
	ptr3 = "just use";
	char* ptr1 = malloc(1000);
	ptr1 = "amAAzing";
	char* ptr4 = malloc(2000);
	ptr4 = "a debugger?";
	char* ptr5 = malloc(500;
	ptr5 = "that would be";
	char* ptr6 = malloc(10);
	ptr6 = "too easy";
	char* ptr7 = malloc(50);
	ptr7 = "!";

	if( (ptr == NULL)||(ptr1 == NULL)||(ptr2 == NULL)||(ptr3==NULL)||(ptr4==NULL)||(ptr5==NULL)||(ptr6==NULL)||(ptr7==NULL) ) { printf("okok\n"); }
	else 
	{
		printf("%c\n%c\n%c\n%c\n%c\n%c\n%c\n%c\n", (char)ptr, (char)ptr1, (char)ptr2, (char)ptr3, (char)ptr4, (char)ptr5, (char)ptr6, (char)ptr7);
	}
	
	return 0;
}
