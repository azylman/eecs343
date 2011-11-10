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
typedef struct buffer_struct
{
	void* header;
	void* data;
} buffer;

typedef struct page_header_info_struct
{
	kpage_t* page_info;
	struct page_header_info_struct* next_page;
} page_header_info;

typedef struct
{
	buffer* next_buffer;
	int numAllocatedBuffers;
	page_header_info* first_page;
} free_list_info;

typedef struct
{
	kpage_t* page_info;
	free_list_info bytes32;
	free_list_info bytes64;
	free_list_info bytes128;
	free_list_info bytes256;
	free_list_info bytes512;
	free_list_info bytes1024;
	free_list_info bytes2048;
	free_list_info bytes4096;
	free_list_info bytes8192;
	int numAllocatedPages;
} free_list_pointers;

/************Function Prototypes******************************************/

kpage_t* get_entry_point();
void* get_next_buffer(free_list_info*);
void get_space_if_needed(free_list_info*, int size);
void add_buffer_to_free_list(buffer*, free_list_info*);
void add_page_to_free_list(page_header_info*, free_list_info*);

/************External Declaration*****************************************/

/**************Implementation***********************************************/

// Entry point into data structures.
static kpage_t* entry_point = 0;
static int debug = 0;

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
		free_list_info* free_list = &free_lists->bytes32;

		get_space_if_needed(free_list, 32);
		
		return get_next_buffer(free_list);
	}
	
	if (adjusted_size < 64) {
		free_list_info* free_list = &free_lists->bytes64;
		
		get_space_if_needed(free_list, 64);
		
		return get_next_buffer(free_list);
	}
	
	if (adjusted_size < 128) {
		free_list_info* free_list = &free_lists->bytes128;
		
		get_space_if_needed(free_list, 128);
		
		return get_next_buffer(free_list);
	}
	
	if (adjusted_size < 256) {
		free_list_info* free_list = &free_lists->bytes256;
		
		get_space_if_needed(free_list, 256);
		
		return get_next_buffer(free_list);
	}
	
	if (adjusted_size < 512) {
		free_list_info* free_list = &free_lists->bytes512;
		
		get_space_if_needed(free_list, 512);
		
		return get_next_buffer(free_list);
	}
	
	if (adjusted_size < 1024) {
		free_list_info* free_list = &free_lists->bytes1024;
		
		get_space_if_needed(free_list, 1024);
		
		return get_next_buffer(free_list);
	}
	
	if (adjusted_size < 2048) {
		free_list_info* free_list = &free_lists->bytes2048;
		
		get_space_if_needed(free_list, 2048);
		
		return get_next_buffer(free_list);
	}
	
	if (adjusted_size < 4096) {
		free_list_info* free_list = &free_lists->bytes4096;
		
		get_space_if_needed(free_list, 4096);
		
		return get_next_buffer(free_list);
	}
	
	if (adjusted_size < 8192) {
		free_list_info* free_list = &free_lists->bytes8192;
		
		get_space_if_needed(free_list, 8192);
		
		return get_next_buffer(free_list);
	}
	
	// If the size we're given is bigger than the size of a page.
	return NULL;
}

void
kma_free(void* ptr, kma_size_t size)
{
	printf("FREE %i\n", size);
	buffer* aBuffer = (buffer*)(ptr - sizeof(void*));
	if (debug) printf("Create buffer\n");
	free_list_info* free_list = aBuffer->header;
	aBuffer->header = free_list->next_buffer;
	add_buffer_to_free_list(aBuffer, free_list);

	free_list_pointers* free_lists = (free_list_pointers*)entry_point->ptr;
	
	if (debug) printf("Our number of allocated buffers went from %i ", free_list->numAllocatedBuffers);
	free_list->numAllocatedBuffers--;
	if (debug) printf("to %i\n", free_list->numAllocatedBuffers);
	if (free_list->numAllocatedBuffers == 0) {
	
		printf("First page: %p\n", free_list->first_page);
		page_header_info* cur_page = free_list->first_page;
		while (cur_page != 0) {
			kpage_t* page = cur_page->page_info;
			cur_page = cur_page->next_page;
			free_lists->numAllocatedPages--;
			free_page(page);
		}
		free_list->first_page = 0;
		free_list->next_buffer = 0;
	}
	
	if (free_lists->numAllocatedPages == 0) {
		free_page(entry_point);
		entry_point = 0;
	}
}

