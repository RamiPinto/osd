/*
 * OPERATING SYSTEMS DESING - 16/17
 *
 * @file 	filesystem.c
 * @brief 	Implementation of the core file system funcionalities and auxiliary functions.
 * @date	01/03/2017
 */

#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "include/filesystem.h"		// Headers for the core functionality
#include "include/auxiliary.h"		// Headers for auxiliary functions
#include "include/metadata.h"		// Type and structure declaration of the file system
#include "include/crc.h"			// Headers for the CRC functionality


struct superblock *sblocks;
struct fs_bitmap *bitmaps;
struct inode *inodes;

/*
 * @brief 	Generates the proper file system structure in a storage device, as designed by the student.
 * @return 	0 if success, -1 otherwise.
 */
int mkFS(long deviceSize)
{
	int i;

	//Check device size
	if(deviceSize < MIN_DISK_SIZE || deviceSize > MAX_DISK_SIZE){
		printf("[ERROR] No valid device size\n");
		return -1;
	}

	char reset[BLOCK_SIZE];
	bzero(reset, BLOCK_SIZE);
	for (i = 0; i < (deviceSize/BLOCK_SIZE); i++){
		if (bwrite(DEVICE_IMAGE, i, reset) == -1){
			printf("[ERROR] Error reseting device: %d\n", i);
			return -1;
		}
	}

	//Reset superblocks, maps and inodes
	sblocks = malloc(sizeof(struct superblock));
	sblocks[0].magicNum = 0x29A;
	sblocks[0].mapNumBlocks = 3;
	sblocks[0].numinodes = NUM_INODES; //TODO: Check if MAX_FILES or NUM_INODES
	sblocks[0].firstinode = 5;
	sblocks[0].dataNumBlock = (unsigned int) ((deviceSize-7*BLOCK_SIZE)/BLOCK_SIZE);
	sblocks[0].firstDataBlock = 7;
	sblocks[0].deviceSize = deviceSize;
	sblocks[0].crc = 0;

	/*bitmaps = malloc(sizeof(struct fs_bitmap));

	//Free inodes bitmap
	for(i = 0; i < sblocks[0].numinodes; i++){
		bitmaps[0].inodes_map[i] = 0;
	}

	//Free data blocks bitmap
	for(i = 0; i < sblocks[0].dataNumBlock; i++){
		bitmaps[0].dataBlocks_map[i] = 0;
	}

	for(i = 0; i < sblocks[0].numinodes; i++){
		memset(&(inodes[i]), 0, sizeof(struct inode));
	}*/


	//Unmount the file system from the device to write the default file system into disk
	if(unmountFS() == -1){
		printf("[ERROR] Cannot unmount the file system\n");
		return -1;
	}

	return 0;
}

/*
 * @brief 	Mounts a file system in the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int mountFS(void)
{
	int i,j;
	char buf[BLOCK_SIZE], bm_buf[3*BLOCK_SIZE];

	//Allocate memory
	sblocks = malloc(sizeof(struct superblock));
	bitmaps = malloc(sizeof(struct fs_bitmap));
	inodes = malloc (sizeof(struct inode) * NUM_INODES);

	//Read superblock
	if(bread(DEVICE_IMAGE, 1, buf) == -1){
		printf("[ERROR] Cannot mount the fs. Error reading superblock\n");
		return -1;
	}
	struct superblock *temp_sb = (struct superblock *) buf;
	sblocks[0] = *temp_sb;

	//Read bitmaps
	for(i = 0; i < 3 ; i++){	//bitmaps fill 3 blocks in memory
		if(bread(DEVICE_IMAGE, i+2, bm_buf + (i*BLOCK_SIZE)) == -1){
			printf("[ERROR] Cannot mount the fs. Error reading bitmaps\n");
			return -1;
		}
	}
	struct fs_bitmap *temp_bm = (struct fs_bitmap *) bm_buf;
	bitmaps[0] = *temp_bm;

	//Read inodes
	for(i = 0; i < 2 ; i++){	//inodes fill 2 blocks in memory
		if(bread(DEVICE_IMAGE, (i + sblocks[0].firstinode), buf) == -1 ){
			printf("[ERROR] Cannot mount the fs. Error reading inodes\n");
			return -1;
		}
		else{
			for(j = 0; j < 32; j++){
				char inode_buf[sizeof(struct inode)];
				memcpy(inode_buf, buf + (sizeof(struct inode) * j), sizeof(struct inode)); //Copy single inodes
				struct inode *temp_in = (struct inode *) inode_buf;
				inodes[j] = *temp_in;
			}
		}
	}

	//TODO: Check integrity
	/*if(checkFS() == -1){
		printf("[ERROR] Cannot mount the fs. Error checking data integrity. ");
		return -1;
	} */

	return 0;
}

