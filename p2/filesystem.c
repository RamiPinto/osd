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
	int i, b;
	if(namei(fileName) != -1){
		printf("[ERROR] Error creating file. The file already exists\n");
		return -1;
	}

	if((i = ialloc())==-1){
		printf("[ERROR] Error creating file. Cannot allocate inode\n");
		return -2;
	}
	if((b = balloc())==-1){
		printf("[ERROR] Error creating file. Cannot allocate data block\n");
		return -2;
	}

	//Set inode data
	strcpy(inodes[i].name, fileName);
	inodes[i].size = 1;
	inodes[i].status = CLOSE;
	inodes[i].position = 0;
	inodes[i].directBlock = b;
	inodes[i].crc = 0;

	return 0;
}

/*
 * @brief	Deletes a file, provided it exists in the file system.
 * @return	0 if success, -1 if the file does not exist, -2 in case of error..
 */
int removeFile(char *fileName)
{
	int i, j;

	//Check if file exists
	if((i = namei(fileName)) == -1){
		printf("[ERROR] Cannot remove file %s. The file doesn't exist\n", fileName);
		return -1;
	}

	if(ifree(i) == -1){
		printf("[ERROR] Cannot remove file %s. Error freeing bitmap position\n", fileName);
		return -2;
	}

	for(j = inodes[i].directBlock; j<inodes[i].size;i++){
		if(bfree(j) == -1){
			printf("[ERROR] Cannot remove file %s. Error freeing datablock\n", fileName);
			return -2;
		}
	}

	return 0;
}

/*
 * @brief	Opens an existing file.
 * @return	The file descriptor if possible, -1 if file does not exist, -2 in case of error..
 */
int openFile(char *fileName)
{
	int i;

	//Check if file exists
	if((i = namei(fileName)) == -1){
		printf("[ERROR] Cannot open file %s. The file doesn't exist\n", fileName);
		return -1;
	}

	//Check file integrity
	if (checkFile(fileName) == -1){
		printf("[ERROR] Cannot open file %s. The file is corrupted\n", fileName);
		return -2;
	}

	//Check if it is already open
	if(inodes[i].status == OPEN){
		printf("[ERROR] Cannot open file %s. The file is already opened\n", fileName);
		return -2;
	}

	inodes[i].status = OPEN;

	return i;
}

/*
 * @brief	Closes a file.
 * @return	0 if success, -1 otherwise.
 */
int closeFile(int fileDescriptor)
{
	if (fileDescriptor < 0 || fileDescriptor >= MAX_FILES){
		printf("[ERROR] Cannot close file %d. Invalid file descriptor\n", fileDescriptor);
		return -1;
	}

	if(inodes[fileDescriptor].status == CLOSE){
		printf("[ERROR] Cannot close file %d. It is already closed\n", fileDescriptor);
		return -1;
	}

	inodes[fileDescriptor].status = CLOSE;

	return 0;
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

	if (fileDescriptor >= MAX_FILES || fileDescriptor < 0){
		printf("[ERROR] Cannot modify the position of the seek pointer. Invalid file descriptor\n");
		return -1;
	}
	if(inodes[fileDescriptor].status == CLOSE){
		printf("[ERROR] Cannot modify the position of the seek pointer. The file is closed\n");
		return -1;
	}

	int new_position = inodes[fileDescriptor].position + offset;
	switch (whence) {

		case FS_SEEK_CUR:
			if (new_position < 0 || new_position > inodes[fileDescriptor].size)
				return -1;
			else inodes[fileDescriptor].position = new_position;
		break;

		case FS_SEEK_END:
			inodes[fileDescriptor].position = inodes[fileDescriptor].size;
		break;

		case FS_SEEK_BEGIN:
			inodes[fileDescriptor].position = 0;
		break;

		default: return -1;
	}

	return 0;
}

/*
 * @brief 	Verifies the integrity of the file system metadata.
 * @return 	0 if the file system is correct, -1 if the file system is corrupted, -2 in case of error.
 */
int checkFS(void)
{
	if(sblocks[0].crc != CRCheck(SB_ID, -1)){
		return -1;
	}
	return 0;
}

/*
 * @brief 	Verifies the integrity of a file.
 * @return 	0 if the file is correct, -1 if the file is corrupted, -2 in case of error.
 */
int checkFile(char *fileName)
{
	int i;
	if((i = namei(fileName)) == -1){
		printf("[ERROR] Cannot check the file %s. The file doesn't exist\n", fileName);
		return -2;
	}

	if (inodes[i].crc != CRCheck(F_ID, i)){
		printf("[ERROR] The file %s is corrupted\n", fileName);
		return -1;
	}

	return 0;
}