kpage_t* get_entry_point() {
	if (debug) printf("Getting entry point\n");
	kpage_t* entry_point = get_page();
	free_list_pointers* free_lists = (free_list_pointers*)entry_point->ptr;
	
	free_lists->page_info = entry_point;
	
	free_lists->bytes32.next_buffer = 0;
	free_lists->bytes32.numAllocatedBuffers = 0;
	free_lists->bytes32.first_page = 0;
	
	free_lists->bytes64.next_buffer = 0;
	free_lists->bytes64.numAllocatedBuffers = 0;
	free_lists->bytes64.first_page = 0;
	
	free_lists->bytes128.next_buffer = 0;
	free_lists->bytes128.numAllocatedBuffers = 0;
	free_lists->bytes128.first_page = 0;
	
	free_lists->bytes256.next_buffer = 0;
	free_lists->bytes256.numAllocatedBuffers = 0;
	free_lists->bytes256.first_page = 0;
	
	free_lists->bytes512.next_buffer = 0;
	free_lists->bytes512.numAllocatedBuffers = 0;
	free_lists->bytes512.first_page = 0;
	
	free_lists->bytes1024.next_buffer = 0;
	free_lists->bytes1024.numAllocatedBuffers = 0;
	free_lists->bytes1024.first_page = 0;
	
	free_lists->bytes2048.next_buffer = 0;
	free_lists->bytes2048.numAllocatedBuffers = 0;
	free_lists->bytes2048.first_page = 0;
	
	free_lists->bytes4096.next_buffer = 0;
	free_lists->bytes4096.numAllocatedBuffers = 0;
	free_lists->bytes4096.first_page = 0;
	
	free_lists->bytes8192.next_buffer = 0;
	free_lists->bytes8192.numAllocatedBuffers = 0;
	free_lists->bytes8192.first_page = 0;
	
	free_lists->numAllocatedPages = 0;
	
	return entry_point;
}

void* get_next_buffer(free_list_info* free_list) {

	free_list->numAllocatedBuffers++;

	if (debug) printf("Get buffer\n");
	buffer* aBuffer = free_list->next_buffer;
	if (debug) printf("Set free list pointer\n");
	if (debug) printf("Old free list starting point: %p, new: %p\n", free_list->next_buffer, aBuffer->header);
	free_list->next_buffer = aBuffer->header;
	if (debug) printf("Set buffer header\n");
	aBuffer->header = free_list;
	return &(aBuffer->data);
}

void get_space_if_needed(free_list_info* free_list, int size) {
	if (debug) printf("Checking %i-byte free list\n", size);
	
	if (free_list->next_buffer == 0) { // If there is no free buffer
		if (debug) printf("Get new page ");
		kpage_t* page = get_page();
		
		free_list_pointers* free_lists = (free_list_pointers*)entry_point->ptr;
		
		free_lists->numAllocatedPages++;
		
		page_header_info* page_header = (page_header_info*)page->ptr;
		page_header->page_info = page;
		page_header->next_page = 0;

		add_page_to_free_list(page_header, free_list);
		
		void* page_begin = page->ptr + sizeof(page_header_info);
		
		int numBuffers = (page->size - sizeof(page_header_info)) / size;
		numBuffers = numBuffers == 0 ? 1 : numBuffers;
		
		if (debug) printf("of size %i at %p with %i buffers\n", page->size, free_list->next_buffer, numBuffers);
		
		int i;
		buffer* aBuffer = 0;
		for (i = 0; i < numBuffers; i++) {
			aBuffer = (page_begin + i * size);
			if (debug) printf("Buffer %i starts at %p ", i + 1, aBuffer);
			aBuffer->header = 0;
			if (debug) printf("and points to %p\n", aBuffer->header);
			add_buffer_to_free_list(aBuffer, free_list);
		}
		
		if (debug) {
			printf("Printing new buffer list of size %i...\n", size);
			printf("%p ", free_list->next_buffer);
			for (i = 0; i < numBuffers; i++) {
				aBuffer = (free_list->next_buffer + i * size);
				printf("%p ", aBuffer->header);
			}
			printf("\n");
		}
	}
}

void add_buffer_to_free_list(buffer* aBuffer, free_list_info* free_list) {
	aBuffer->header = free_list->next_buffer;
	free_list->next_buffer = aBuffer;
}

void add_page_to_free_list(page_header_info* page_header, free_list_info* free_list) {
	page_header->next_page = free_list->first_page;
	free_list->first_page = page_header;
}

#endif // KMA_P2FL
