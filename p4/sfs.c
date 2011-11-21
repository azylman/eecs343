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
#include <string.h>
#include <sys/param.h>
 
#include "sfs.h"
#include "sdisk.h"

typedef struct inode_s {
	bool isFile;
	int parent;
	int cont;
	char name[16];
	int num;
} inode;

typedef struct inodeFile_s {
	bool isFile;
	int parent;
	int cont;
	char name[16];
	int num;
	int sectors[6];
	int filesize;
} inodeFile;

typedef struct inodeDir_s {
	bool isFile;
	int parent;
	int cont;
	char name[16];
	int num;
	int children[6];
} inodeDir;

typedef struct fileDescriptor_s {
	inode* INODE;
	int currentPos;
	char* data;
	struct fileDescriptor_s* next;
} fileDescriptor;

typedef struct tokenResult_s {
	int numTokens;
	char** tokens;
} tokenResult;

static int sectorBitmapSizeInSectors = -1;
static int inodeBitmapSizeInSectors = -1;
static int inodeArraySizeInSectors = -1;

static int rootInodeNum = -1;

static int cwd = -1;

// some static file containing info on all open files

static short DEBUG = 0;

static int inodeSize = sizeof(inodeFile) > sizeof(inodeDir) ? sizeof(inodeFile) : sizeof(inodeDir);

Sector* getSector(int);

void markSectorAsUsed(int);
void markSectorAsNotUsed(int);

void markInodeAsUsed(int);
void markInodeAsNotUsed(int);

int getNextFreeInode();

int createInode();
inode* getInode(int);
void saveInode(inode*);

void initDir(inodeDir*, int, int, int, char[16]);
void initFile(inodeFile*, int, int, int, char[16]);
void initInode(inode*, int, bool, int, int, char[16]);

void setBit(int*, int);
void clearBit(int*, int);
void toggleBit(int*, int);
int getBit(int*, int);
void initSector(int);

tokenResult* parsePath(char*);

void addChild(inodeDir*, int);

inode* getCont(inode*);

Sector* getSector(int sector) {
	Sector* retrievedSector = malloc(sizeof(Sector));
	SD_read(sector, retrievedSector);
	return retrievedSector;
}

// Mark a sector as in use in our bitmap
void markSectorAsUsed(int sector) {
	int bitmapSectorNumber = floor( sector / SD_SECTORSIZE );
	int sectorOffset = sector % SD_SECTORSIZE;
	
	Sector* bitmapSector = getSector(bitmapSectorNumber);
	
	int* intToInspect = (int*)bitmapSector + (int)(floor( sectorOffset / 32 ));
	
	setBit(intToInspect, sectorOffset % 32);
	SD_write(bitmapSectorNumber, bitmapSector);
	
	free(bitmapSector);
}

// Mark a sector as not in use in our bitmap
void markSectorAsNotUsed(int sector) {
	int bitmapSectorNumber = floor( sector / SD_SECTORSIZE );
	int sectorOffset = sector % SD_SECTORSIZE;
	
	Sector* bitmapSector = getSector(bitmapSectorNumber);
	
	int* intToInspect = (int*)bitmapSector + (int)(floor( sectorOffset / 32 ));
	
	clearBit(intToInspect, sectorOffset % 32);
	SD_write(bitmapSectorNumber, bitmapSector);
	
	free(bitmapSector);
}

void markInodeAsUsed(int inodeNumber) {
	int inodeSectorNumber = floor( inodeNumber / SD_SECTORSIZE ) + sectorBitmapSizeInSectors;
	int sectorOffset = inodeNumber % SD_SECTORSIZE;
	
	Sector* inodeSector = getSector(inodeSectorNumber);
	
	int* intToInspect = (int*)inodeSector + (int)(floor( sectorOffset / 32 ));
	
	setBit(intToInspect, sectorOffset % 32);
	SD_write(inodeSectorNumber, inodeSector);
	
	free(inodeSector);
}

void markInodeAsNotUsed(int inodeNumber) {
	int inodeSectorNumber = floor( inodeNumber / SD_SECTORSIZE ) + sectorBitmapSizeInSectors;
	int sectorOffset = inodeNumber % SD_SECTORSIZE;
	
	Sector* inodeSector = getSector(inodeSectorNumber);
	
	int* intToInspect = (int*)inodeSector + (int)(floor( sectorOffset / 32 ));
	
	clearBit(intToInspect, sectorOffset % 32);
	SD_write(inodeSectorNumber, inodeSector);
	
	free(inodeSector);
}

