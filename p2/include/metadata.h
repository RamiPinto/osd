/*
 * OPERATING SYSTEMS DESING - 16/17
 *
 * @file 	metadata.h
 * @brief 	Definition of the structures and data types of the file system.
 * @date	01/03/2017
 */

#define MAX_FILES 40				//NF1
#define MAX_FILE_NAME 32			//NF2
//#define MAX_FILE_SIZE 1048576		//NF3 (1 MiB)
//#define BLOCK_SIZE 2048				//NF4
#define MAX_BLOCKS 5120				//Obtained from [ (MAX_DISK_SIZE-(MAX_FILES+4))/BLOCK_SIZE ]
#define MIN_DISK_SIZE 51200
#define MAX_DISK_SIZE 10485760

#define CLOSE 0
#define OPEN 1

#define SB_PADDING 2008
#define I_PADDING 1991
#define BM_PADDING 984


#define bitmap_getbit(bitmap_, i_) (bitmap_[i_ >> 3] & (1 << (i_ & 0x07)))
static inline void bitmap_setbit(char *bitmap_, int i_, int val_) {
	if (val_)
		bitmap_[(i_ >> 3)] |= (1 << (i_ & 0x07));
	else
		bitmap_[(i_ >> 3)] &= ~(1 << (i_ & 0x07));
}

typedef struct {
	unsigned int magicNum;			//Magic number fo the superblock
	unsigned int InodeMapNumBlocks;	// Number of blocks of the inode map
	unsigned int numinodes;			//Number of i-nodes in the deviceSize
	unsigned int firstinode;		//Number fo the first inode in the device
	unsigned int DataMapNumBlock;	//Number of blocks of the data map
	unsigned int dataBlockNum;		//Number of data blocks in the device
	unsigned int firstDataBlock;	// Number fo first data block
	unsigned int deviceSize;		// Total disk space
	uint32_t crc;					//cr value for checking integrity
	char padding[SB_PADDING];		//Padding field to fulfill a block
} superblock;

typedef struct {
	char name[MAX_FILE_NAME+1];		//File name
	unsigned int size;				//Current file size in bytes
	unsigned int directBlock;		//Direct block number
	unsigned int position;			//Seek pointer position
	unsigned int status;			//OPEN/CLOSE
	uint32_t crc; 					//cr value for checking integrity
	char padding[I_PADDING];		//Padding field to fill a block
} inode;

typedef struct {
	char iNode[MAX_FILES];
	char d_blocks[MAX_BLOCKS];
	char padding[BM_PADDING];		// Padding field to fill three blocks
} fs_bitmap;
