/***************************************************************************
 *  Title: Kernel Memory Allocator
 * -------------------------------------------------------------------------
 *    Purpose: Kernel memory allocator based on the power-of-two free list
 *             algorithm
 *    Author: Stefan Birrer
 *    Version: $Revision: 1.2 $
 *    Last Modification: $Date: 2009/10/31 21:28:52 $
 *    File: $RCSfile: kma_p2fl.c,v $
 *    Copyright: 2004 Northwestern University
 ***************************************************************************/
/***************************************************************************
 *  ChangeLog:
 * -------------------------------------------------------------------------
 *    $Log: kma_p2fl.c,v $
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
#ifdef KMA_P2FL
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

/************Function Prototypes******************************************/

kpage_t* get_entry_point();
void* get_next_buffer(void**);
void get_space_if_needed(void**, int size);

/************External Declaration*****************************************/

/**************Implementation***********************************************/


typedef struct
{
	void* bytes32;
	void* bytes64;
	void* bytes128;
	void* bytes256;
	void* bytes512;
	void* bytes1024;
	void* bytes2048;
	void* bytes4096;
	void* bytes8192;
} free_list_pointers;

typedef struct
{
	void* header;
	void* data;
} buffer;

// Entry point into data structures.
static kpage_t* entry_point = 0;

void*
kma_malloc(kma_size_t size)
{	
	printf("REQUEST %i\n", size);
	if (entry_point == 0) {
		entry_point = get_entry_point();
	}
	
	free_list_pointers* free_lists = (free_list_pointers*)entry_point->ptr;
	
	int adjusted_size = size + sizeof(void*);
	if (adjusted_size < 32) {
		void** free_list = &(free_lists->bytes32);
		
		get_space_if_needed(free_list, 32);
		
		return get_next_buffer(free_list);
	}
	
	if (adjusted_size < 64) {
		void** free_list = &(free_lists->bytes64);
		
		get_space_if_needed(free_list, 64);
		
		return get_next_buffer(free_list);
	}
	
	if (adjusted_size < 128) {
		void** free_list = &(free_lists->bytes128);
		
		get_space_if_needed(free_list, 128);
		
		return get_next_buffer(free_list);
	}
	
	if (adjusted_size < 256) {
		void** free_list = &(free_lists->bytes256);
		
		get_space_if_needed(free_list, 256);
		
		return get_next_buffer(free_list);
	}
	
	if (adjusted_size < 512) {
		void** free_list = &(free_lists->bytes512);
		
		get_space_if_needed(free_list, 512);
		
		return get_next_buffer(free_list);
	}
	
	if (adjusted_size < 1024) {
		void** free_list = &(free_lists->bytes1024);
		
		get_space_if_needed(free_list, 1024);
		
		return get_next_buffer(free_list);
	}
	
	if (adjusted_size < 2048) {
		void** free_list = &(free_lists->bytes2048);
		
		get_space_if_needed(free_list, 2048);
		
		return get_next_buffer(free_list);
	}
	
	if (adjusted_size < 4096) {
		void** free_list = &(free_lists->bytes4096);
		
		get_space_if_needed(free_list, 4096);
		
		return get_next_buffer(free_list);
	}
	
	if (adjusted_size < 8192) {
		void** free_list = &(free_lists->bytes8192);
		
		get_space_if_needed(free_list, 8192);
		
		return get_next_buffer(free_list);
	} else { // If the size we're given is bigger than the size of a page.
		return NULL;
	}
}

void
kma_free(void* ptr, kma_size_t size)
{
	printf("FREE %i\n", size);
	buffer* aBuffer = (buffer*)(ptr - sizeof(void*));
	printf("Create buffer\n");
	void** free_list = aBuffer->header;
	printf("Get free list\n");
	aBuffer->header = *free_list;
	printf("Set buffer header\n");
	*free_list = aBuffer;
	printf("Set first free buffer\n");
}

kpage_t* get_entry_point() {
	kpage_t* entry_point = get_page();
	free_list_pointers* free_lists = (free_list_pointers*)entry_point->ptr;
	free_lists->bytes32 = 0;
	free_lists->bytes64 = 0;
	free_lists->bytes128 = 0;
	free_lists->bytes256 = 0;
	free_lists->bytes512 = 0;
	free_lists->bytes1024 = 0;
	free_lists->bytes2048 = 0;
	free_lists->bytes4096 = 0;
	free_lists->bytes8192 = 0;
	return entry_point;
}

void* get_next_buffer(void** free_list) {
	printf("Get buffer\n");
	buffer* aBuffer = (buffer*)(*free_list);
	printf("Set free list pointer\n");
	*free_list = aBuffer->header;
	printf("Set buffer header\n");
	aBuffer->header = free_list;
	return &(aBuffer->data);	
}

void get_space_if_needed(void** free_list, int size) {
	printf("Checking %i-byte free list\n", size);
	if (*free_list == 0) { // If there is no free buffer
		printf("Get new page\n");
		kpage_t* page = get_page();
		*free_list = page->ptr;
		int numBuffers = PAGESIZE / size;
		int i;
		for (i = 0; i < numBuffers; i++) {
			buffer* aBuffer = (buffer*)(*free_list + i * size);
			
			if (i == numBuffers - 1) {
				aBuffer->header = 0;
			} else {
				aBuffer->header = aBuffer + size;
			}
		}
	}
}

#endif // KMA_P2FL