int getNextFreeInode() {
	int fullIntegerBitmap = ~0;
	int value = 0;
	
	int secNum = sectorBitmapSizeInSectors;
	
	Sector* bitmap = getSector(secNum);
	int* curPos = (int*)bitmap;
	
	if (DEBUG) printf("Our bitmap is %i, ", *(int*)bitmap);
	
	while (*(int*)curPos == fullIntegerBitmap) {
		if (DEBUG) printf("we need a new int boundary, ");
		value += 32;
		if (DEBUG) printf("our bitmap pointer goes from %p ", curPos);
		curPos++;
		if (DEBUG) printf("to %p, where the value of curPos is %i, ", curPos, *(int*)curPos);
		
		if (value % sizeof(Sector) == 0) {
			secNum++;
			free(bitmap);
			bitmap = getSector(secNum);
			curPos = (int*)bitmap;
		}
	}
	
	while ((*(int*)curPos & 1) != 0) {
		value++;
		*(int*)curPos >>= 1;
	}
	
	free(bitmap);
	
	if (DEBUG) printf("and our next free one is %i\n", value);
	
	return value;
}

int createInode() {
	int inodeNum = getNextFreeInode();
	markInodeAsUsed(inodeNum);
	return inodeNum;
}

inode* getInode(int inodeNum) {
	int inodesPerSector = floor( (double)SD_SECTORSIZE / (double)inodeSize );
	int inodeSectorNumber = floor( (double)inodeNum / (double)inodesPerSector ) + sectorBitmapSizeInSectors + inodeBitmapSizeInSectors;
	
	Sector* inodeSector = getSector(inodeSectorNumber);
	char* inodeList = (char*)inodeSector;
	
	int inodeOffset = inodeNum % inodesPerSector;
	int byteOffset = inodeOffset * inodeSize;
	inodeList += byteOffset;
	
	inode* resultInode = malloc(inodeSize);
	memcpy(resultInode, inodeList, inodeSize);
	
	free(inodeSector);
	return resultInode;
}

void saveInode(inode* INODE) {
	int inodeNum = INODE->num;
	int inodesPerSector = floor( (double)SD_SECTORSIZE / (double)inodeSize );
	int inodeSectorNumber = floor( (double)inodeNum / (double)inodesPerSector ) + sectorBitmapSizeInSectors + inodeBitmapSizeInSectors;
	
	Sector* inodeSector = getSector(inodeSectorNumber);
	char* inodeList = (char*)inodeSector;
	
	int inodeOffset = inodeNum % inodesPerSector;
	int byteOffset = inodeOffset * inodeSize;
	
	inodeList += byteOffset;
	
	inode* inMem = (inode*)INODE;
	inode* onDisk = (inode*)inodeList;
	
	onDisk->parent = inMem->parent;
	onDisk->cont = inMem->cont;
	
	strcpy(onDisk->name, inMem->name);
	
	onDisk->num = inMem->num;
	
	if (INODE->isFile) {
		inodeFile* inMemFile = (inodeFile*)INODE;
		inodeFile* onDiskFile = (inodeFile*)inodeList;
		
		memcpy(onDiskFile->sectors, inMemFile->sectors, sizeof(int)*6);
		onDiskFile->filesize = inMemFile->filesize;
	} else {
		inodeDir* inMemDir = (inodeDir*)INODE;
		inodeDir* onDiskDir = (inodeDir*)inodeList;
		
		memcpy(onDiskDir->children, inMemDir->children, sizeof(int)*6);
	}
	
	Sector* inodeSectorCopy = inodeSector;
	SD_write(inodeSectorNumber, inodeSectorCopy);
	
	free(inodeSector);
}

