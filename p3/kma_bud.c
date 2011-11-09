/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the buddy algorithm
 *    Author: Stefan Birrer
 *    Version: $Revision: 1.2 $
 *    Last Modification: $Date: 2009/10/31 21:28:52 $
 *    File: $RCSfile: kma_bud.c,v $
 *    Copyright: 2004 Northwestern University
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    $Log: kma_bud.c,v $
 *    Revision 1.2  2009/10/31 21:28:52  jot836
 *    This is the current version of KMA project 3.
 *    It includes:
 *    - the most up-to-date handout (F'09)
 *    - updated skeleton including
 *        file-driven test harness,
 *        trace generator script,
 *        support for evaluating efficiency of algorithm (wasted memory),
 *        gnuplot support for plotting allocation and waste,
 *        set of traces for all students to use (including a makefile and README of the settings),
 *    - different version of the testsuite for use on the submission site, including:
 *        scoreboard Python scripts, which posts the top 5 scores on the course webpage
 *
 *    Revision 1.1  2005/10/24 16:07:09  sbirrer
 *    - skeleton
 *
 *    Revision 1.2  2004/11/05 15:45:56  sbirrer
 *    - added size as a parameter to kma_free
 *
 *    Revision 1.1  2004/11/03 23:04:03  sbirrer
 *    - initial version for the kernel memory allocator project
 *
 ***************************************************************************/
#ifdef KMA_BUD
#define __KMA_IMPL__

/************System include***********************************************/
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

/************Private include**********************************************/
#include "kpage.h"
#include "kma.h"

/************Defines and Typedefs*****************************************/
/*  #defines and typedefs should have their names in all caps.
 *  Global variables begin with g. Global constants with k. Local
 *  variables should be in all lower case. When initializing
 *  structures and arrays, line everything up in neat columns.
 */

/************Global Variables*********************************************/
typedef struct pageHeaderInfo_struct
{
	kpage_t* pageInfo;
	struct pageHeaderInfo_struct* nextPage;
} pageHeaderInfo;

typedef struct
{
	void* nextBuffer;
	int numAllocatedBuffers;
	pageHeaderInfo* firstPage;
} freeListInfo;

typedef struct
{
	kpage_t* pageInfo;
	freeListInfo bytes32;
	freeListInfo bytes64;
	freeListInfo bytes128;
	freeListInfo bytes256;
	freeListInfo bytes512;
	freeListInfo bytes1024;
	freeListInfo bytes2048;
	freeListInfo bytes4096;
	freeListInfo bytes8192;
	int numAllocatedPages;
} freeListPointers;

typedef struct bufferStruct
{
	void* header;
	struct bufferStruct* buddy;
	bool isAllocated;
	void* data;
} buffer;

/************Function Prototypes******************************************/

kpage_t* getEntryPoint();
void* getNextBuffer(freeListInfo*);
void getSpaceIfNeeded(freeListInfo*, int);
void addBufferToFreeList(buffer*, freeListInfo*);
void addPageToFreeList(pageHeaderInfo*, freeListInfo*);
void splitLargerList(int);
void splitLargeBufferToSmallBuffer(freeListInfo*, freeListInfo*, int);
void* removeFirstBuffer(freeListInfo*);
void* splitBuffer(buffer*, int);

/************External Declaration*****************************************/

/**************Implementation***********************************************/

// Entry point into data structures.
static kpage_t* entryPoint = 0;
static int debug = 1;

void*
kma_malloc(kma_size_t size)
{
	printf("\nREQUEST %i\n", size);
	if (entryPoint == 0) {
		entryPoint = getEntryPoint();
	}
	
	freeListPointers* freeLists = (freeListPointers*)entryPoint->ptr;
	
	int adjustedSize = size + sizeof(void*);
	if (adjustedSize < 32) {
		freeListInfo* freeList = &freeLists->bytes32;

		getSpaceIfNeeded(freeList, 32);
		
		return getNextBuffer(freeList);
	}
	
	if (adjustedSize < 64) {
		freeListInfo* freeList = &freeLists->bytes64;
		
		getSpaceIfNeeded(freeList, 64);
		
		return getNextBuffer(freeList);
	}
	
	if (adjustedSize < 128) {
		freeListInfo* freeList = &freeLists->bytes128;
		
		getSpaceIfNeeded(freeList, 128);
		
		return getNextBuffer(freeList);
	}
	
	if (adjustedSize < 256) {
		freeListInfo* freeList = &freeLists->bytes256;
		
		getSpaceIfNeeded(freeList, 256);
		
		return getNextBuffer(freeList);
	}
	
	if (adjustedSize < 512) {
		freeListInfo* freeList = &freeLists->bytes512;
		
		getSpaceIfNeeded(freeList, 512);
		
		return getNextBuffer(freeList);
	}
	
	if (adjustedSize < 1024) {
		freeListInfo* freeList = &freeLists->bytes1024;
		
		getSpaceIfNeeded(freeList, 1024);
		
		return getNextBuffer(freeList);
	}
	
	if (adjustedSize < 2048) {
		freeListInfo* freeList = &freeLists->bytes2048;
		
		getSpaceIfNeeded(freeList, 2048);
		
		return getNextBuffer(freeList);
	}
	
	if (adjustedSize < 4096) {
		freeListInfo* freeList = &freeLists->bytes4096;
		
		getSpaceIfNeeded(freeList, 4096);
		
		return getNextBuffer(freeList);
	}
	
	if (adjustedSize < 8192) {
		freeListInfo* freeList = &freeLists->bytes8192;
		
		getSpaceIfNeeded(freeList, 8192);
		
		return getNextBuffer(freeList);
	}
	
	// If the size we're given is bigger than the size of a page.
	return NULL;
}

void
kma_free(void* ptr, kma_size_t size)
{
	/*
	printf("FREE %i\n", size);
	buffer* aBuffer = (buffer*)(ptr - sizeof(void*));
	if (debug) printf("Create buffer\n");
	freeListInfo* freeList = aBuffer->header;
	aBuffer->header = freeList->nextBuffer;
	addBufferToFreeList(aBuffer, freeList);

	freeListPointers* freeLists = (freeListPointers*)entryPoint->ptr;
	
	if (debug) printf("Our number of allocated buffers went from %i ", freeList->numAllocatedBuffers);
	freeList->numAllocatedBuffers--;
	if (debug) printf("to %i\n", freeList->numAllocatedBuffers);
	if (freeList->numAllocatedBuffers == 0) {
	
		printf("First page: %p\n", freeList->firstPage);
		pageHeaderInfo* curPage = freeList->firstPage;
		while (curPage != 0) {
			kpage_t* page = curPage->pageInfo;
			curPage = curPage->nextPage;
			freeLists->numAllocatedPages--;
			free_page(page);
		}
		freeList->firstPage = 0;
		freeList->nextBuffer = 0;
	}
	
	if (freeLists->numAllocatedPages == 0) {
		free_page(entryPoint);
		entryPoint = 0;
	}
	*/
}

kpage_t* getEntryPoint() {
	if (debug) printf("Getting entry point\n");
	kpage_t* entryPoint = get_page();
	freeListPointers* freeLists = (freeListPointers*)entryPoint->ptr;
	
	freeLists->pageInfo = entryPoint;
	
	freeLists->bytes32.nextBuffer = 0;
	freeLists->bytes32.numAllocatedBuffers = 0;
	freeLists->bytes32.firstPage = 0;
	
	freeLists->bytes64.nextBuffer = 0;
	freeLists->bytes64.numAllocatedBuffers = 0;
	freeLists->bytes64.firstPage = 0;
	
	freeLists->bytes128.nextBuffer = 0;
	freeLists->bytes128.numAllocatedBuffers = 0;
	freeLists->bytes128.firstPage = 0;
	
	freeLists->bytes256.nextBuffer = 0;
	freeLists->bytes256.numAllocatedBuffers = 0;
	freeLists->bytes256.firstPage = 0;
	
	freeLists->bytes512.nextBuffer = 0;
	freeLists->bytes512.numAllocatedBuffers = 0;
	freeLists->bytes512.firstPage = 0;
	
	freeLists->bytes1024.nextBuffer = 0;
	freeLists->bytes1024.numAllocatedBuffers = 0;
	freeLists->bytes1024.firstPage = 0;
	
	freeLists->bytes2048.nextBuffer = 0;
	freeLists->bytes2048.numAllocatedBuffers = 0;
	freeLists->bytes2048.firstPage = 0;
	
	freeLists->bytes4096.nextBuffer = 0;
	freeLists->bytes4096.numAllocatedBuffers = 0;
	freeLists->bytes4096.firstPage = 0;
	
	freeLists->bytes8192.nextBuffer = 0;
	freeLists->bytes8192.numAllocatedBuffers = 0;
	freeLists->bytes8192.firstPage = 0;
	
	freeLists->numAllocatedPages = 0;
	
	return entryPoint;
}

void* getNextBuffer(freeListInfo* freeList) {

	freeList->numAllocatedBuffers++;

	buffer* aBuffer = removeFirstBuffer(freeList);
	if (debug) printf("Returning %p as the result of malloc\n", aBuffer);
	return &(aBuffer->data);
}

void getSpaceIfNeeded(freeListInfo* freeList, int size) {
	if (debug) printf("Checking %i-byte free list\n", size);
	
	if (freeList->nextBuffer == 0) { // If there is no free buffer
		if (debug) printf("Need to split to get a buffer of size %i\n", size);
		splitLargerList(size);
	}
}

void addBufferToFreeList(buffer* aBuffer, freeListInfo* freeList) {
	if (debug) printf("Adding a buffer %p to the free list %p ", aBuffer, freeList);
	aBuffer->header = freeList->nextBuffer;
	if (debug) printf("and setting its header to %p\n", aBuffer->header);
	freeList->nextBuffer = aBuffer;
}

void addPageToFreeList(pageHeaderInfo* pageHeader, freeListInfo* freeList) {
	pageHeader->nextPage = freeList->firstPage;
	freeList->firstPage = pageHeader;
}

void splitLargerList(int size) {
	freeListPointers* freeLists = (freeListPointers*)entryPoint->ptr;
	
	if (size == 32) {
		freeListInfo* freeList = &freeLists->bytes64;
		freeListInfo* smallFreeList = &freeLists->bytes32;
		if (freeList->nextBuffer == 0) {
			//if (debug) printf("Splitting %i into %i\n", size*2, size);
			splitLargerList(64);
		}
		splitLargeBufferToSmallBuffer(freeList, smallFreeList, 32);
	}
	
	if (size == 64) {
		freeListInfo* freeList = &freeLists->bytes128;
		freeListInfo* smallFreeList = &freeLists->bytes64;
		if (freeList->nextBuffer == 0) {
			//if (debug) printf("Splitting %i into %i\n", size*2, size);
			splitLargerList(128);
		}
		splitLargeBufferToSmallBuffer(freeList, smallFreeList, 64);
	}
	
	if (size == 128) {
		freeListInfo* freeList = &freeLists->bytes256;
		freeListInfo* smallFreeList = &freeLists->bytes128;
		if (freeList->nextBuffer == 0) {
			//if (debug) printf("Splitting %i into %i\n", size*2, size);
			splitLargerList(256);
		}
		splitLargeBufferToSmallBuffer(freeList, smallFreeList, 128);
	}
	
	if (size == 256) {
		freeListInfo* freeList = &freeLists->bytes512;
		freeListInfo* smallFreeList = &freeLists->bytes256;
		if (freeList->nextBuffer == 0) {
			//if (debug) printf("Splitting %i into %i\n", size*2, size);
			splitLargerList(512);
		}
		splitLargeBufferToSmallBuffer(freeList, smallFreeList, 256);
	}
	
	if (size == 512) {
		freeListInfo* freeList = &freeLists->bytes1024;
		freeListInfo* smallFreeList = &freeLists->bytes512;
		if (freeList->nextBuffer == 0) {
			//if (debug) printf("Splitting %i into %i\n", size*2, size);
			splitLargerList(1024);
		}
		splitLargeBufferToSmallBuffer(freeList, smallFreeList, 512);
	}
	
	if (size == 1024) {
		freeListInfo* freeList = &freeLists->bytes2048;
		freeListInfo* smallFreeList = &freeLists->bytes1024;
		if (freeList->nextBuffer == 0) {
			//if (debug) printf("Splitting %i into %i\n", size*2, size);
			splitLargerList(2048);
		}
		splitLargeBufferToSmallBuffer(freeList, smallFreeList, 1024);
	}
	
	if (size == 2048) {
		freeListInfo* freeList = &freeLists->bytes4096;
		freeListInfo* smallFreeList = &freeLists->bytes2048;
		if (freeList->nextBuffer == 0) {
			//if (debug) printf("Splitting %i into %i\n", size*2, size);
			splitLargerList(4096);
		}
		splitLargeBufferToSmallBuffer(freeList, smallFreeList, 2048);
	}
	
	if (size == 4096) {
		freeListInfo* freeList = &freeLists->bytes8192;
		freeListInfo* smallFreeList = &freeLists->bytes4096;
		if (freeList->nextBuffer == 0) {
			//if (debug) printf("Splitting %i into %i\n", size*2, size);
			splitLargerList(8192);
		}
		splitLargeBufferToSmallBuffer(freeList, smallFreeList, 4096);
	}
	
	if (size == 8192) {
		freeListInfo* freeList = &freeLists->bytes8192;
		if (freeList->nextBuffer == 0) {
			printf("Getting a new page\n");
			kpage_t* page = get_page();
			
			freeListPointers* freeLists = (freeListPointers*)entryPoint->ptr;
			
			freeLists->numAllocatedPages++;
			
			pageHeaderInfo* pageHeader = (pageHeaderInfo*)page->ptr;
			pageHeader->pageInfo = page;
			pageHeader->nextPage = 0;

			addPageToFreeList(pageHeader, freeList);
			
			void* pageBegin = page->ptr + sizeof(pageHeaderInfo);
			
			int numBuffers = (page->size - sizeof(pageHeaderInfo)) / size;
			numBuffers = numBuffers == 0 ? 1 : numBuffers;
			
			int i;
			buffer* aBuffer = 0;
			for (i = 0; i < numBuffers; i++) {
				aBuffer = (pageBegin + i * size);
				aBuffer->header = 0;
				addBufferToFreeList(aBuffer, freeList);
			}
		}
	}	
}

void splitLargeBufferToSmallBuffer(freeListInfo* largeFreeList, freeListInfo* smallFreeList, int size) {
	if (debug) printf("Splitting a %i buffer into two %i buffers\n", size*2, size);
	buffer* largeBuffer = removeFirstBuffer(largeFreeList);
	if (debug) printf("Begin splitting to add to %p...\n", smallFreeList);
	buffer* smallBuffer = splitBuffer(largeBuffer, size);
	addBufferToFreeList(smallBuffer, smallFreeList);
	addBufferToFreeList(smallBuffer->buddy, smallFreeList);
}


void* removeFirstBuffer(freeListInfo* freeList) {
	printf("test1\n");
	if (debug) printf("From free list %p, ", freeList);
	printf("test1\n");
	buffer* aBuffer = freeList->nextBuffer;
	if (debug) printf("getting buffer %p and ", aBuffer);
	freeList->nextBuffer = aBuffer->header;
	if (debug) printf("updating the list to point to %p\n", freeList->nextBuffer);
	return aBuffer;
}

void* splitBuffer(buffer* largeBuffer, int size) {
	largeBuffer->buddy = largeBuffer + size/sizeof(buffer);
	largeBuffer->buddy->buddy = largeBuffer;
	
	if (debug) printf("Splitting %p into %p and %p which points to %p\n", largeBuffer, largeBuffer, largeBuffer->buddy, largeBuffer->buddy->buddy);
	
	largeBuffer->isAllocated = 0;
	largeBuffer->buddy->isAllocated = 0;
	return largeBuffer;
}

#endif // KMA_BUD