/*
 * @brief 	Unmounts the file system from the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int unmountFS(void)
{
	int i;

	//Write inodes
	for(i = 0; i < 2 ; i++){	//inodes fill 2 blocks in memory
		if(bwrite(DEVICE_IMAGE, (i + sblocks[0].firstinode), ((char *) inodes + (i*BLOCK_SIZE))) == -1 ){
			printf("[ERROR] Cannot unmount the fs. Error writing inodes\n");
			return -1;
		}
	}

	//Write bitmaps
	for(i = 0; i < 3 ; i++){	//bitmaps fill 3 blocks in memory
		if(bwrite(DEVICE_IMAGE, i+2, ((char *) bitmaps + (i*BLOCK_SIZE))) == -1){
			printf("[ERROR] Cannot unmount the fs. Error writing bitmaps\n");
			return -1;
		}
	}

	//TODO: CALCULATE NEW CRC

	//Write superblock
	if(bwrite(DEVICE_IMAGE, 1, (char *) sblocks) == -1){
		printf("[ERROR] Cannot unmount the fs. Error writing superblock\n");
		return -1;
	}

	//Free resources
	free(inodes);
	free(bitmaps);
	free(sblocks);

	return 0;
}

/*
 * @brief	Creates a new file, provided it it doesn't exist in the file system.
 * @return	0 if success, -1 if the file already exists, -2 in case of error.
 */
int createFile(char *fileName)
{
	return -2;
}

/*
 * @brief	Deletes a file, provided it exists in the file system.
 * @return	0 if success, -1 if the file does not exist, -2 in case of error..
 */
int removeFile(char *fileName)
{
	return -2;
}

/*
 * @brief	Opens an existing file.
 * @return	The file descriptor if possible, -1 if file does not exist, -2 in case of error..
 */
int openFile(char *fileName)
{
	return -2;
}

/*
 * @brief	Closes a file.
 * @return	0 if success, -1 otherwise.
 */
int closeFile(int fileDescriptor)
{
	return -1;
}

/*
 * @brief	Reads a number of bytes from a file and stores them in a buffer.
 * @return	Number of bytes properly read, -1 in case of error.
 */
int readFile(int fileDescriptor, void *buffer, int numBytes)
{
	return -1;
}

/*
 * @brief	Writes a number of bytes from a buffer and into a file.
 * @return	Number of bytes properly written, -1 in case of error.
 */
int writeFile(int fileDescriptor, void *buffer, int numBytes)
{
	return -1;
}


/*
 * @brief	Modifies the position of the seek pointer of a file.
 * @return	0 if succes, -1 otherwise.
 */
int lseekFile(int fileDescriptor, long offset, int whence)
{
	return -1;
}

/*
 * @brief 	Verifies the integrity of the file system metadata.
 * @return 	0 if the file system is correct, -1 if the file system is corrupted, -2 in case of error.
 */
int checkFS(void)
{
	return -2;
}

/*
 * @brief 	Verifies the integrity of a file.
 * @return 	0 if the file is correct, -1 if the file is corrupted, -2 in case of error.
 */
int checkFile(char *fileName)
{
	return -2;
}