void initDir(inodeDir* dir, int inodeNum, int parent, int cont, char name[16]) {
	int i;
	for (i = 0; i < 6; ++i) {
		dir->children[i] = -1;
	}
	initInode((inode*) dir, inodeNum, 0, parent, cont, name);
}
void initFile(inodeFile* file, int inodeNum, int parent, int cont, char name[16]) {
	int i;
	for (i = 0; i < 6; ++i) {
		file->sectors[i] = -1;
	}
	initInode((inode*) file, inodeNum, 1, parent, cont, name);
}
void initInode(inode* INODE, int inodeNum, bool isFile, int parent, int cont, char name[16]) {
	INODE->isFile = isFile;
	INODE->parent = parent;
	INODE->cont = cont;
	strcpy(INODE->name, name);
	INODE->num = inodeNum;
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
	int* intBoundary = (int*)bitmapSector;
	SD_read(sector, bitmapSector);
	int i;
	for (i = 0; i < SD_SECTORSIZE / sizeof(int); i++) {
		*intBoundary = 0;
		intBoundary++;
	}
	*(int*)bitmapSector = 0;
	SD_write(sector, bitmapSector);
	free(bitmapSector);
}

tokenResult* parsePath(char* path) {

	char* pathCopy = malloc(sizeof(char) * 20);
	
	strcpy(pathCopy, path);
	tokenResult* tokens = malloc(sizeof(tokenResult));
	tokens->tokens = malloc(MAXPATHLEN);
	char* token;
	token = strtok(pathCopy, "/");
	int numTokens = 0;
	while (token != NULL) {
		tokens->tokens[numTokens] = (char*)malloc(sizeof(char) * 17);
		strcpy(tokens->tokens[numTokens], token);
		numTokens++;
		token = strtok(NULL, "/");
	}
	
	free(pathCopy);
	tokens->numTokens = numTokens;
	return tokens;
}

void addChild(inodeDir* parent, int childNum) {
	int i;
	bool added = 0;
	for (i = 0; i < 6; i++) {
		if (parent->children[i] == -1) {
			added = 1;
			parent->children[i] = childNum;
			break;
		}
	}
	if (!added) {
		printf("Checking if we need to add a continuing inode...\n");
		if (parent->cont == -1) {
			int contInodeNum = createInode();
			inodeDir* contInode = (inodeDir*)getInode(contInodeNum);
			initDir(contInode, contInodeNum, -1, -1, "");
			parent->cont = contInodeNum;
			saveInode((inode*)contInode);
			free(contInode);
		}
		printf("Adding the child to the continuing inode...\n");
		inode* contInode = getInode(parent->cont);
		addChild((inodeDir*)contInode, childNum);
		saveInode(contInode);
		free(contInode);
	}
}

