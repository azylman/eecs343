/* -*-C-*-
 *******************************************************************************
 *
 * File:         sfs.c
 * RCS:          $Id: sfs.c,v 1.2 2009/11/10 21:17:25 npb853 Exp $
 * Description:  Simple File System
 * Author:       Fabian E. Bustamante
 *               Northwestern Systems Research Group
 *               Department of Computer Science
 *               Northwestern University
 * Created:      Tue Nov 05, 2002 at 07:40:42
 * Modified:     Fri Nov 19, 2004 at 15:45:57 fabianb@cs.northwestern.edu
 * Language:     C
 * Package:      N/A
 * Status:       Experimental (Do Not Distribute)
 *
 * (C) Copyright 2003, Northwestern University, all rights reserved.
 *
 *******************************************************************************
 */

#include <math.h>
#include <stdbool.h>
 
#include "sfs.h"
#include "sdisk.h"

Sector* getSector(int);

void allocateSector(int);
void deallocateSector(int);

void setBit(int*, int);
void clearBit(int*, int);
void toggleBit(int*, int);
int getBit(int*, int);
void initSector(int);

Sector* getSector(int sector) {
	Sector* retrievedSector = malloc(sizeof(Sector));
	SD_read(sector, retrievedSector);
	return retrievedSector;
}

void allocateSector(int sector) {
	int bitmapSectorNumber = floor( sector / SD_SECTORSIZE );
	int sectorOffset = sector % SD_SECTORSIZE;
	
	Sector* bitmapSector = getSector(bitmapSectorNumber);
	
	setBit((int*)bitmapSector, sectorOffset);
	SD_write(bitmapSectorNumber, bitmapSector);
	
	free(bitmapSector);
}

void deallocateSector(int sector) {
	int bitmapSectorNumber = floor( sector / SD_SECTORSIZE );
	int sectorOffset = sector % SD_SECTORSIZE;
	
	Sector* bitmapSector = getSector(bitmapSectorNumber);
	
	clearBit((int*)bitmapSector, sectorOffset);
	SD_write(bitmapSectorNumber, bitmapSector);
	
	free(bitmapSector);
}

void setBit(int* sequence, int bitNum) {
	*sequence |= 1 << bitNum;
}

void clearBit(int* sequence, int bitNum) {
	*sequence &= ~(1 << bitNum);
}

int getBit(int* sequence, int bitNum) {
	return *sequence & (1 << bitNum);
}

void toggleBit(int* sequence, int bitNum) {
	*sequence ^= 1 << bitNum;
}

void initSector(int sector) {
	Sector* bitmapSector = malloc(sizeof(Sector));
	SD_read(sector, bitmapSector);
	*(int*)bitmapSector = 0;
	SD_write(sector, bitmapSector);
	free(bitmapSector);
}

typedef struct inodeHeader_s {
	int nextInodeSector;
	int usedInodesBitmap; // we need a size for this
} inodeHeader;

typedef struct inode_s {
	bool isFile;
	struct inode_s* parent;
	struct inode_s* cont;
} inode;

typedef struct inodeFile_s {
	bool isFile;
	inode* parent;
	inode* cont;
	int sectors[6]; // find out what size this should be
} inodeFile;

typedef struct inodeDir_s {
	bool isFile;
	inode* parent;
	inode* cont;
	inode* children[6]; // find out what size this should be
} inodeDir;

static int numSectorsForFreeBitmap = -1;

static int inodeSize = sizeof(inodeFile) > sizeof(inodeDir) ? sizeof(inodeFile) : sizeof(inodeDir);

// some static file containing info on all open files

/*
 * sfs_mkfs: use to build your filesystem
 *
 * Parameters: -
 *
 * Returns: 0 on success, or -1 if an error occurred
 *
 */
int sfs_mkfs() {
	int numBytes = ceil( (double)SD_NUMSECTORS / (double)8 );
	numSectorsForFreeBitmap = ceil( (double)numBytes / (double)SD_SECTORSIZE );
	
	int i;
	for(i = 0; i < numSectorsForFreeBitmap; ++i) {
		initSector(i);
		allocateSector(i);
	}
	
	// set up root inode
	
    return 0;
} /* !sfs_mkfs */