/********************************************************/
/*ADDITIONAL FUNCTIONS*/

int ialloc()
{
	int i;
	for(i = 0; i < sblocks[0].numinodes; i++){
		if(bitmaps->inodes_map[i] == 0){	//Free inodes
			bitmaps->inodes_map[i] = 1;
			return i;
		}
	}

	//No free inodes
	return -1;
}

int ifree(int i)
{
	if(i > sblocks[0].numinodes || i < 0){
		printf("[ERROR] Cannot free inode. No valid inode id\n");
		return -1;
	}

	bzero(inodes[i].name, MAX_FILE_NAME);
	bitmaps->inodes_map[i] = 0;

	return 0;
}

int balloc()
{
	int i;
	for(i = 0; i < sblocks[0].dataNumBlock; i++){
		if(bitmaps->dataBlocks_map[i] == 0){
			bitmaps->dataBlocks_map[i] = 1;
			return i;
		}
	}
	return -1;
}

int bfree(int i)
{
	char buf[BLOCK_SIZE];

	if(i > sblocks[0].dataNumBlock || i < 0){
		printf("[ERROR] Cannot free data block. No valid data block id\n");
		return -1;
	}

	bzero(buf, BLOCK_SIZE);
	if(bwrite(DEVICE_IMAGE, (i + sblocks[0].firstDataBlock), buf) == -1){
		printf("[ERROR] Cannot free data block. Error removing block data\n");
		return -1;
	}
	bitmaps->dataBlocks_map[i] = 0;
	return 0;
}

int namei(char *name)
{
	int i, result=-1;
	for (i = 0; i < sblocks[0].numinodes; i++){
		if(!strcmp(inodes[i].name, name)){
			result = i;
		}
		else{
			result = -1;
		}
	}
	return result;
}

int bmap(int i, int offset)
{
	if(i>sblocks[0].numinodes || i<0 || offset < 0){
		printf("[ERROR] Cannot locate data block. No valid data block id\n");
		return -1;
	}

	//Return inode block
	if(offset < BLOCK_SIZE){
		return inodes[i].directBlock;
	}

	return -1;
}

int myceil(double x){
	int y = (int) x, result;
	if((x - y) > 0){
		result = y+1;
	}
	else{
		result = y;
	}

	return result;
}


uint32_t CRCheck(int type, int i)
{
	uint32_t result = -1;
	char buf[BLOCK_SIZE], *temp_buf;
	int j;

	switch(type) {
		case SB_ID: //Superblock: get metadata (inodes + bitmaps)
			temp_buf = malloc(5*BLOCK_SIZE); //3 blocks bitmaps + 2 blocks inodes

			//Read bitmaps
			for(j = 0; j < 3 ; j++){	//bitmaps fill 3 blocks in memory
				if(bread(DEVICE_IMAGE, j+2, buf + (j*BLOCK_SIZE)) == -1){
					printf("[ERROR] Cannot mount the fs. Error reading bitmaps\n");
					return -1;
				}
				else{
					strncpy((char *) temp_buf + (j * BLOCK_SIZE), buf, BLOCK_SIZE);
				}
			}

			//Read inodes
			for(j = 0; j < 2 ; j++){	//inodes fill 2 blocks in memory
				if(bread(DEVICE_IMAGE, (j + sblocks[0].firstinode), buf) == -1 ){
					printf("[ERROR] Cannot mount the fs. Error reading inodes\n");
					return -1;
				}
				else{
					strncpy((char *) temp_buf + ((3 + j) * BLOCK_SIZE), buf, BLOCK_SIZE);
				}
			}

			result = CRC32((const unsigned char *) temp_buf, 5*BLOCK_SIZE, sblocks[0].crc);
			free(temp_buf);
		break;

		case F_ID:
			if(i<0 || i>=MAX_FILES){
				printf("[ERROR] Cannot execute CRC. No valid inode id\n");
				return -1;
			}

			temp_buf = malloc(inodes[i].size);
			int num_blocks = myceil(inodes[i].size/BLOCK_SIZE);

			for(j = 0; j < num_blocks; j++){
				if(bread(DEVICE_IMAGE, (j + inodes[i].directBlock), buf) == -1){
					printf("[ERROR] Cannot execute CRC. Error reading file\n");
					return -1;
				}
				else{
					strncpy((char *) temp_buf + (j * BLOCK_SIZE), buf, BLOCK_SIZE);
				}
			}

			result = CRC32((const unsigned char *) temp_buf, inodes[i].size, inodes[i].crc);
			free(temp_buf);
		break;

		default:
			result=-1;
	}

	return result;
}