inode* getCont(inode* INODE) {
	if (INODE->cont == -1) {
		return 0;
	}
	int contInodeNum = INODE->cont;
	free(INODE);
	return getInode(contInodeNum);
}

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
	sectorBitmapSizeInSectors = ceil( (double)numBytes / (double)SD_SECTORSIZE );
	
	int numInodes = SD_NUMSECTORS - sectorBitmapSizeInSectors;
	inodeBitmapSizeInSectors = ceil( (double)numInodes / (double)SD_SECTORSIZE / (double)8 );
	
	inodeArraySizeInSectors = ceil( (double)numInodes * (double)inodeSize / (double)SD_SECTORSIZE );
	
	int i;
	for(i = 0; i < sectorBitmapSizeInSectors + inodeBitmapSizeInSectors + inodeArraySizeInSectors; ++i) {
		initSector(i);
		markSectorAsUsed(i);
	}
	
	rootInodeNum = createInode();
	cwd = rootInodeNum; // initialize current working directory to root inode
	
	inodeDir* rootInode = (inodeDir*)getInode(cwd);
	initDir(rootInode, rootInodeNum, -1, -1, "");
	saveInode((inode*)rootInode);
	free(rootInode);
	
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

	printf("Creating folder %s\n", name);

    bool error = 0;
	bool absolute = name[0] == '/';
	int result = absolute ? rootInodeNum : cwd;
	inodeDir* workingDir = (inodeDir*)getInode(result);
	
	tokenResult* tokens = parsePath(name);
	int i;
	for (i = 0; i < tokens->numTokens - 1; i++) {
		if (strcmp(tokens->tokens[i], ".") == 0) {
			// do nothing
		} else {
			if (strcmp(tokens->tokens[i], "..") == 0) {
				if (workingDir->parent != -1) {
					int parent = workingDir->parent;
					free(workingDir);
					printf("Setting result to %i\n", workingDir->parent);
					workingDir = (inodeDir*)getInode(parent);
					result = workingDir->parent;
				} else {
					error = 1;
				}
			} else {
				inodeDir* workingDirCont = workingDir;
				bool found = 0;
				do {
					int j;
					for (j = 0; j < 6; j++) {
						if (workingDirCont->children[j] != -1) {
							inode* child = getInode(workingDirCont->children[j]);
							if (strcmp(child->name, tokens->tokens[i]) == 0) {
								if (!child->isFile) {
									printf("Setting result to %i\n", workingDirCont->children[j]);
									result = workingDirCont->children[j];
									free(workingDir);
									workingDir = (inodeDir*)child;
									found = 1;
									break;
								} else {
									error = 1;
									found = 1;
									break;
								}
							}
						}
					}
					if (found) break;
				} while( (workingDirCont = (inodeDir*)getCont((inode*)workingDirCont)) != 0);
				if (!found) {
					error = 1;
				}
			}
		}
	}
	
	if (!error) {
		int newInode = createInode();
		inode* child = getInode(newInode);
		
		printf("Creating child %i with a parent of %i\n", newInode, result);
		printf("Initializing directory...\n");
		initDir((inodeDir*)child, newInode, result, -1, tokens->tokens[tokens->numTokens - 1]);
		printf("Adding child...\n");
		addChild(workingDir, newInode);
		printf("Saving child...\n");
		saveInode((inode*)child);
		printf("Saving working directory...\n");
		saveInode((inode*)workingDir);
		
		printf("Trying to free working directory...\n");
		free(workingDir);
		printf("Trying to free child...\n");
		free(child);
		return 0;
	} else {
		return -1;
	}
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

	printf("Cding to %s\n", name);

	bool error = 0;
	bool absolute = name[0] == '/';
	int result = absolute? 0 : cwd;
	inodeDir* workingDir = (inodeDir*)getInode(result);
	tokenResult* tokens = parsePath(name);
	
	if (tokens->numTokens == 0) {
		cwd = rootInodeNum;
		free(workingDir);
		return 0;
	}
	
	int i;
	for (i = 0; i < tokens->numTokens; i++) {
		if (strcmp(tokens->tokens[i], ".") == 0) {
			// do nothing
		} else {
			if (strcmp(tokens->tokens[i], "..") == 0) {
				if (workingDir->parent != -1) {
					int parent = workingDir->parent;
					result = workingDir->parent;
					free(workingDir);
					workingDir = (inodeDir*)getInode(parent);
				} else {
					error = 1;
				}
			} else {
				inodeDir* workingDirCont = workingDir;
				bool found = 0;
				do {
					int j;
					for (j = 0; j < 6; j++) {
						if (workingDirCont->children[j] != -1) {
							inode* child = getInode(workingDirCont->children[j]);
							if (strcmp(child->name, tokens->tokens[i]) == 0) {
								if (!child->isFile) {
									result = workingDirCont->children[j];
									free(workingDir);
									workingDir = (inodeDir*)child;
									found = 1;
									break;
								} else {
									error = 1;
									found = 1;
									break;
								}
							}
						}
					}
					if (found) break;
				} while( (workingDirCont = (inodeDir*)getCont((inode*)workingDirCont)) != 0);
				if (!found) {
					error = 1;
				}
			}
		}
	}
	if (!error) {
		free(workingDir);
		cwd = result;
		return 0;
	} else {
		return -1;
	}
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
    inodeDir* cwi = (inodeDir*)getInode(cwd);
	do {
		int i;
		for (i = 0; i < 6; ++i) {
			if (cwi->children[i] != -1) {
				inode* child = getInode(cwi->children[i]);
				fprintf(f, "%s\n", child->name);
				free(child);
			}
		}
	} while( (cwi = (inodeDir*)getCont((inode*)cwi)) != 0);
	free(cwi);
    return 0;
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
    // find the inode based on the name
	// write file info to our file descriptors array
	// and return the index into the array
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
    // write any changes in the data to the disk
	// write any changes in the inode to the disk
	// delete the file descriptor
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
	// copy the length bytes from the sectors into buffer
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
	// copy length bytes from the buffer into the sectors
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