/*
 * sfs_mkdir: attempts to create the name directory
 *
 * Parameters: directory name
 *
 * Returns: 0 on success, or -1 if an error occurred
 *
 */
int sfs_mkdir(char *name) {
    // parse the name to get the parents and the name of the new directory
	// iterate over the inodes until you find the subdirectory you're supposed to create it at
	// create it
    return -1;
} /* !sfs_mkdir */

/*
 * sfs_fcd: attempts to change current directory to named directory
 *
 * Parameters: new directory name
 *
 * Returns: 0 on success, or -1 if an error occurred
 *
 */
int sfs_fcd(char* name) {
	// how do we keep track of current directory? a static var?
	// parse the path and iterate through all the inodes until we find the new one
    return -1;
} /* !sfs_fcd */

/*
 * sfs_ls: output the information of all existing files in 
 *   current directory
 *
 * Parameters: -
 *
 * Returns: 0 on success, or -1 if an error occurred
 *
 */
int sfs_ls(FILE* f) {
    // read all the info in the current directory
	// write it to f
    return -1;
} /* !sfs_ls */

/*
 * sfs_fopen: convert a pathname into a file descriptor. When the call
 *   is successful, the file descriptor returned will be the lowest file
 *   descriptor not currently open for the process. If the file does not
 *   exist it will be created.
 *
 * Parameters: file name
 *
 * Returns:  return the new file descriptor, or -1 if an error occurred
 *
 */
int sfs_fopen(char* name) {
    // find the inode based on the name, write file info to our file descriptors array, and return the index into the array
    return -1;
} /* !sfs_fopen */

/*
 * sfs_fclose: close closes a file descriptor, so that it no longer
 *   refers to any file and may be reused.
 *
 * Parameters: -
 *
 * Returns: 0 on success, or -1 if an error occurred
 *
 */
int sfs_fclose(int fileID) {
    // close the file descriptor represented by fileID? what does that entail exactly?
    return -1;
} /* !sfs_fclose */

/*
 * sfs_fread: attempts to read up to length bytes from file
 *   descriptor fileID into the buffer starting at buffer
 *
 * Parameters: file descriptor, buffer to read and its lenght
 *
 * Returns: on success, the number of bytes read are returned. On
 *   error, -1 is returned
 *
 */
int sfs_fread(int fileID, char *buffer, int length) {
    // get the file descriptor
	// find out which sectors comprise that file
	// copy the length bytes from the sectors into buffer - do we need to start at an offset stored in the descriptor?
    return -1;
}

/*
 * sfs_fwrite: writes up to length bytes to the file referenced by
 *   fileID from the buffer starting at buffer
 *
 * Parameters: file descriptor, buffer to write and its lenght
 *
 * Returns: on success, the number of bytes written are returned. On
 *   error, -1 is returned
 *
 */
int sfs_fwrite(int fileID, char *buffer, int length) {
    // get the file descriptor
	// find out which sectors comprise that file
	// copy length bytes from the buffer into the sectors - do we need to start at an offset stored in the descriptor?
    return -1;
} /* !sfs_fwrite */

/*
 * sfs_lseek: reposition the offset of the file descriptor 
 *   fileID to position
 *
 * Parameters: file descriptor and new position
 *
 * Returns: Upon successful completion, lseek returns the resulting
 *   offset location, otherwise the value -1 is returned
 *
 */
int sfs_lseek(int fileID, int position) {
    // get the file descriptor
	// change the offset to position
    return -1;
} /* !sfs_lseek */

/*
 * sfs_rm: removes a file in the current directory by name if it exists.
 *
 * Parameters: file name
 *
 * Returns: 0 on success, or -1 if an error occurred
 */
int sfs_rm(char *file_name) {
    // TODO: Implement for extra credit
    return -1;
} /* !sfs_rm */
