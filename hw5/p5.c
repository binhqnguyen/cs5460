
#include <string.h>
#include <stdio.h>


#define BLOCK_SIZE 1024			/* each block has a size of 1024B */


#define INODE_NUM 5000 			/*maximum number of files 5000*/
#define INODE_START 34
#define INODE_END 193			/*159 blocks for i-node segment*/
#define INODE_MAP 1				/*1block = 1024B for i-node bitmap*/

#define BLOCK_MAP_START 2		/*Block's bitmap beginning position*/
#define BLOCK_MAP_END 33		/*Block's bitmap ending position*/
#define BLOCK_START 200


#define BLOCK_SIZE_BIT 8192
#define INODES_IN_BLOCK 32		/*number of i-nodes in each block*/

#define SET 0
#define RESET 1

#define RESET_CODE 0x7F
#define SET_CODE 0x80

/* limitations */
#define MAX_FILE_NAME_LENGTH 200	/*file length*/

#define MAX_PATH_DEPTH	10			/*maximum number of levels in a file path*/
#define MAX_FILENAME	50

/*block*/
struct superblock
{
	int inodeNum;	/*numer of i-nodes*/
	int inodeBitMapPosition;	/*i-node bitmap position*/
	int blockBitmapPosition;	/*block bitmap position*/
}superblock;

/*i-node*/
struct inode
{
	int fileOrFolder; 	/*file or folder*/
	int fileLength;	/*fileLength*/
	int directBlockPosition[4];	/*directBlockPosition[0]: position of folder*/
	int firstLevelIndirectBlock;
	int secondLevelIndirectBlock;
};

/*buffer block, for storage, data is a ASCII stream, each character is storaged in one integer*/
struct bufferBlock
{
 int bufferInt[256];
};

/*folder(folder)*/
struct folder
{
	char name[10][20];
	int inodePosition[10];
};



int highwatermarkforBlock;
int highwatermarkforInode;
int block_size;				/*number of blocks that the system has occupied*/

int openedFileNum;
int openedFiles[10];					/*files which has openned*/
int blockbuf[10];			/*read buffers*/

/*Get the depth of the given path, parse it and assign level names to argp[]*/
int resolveFilePath(char * path,char **folderNameArray)
{
	int i;
	int length;
	char * pathp = malloc(MAX_PATH_DEPTH*MAX_FILENAME);
	memset(pathp,'\0',sizeof(*pathp));

	length=strlen(path);
	strncpy(pathp,path,strlen(path));
	for(i=0;i<MAX_PATH_DEPTH;i++)
	{
		if((folderNameArray[i+1]=strtok(pathp,"/"))==NULL)		/*end of parsing*/
			break;
		pathp = NULL;
		if(i==0)		/*root folder is "/", one character*/
			length=length-1;
		else
			length=length-1-strlen(folderNameArray[i]);	/*each level has the name of "xxxx/", its length is (1+actual name)*/
	}
	folderNameArray[i][length]='\0';	/*assign NULL*/
	folderNameArray[0]="/";			/*root folder*/
	if(i>=MAX_PATH_DEPTH){
		printf("Too depth\n");
		return -1;
	}
	return i;
}

/*Initialize the buffer block. Buffer zone is filled with 0*/
void initBufferBlock(struct bufferBlock* buffBlock)
{
 int i;
  for(i=0;i<256;i++)
		buffBlock->bufferInt[i]=0;
}

/*Initialize a blank folder*/
void initFolder(struct folder* Folder)
{
	int i;
	for(i=0;i<10;i++)
	{
		Folder->name[i][0]='\0';
		Folder->inodePosition[i]=0;

	}
}

/*Initialize a blank I-node*/
void initInode(struct inode* iNode)
{
	int i;
	for(i=0;i<4;i++)
	{
		iNode->directBlockPosition[i]=0;	/*zero means no folder*/

	}
	iNode->firstLevelIndirectBlock=0;
	iNode->secondLevelIndirectBlock=0;
	iNode->fileOrFolder=0;
	iNode->fileLength=0;
}

/* open an exisiting file for reading or writing */
int my_open (char * path)
{
	int pathDepth;
	int i;
	int nodeIndex;
	struct inode iNode;
	char * folderNameArray[MAX_PATH_DEPTH];

	if(openedFileNum==10)	/*to many files opened*/
	{
		printf("There are too many files opened\n");
		return -1;
	}

	for(i=0;i<MAX_PATH_DEPTH;i++)
		folderNameArray[i]=malloc(MAX_FILENAME);

	/*pathDepth represents depth*/
	pathDepth= resolveFilePath(path,folderNameArray);
	/*nodeIndex represents inode index */
	nodeIndex= getInodeByFilePath(pathDepth+1,folderNameArray);


	if(nodeIndex<0)
	{   
		printf("Can't find the path");
		return -1;
	}

	getInodeByIndex(nodeIndex,&iNode);

	if(iNode.fileOrFolder==0)
	{
		printf("You can't open a dir folder\n");
		return -1;
	}
	openedFileNum++;
	for(i=0;i<10;i++)
	{
		if(openedFiles[i]==-1)	/*traversal openedFiles array, find the first fit place*/
		{
			openedFiles[i]=nodeIndex;	/*store the opened i-node's index*/
			blockbuf[i]=0;
			return i;
		}
	}

}

/* open a new file for writing only */
int my_creat (char * path)
{
	int pathDepth;
	int i,tmp;
	int nodeIndex;
	char* deepestFolderName;	/*name of the deepest folder of the given path*/
	struct inode iNode;
	struct folder Folder;
	struct inode newInode;
	int     newInodeIndex;
	char * folderNameArray[MAX_PATH_DEPTH];

	if(openedFileNum==10)	/*too many files created*/
	{
		printf("Too many files are created\n");
		return -1;
	}

	for(i=0;i<MAX_PATH_DEPTH;i++)	/*allocate memory*/
		folderNameArray[i]=malloc(MAX_FILENAME);


	pathDepth= resolveFilePath(path,folderNameArray);	/*parse path, get info*/

	deepestFolderName=malloc(MAX_FILENAME);
	strcpy(deepestFolderName,folderNameArray[pathDepth]);	/*get deepest folder name*/


	/*get the position of i-node that mentioned in the given path*/
	nodeIndex= getInodeByFilePath(pathDepth,folderNameArray);

	/*Can't find */
	if(nodeIndex<0)
	{   
		printf("Can't find the path");
		return -1;
	}
	/*get the i-node by its postion*/
	getInodeByIndex(nodeIndex,&iNode);


	if(iNode.fileOrFolder==1)
	{
		printf("The file of the inode  is not a folder\n");
		return -1;
	}
	else
	{
		getFolderByBlockIndex(iNode.directBlockPosition[0],&Folder);	/*get the folder inside this i-node*/
		for(i=0;i<10;i++)	/*check all the names inside this folder*/
		{
			if(strlen(Folder.name[i])==0)break;	/*found a free space to store this name*/
			if(strcmp(Folder.name[i],deepestFolderName)==0)	/*name collision*/
			{
				printf("there is a file having the same name as %s",deepestFolderName);
				return -1;
			}

		}
		strcpy(Folder.name[i],deepestFolderName); /*copy name to found position*/

		initInode(&newInode);	/*Initialize a empty i-node*/
		newInode.fileOrFolder=1; /*file type*/
		newInode.fileLength=0;

		newInodeIndex=findAnEmptyInode(); /*find a empty i-node to use*/


		Folder.inodePosition[i]=newInodeIndex;
		/*Refresh the folder*/
		tmp=writeFolderToHardisk(iNode.directBlockPosition[0],Folder);
		if(tmp<0)
		{
			perror("Write new folder error");
			return -1;
		}

		/*set the i-node's bitmap*/
		tmp=inodeCommand(newInodeIndex,SET);
		if(tmp<0)
		{
			perror("Set new inode bitmap error");
			return -1;
		}

		/*write i-node to block*/
		tmp=writeInodeToHardisk(newInodeIndex,newInode);
		if(tmp<0)
		{
			perror("Write new inode error");
			return -1;
		}

		openedFileNum++;
		for(i=0;i<10;i++)
		{
			if(openedFiles[i]==-1)
			{
				openedFiles[i]=newInodeIndex;
				return i;
			}
		}
	}
}

/* sequentially read from a file */
int my_read (int fileDescription, char * destination, int readLength)
{
	struct inode iNode;
	int lengthInBlock;
	int i,start;
	int j;
	int t;
	int readBlockIndex;
	char* buffer1;
	char* buffer2;
	struct bufferBlock buffBlock;
	lengthInBlock=(readLength+blockbuf[fileDescription])/BLOCK_SIZE+1;	/*how many blocks to be read*/
	buffer1=malloc(BLOCK_SIZE);
	buffer2=malloc(BLOCK_SIZE);

	if(openedFiles[fileDescription]==-1)	/*check if file is still open*/
	{
		printf("You can't write to a closed file\n");
		return -1;
	}
	getInodeByIndex(openedFiles[fileDescription],&iNode);	/*from file decription get the i-node, then assign it to iNode*/

	/*start to copy blocks to buffer*/
	start = blockbuf[fileDescription]/BLOCK_SIZE;
	for(i=start;i<lengthInBlock;i++)	/*traversal all amttempting to be read blocks*/
	{
		if(iNode.fileLength<=i)	/*just for sure not out of range*/
			return -1;
		else
		{
			if(i<4)	/*direct blocks*/
				readBlockIndex=iNode.directBlockPosition[i];	/*copy folder positions particularly*/
			else if(i>=4 && i<260) 	/*firstlevel blocks,for blocks from 4-260 */
				 {
					read_block(iNode.firstLevelIndirectBlock,buffer2);
                	memcpy(&buffBlock,buffer2,sizeof(struct bufferBlock));
					readBlockIndex=buffBlock.bufferInt[i-4];
				 }
				 else	/*second level blocks,from 260 on*/
				 {
					read_block(iNode.secondLevelIndirectBlock,buffer2);		/*get the second level block*/
					memcpy(&buffBlock,buffer2,sizeof(struct bufferBlock));
					j=(i-260)/256;											/*calculate the position of second level block*/
                	read_block(buffBlock.bufferInt[j],buffer2);				/*get next level's block*/
                	memcpy(&buffBlock,buffer2,sizeof(struct bufferBlock));
                	j=(i-260)%256;											/*calculate the position of the next level(level 3)*/
			    	readBlockIndex= buffBlock.bufferInt[j];					/*get the postion to read from*/
				 }

			/*copy. Handle with whole block copying or part of block copying*/
			read_block(readBlockIndex,(char*)buffer1);	/*read the specified block to buffer*/
			if(i==blockbuf[fileDescription]/BLOCK_SIZE && i==lengthInBlock-1)	/*very small file, just 1 block big(process not fully occupied block)*/
				memcpy((char*)(destination),(char*)(buffer1+blockbuf[fileDescription]%BLOCK_SIZE),readLength);
			else if(i==blockbuf[fileDescription]/BLOCK_SIZE)	/*first block(process not fully occupied block)*/
					memcpy((char*)(destination),(char*)(buffer1+blockbuf[fileDescription]%BLOCK_SIZE),BLOCK_SIZE-(blockbuf[fileDescription]%BLOCK_SIZE));
				 else if(i==lengthInBlock-1)	/*last block(may be the last block is not fully occupied)*/
						  memcpy((char*)(destination+i*BLOCK_SIZE-blockbuf[fileDescription]),(char*)(buffer1),(readLength+blockbuf[fileDescription])%BLOCK_SIZE);
					  else	/*other blocks, read from buffer1, read each block as a entity, no checking for not fully occupied block*/
						  memcpy((char*)(destination+i*BLOCK_SIZE-blockbuf[fileDescription]),(char*)(buffer1),BLOCK_SIZE);
		}

	}
	blockbuf[fileDescription]=blockbuf[fileDescription]+readLength;
	return readLength;
}

/* sequentially write to a file */
int my_write (int fileDescription, char * source, int writeLength)
{
	struct inode iNode;
	int lengthInBlock;
	int i,j,oldj,t,t2,writeBlockIndex,start;
	char* buffer1;
	char* buffer2;
	struct bufferBlock bufferBlock1;
	struct bufferBlock bufferBlock2;
	struct bufferBlock bufferBlock3;
	lengthInBlock=(writeLength+blockbuf[fileDescription])/BLOCK_SIZE+1;	/*file length in writeBlockIndex*/
	buffer1=malloc(BLOCK_SIZE);
	buffer2=malloc(BLOCK_SIZE);
    oldj=0;
	if(openedFiles[fileDescription]==-1)	/*if file closed*/
	{
		printf("You can't write to a closed file\n");
		return -1;
	}
	getInodeByIndex(openedFiles[fileDescription],&iNode);	/*get i-node*/

	start = blockbuf[fileDescription]/BLOCK_SIZE;

	/*traversal all blocks*/
	for(i=start;i<lengthInBlock;i++)
	{
		/*in case writing exceeded the original file's size*/
		if(iNode.fileLength<=i)
		{
			iNode.fileLength++;	/*increase file length*/
			if(i<4)	/*direct blocks*/
			{ 
				/*get a new empty block and assign it to file*/
				iNode.directBlockPosition[i]=findAnEmptyBlock();
				writeBlockIndex=iNode.directBlockPosition[i];
				blockCommand(iNode.directBlockPosition[i],SET);
			}	
			else if(i>=4 && i<260)	/*first level of indirect blocks*/
			{
				if(iNode.firstLevelIndirectBlock==0)	/*if first level indirect block was never used before, then allocate it*/
				{
					iNode.firstLevelIndirectBlock=findAnEmptyBlock();	/*get a new free block*/
					blockCommand(iNode.firstLevelIndirectBlock,SET);
					initBufferBlock(&bufferBlock1);
					memcpy(buffer2,&bufferBlock1,sizeof(struct bufferBlock));
                    write_block(iNode.firstLevelIndirectBlock,buffer2);	/*write an empty data to first level indirect block*/
				}
				if(i==4)	/*first block of indirect blocks*/
				{
					read_block(iNode.firstLevelIndirectBlock,buffer2);
					writeBlockIndex=findAnEmptyBlock();
					blockCommand(writeBlockIndex,SET);
               		memcpy(&bufferBlock1,buffer2,sizeof(struct bufferBlock));
				}
				bufferBlock1.bufferInt[i-4]=writeBlockIndex;
				memcpy(buffer2,&bufferBlock1,sizeof(struct bufferBlock));
				write_block(iNode.firstLevelIndirectBlock,buffer2);
			}
			else	/*second level of indirect blocks*/
			{
				if(iNode.secondLevelIndirectBlock==0)	/*if second level indirect block was never used before, then allocate it*/
				{
					iNode.secondLevelIndirectBlock=findAnEmptyBlock();
					blockCommand(iNode.secondLevelIndirectBlock,SET);
					initBufferBlock(&bufferBlock1);
                    memcpy(buffer2,&bufferBlock1,sizeof(struct bufferBlock));
                    write_block(iNode.secondLevelIndirectBlock,buffer2);
				}
				if(i==260) /*first block of second level indirect blocks*/
				{
					read_block(iNode.secondLevelIndirectBlock,buffer2);
					memcpy(&bufferBlock1,buffer2,sizeof(struct bufferBlock));
				}
				j=(i-260)/256;
				if(bufferBlock1.bufferInt[j]==0)
				{
				    bufferBlock1.bufferInt[j]=findAnEmptyBlock();
					blockCommand(bufferBlock1.bufferInt[j],SET);
					initBufferBlock(&bufferBlock2);
                    memcpy(buffer2,&bufferBlock1,sizeof(struct bufferBlock));
                    write_block(iNode.secondLevelIndirectBlock,buffer2);
					memcpy(buffer2,&bufferBlock2,sizeof(struct bufferBlock));
                    write_block(bufferBlock1.bufferInt[j],buffer2);
				}
				if(j>oldj)
				{
                	read_block(bufferBlock1.bufferInt[j],buffer2);
                	memcpy(&bufferBlock2,buffer2,sizeof(struct bufferBlock));
				}
               t=bufferBlock1.bufferInt[j];
			   t2=(i-260)%256;
			   bufferBlock2.bufferInt[t2]=findAnEmptyBlock();
			   blockCommand(bufferBlock2.bufferInt[t2],SET);
			   writeBlockIndex= bufferBlock2.bufferInt[t2];
			   memcpy(buffer2,&bufferBlock2,sizeof(struct bufferBlock));
               write_block(t,buffer2);
			   oldj=j;
			}
			oldj=0;
		}
		else	/*if what to write in does not exceed the file's size, just handle in the way that my_read() does, but reversed*/
		{
			if(i<4)	/*direct blocks*/
			{ 
				writeBlockIndex=iNode.directBlockPosition[i];
			}	
			else if(i>=4 && i<260)	/*first level indirect blocks*/
			{
				if(i==4)
				{
				read_block(iNode.firstLevelIndirectBlock,buffer2);
                memcpy(&bufferBlock1,buffer2,sizeof(struct bufferBlock));
				}
				writeBlockIndex=bufferBlock1.bufferInt[i-4];
			}
			else	/*second level indirect blocks*/
			{
				if(i==260)
				{
				read_block(iNode.secondLevelIndirectBlock,buffer2);
				memcpy(&bufferBlock1,buffer2,sizeof(struct bufferBlock));
				}
				j=(i-260)/256;
                read_block(bufferBlock1.bufferInt[j],buffer2);
                memcpy(&bufferBlock2,buffer2,sizeof(struct bufferBlock));
				t=(i-260)%256;
			    writeBlockIndex= bufferBlock2.bufferInt[t];
				
			}
			
		}
		/*copy. Handle with whole block copying or part of block copying*/
		read_block(writeBlockIndex,(char*)buffer1);
			if(i==blockbuf[fileDescription]/BLOCK_SIZE && i==lengthInBlock-1)
				memcpy((char*)(buffer1+blockbuf[fileDescription]%BLOCK_SIZE),(char*)(source),writeLength);
			else if(i==blockbuf[fileDescription]/BLOCK_SIZE)
				memcpy((char*)(buffer1+blockbuf[fileDescription]%BLOCK_SIZE),(char*)(source),BLOCK_SIZE-(blockbuf[fileDescription]%BLOCK_SIZE));
			else if(i==lengthInBlock-1)
				memcpy((char*)(buffer1),(char*)(source+i*BLOCK_SIZE-blockbuf[fileDescription]),(writeLength+blockbuf[fileDescription])%BLOCK_SIZE);
			else
				memcpy((char*)(buffer1),(char*)(source+i*BLOCK_SIZE-blockbuf[fileDescription]),BLOCK_SIZE);
			write_block(writeBlockIndex,(char*)buffer1);
		/*write the whole file back to harddisk*/
		writeInodeToHardisk(openedFiles[fileDescription],iNode);

	}
	blockbuf[fileDescription]=blockbuf[fileDescription]+writeLength;
	return writeLength;
}

/*close a file*/
int my_close (int fd)
{
	openedFileNum--;
	openedFiles[fd]=-1;		/*set file description to -1, this file is no longer in service*/
	blockbuf[fd]=0;

	return 0;	
}

/*remove */
int my_remove (char * path)
{
	int r;
	int i;
	char* last_name;
	struct inode id;
	struct folder cl;
    int freenode;
	char * argp[MAX_PATH_DEPTH];
	for(i=0;i<MAX_PATH_DEPTH;i++)
		argp[i]=malloc(MAX_FILENAME);

	/*parse path and get information*/
	r= resolveFilePath(path,argp);
	
	last_name=malloc(MAX_FILENAME);
	strcpy(last_name,argp[r]);

	/*get i-node number*/
	r= getInodeByFilePath(r,argp);
	if(r<0)
	{   
		printf("Can't find the path");
		return -1;
	}
	/*get i-node by its index*/
	getInodeByIndex(r,&id);
	
    if(id.fileOrFolder==1)
	{
		printf("The specified file is not a folder");
		return -1;
	}
	else
	{
		getFolderByBlockIndex(id.directBlockPosition[0],&cl);	/*get folder inside the i-node*/
		for(i=0;i<10;i++)
		{
			if(strcmp(cl.name[i],last_name)==0)	/*check if the i-node which need to be removed really exist*/
			{
				break;
			}
			if(i==9)return -1;

		}
			

		cl.name[i][0]='\0';		/*set removed folder name to NULL*/
		freenode=cl.inodePosition[i];
        cl.inodePosition[i]=0;		/*Set removed folder i-node position to 0*/
		r=writeFolderToHardisk(id.directBlockPosition[0],cl);	/*write back the lastest data*/
		if(r<0)
		{
			perror("Put new folder error");
			return -1;
		}

		freeInodeAndBlock(freenode);	/*actually free the i-node*/

       return 0;
	}

}

int my_rename (char * oldpath, char * newpath)
{
	int r;
	int i;
	struct inode oldnode;
	struct inode newnode;
	int target;
	struct folder cl;
	char * argp[MAX_PATH_DEPTH];
	char * last_name;


	for(i=0;i<MAX_PATH_DEPTH;i++)
		argp[i]=malloc(MAX_FILENAME);

	last_name=malloc(MAX_FILENAME);
	/*r represents depth here*/
	r= resolveFilePath(oldpath,argp);
	strcpy(last_name,argp[r]);
	/*r represents inode num here*/
	r= getInodeByFilePath(r,argp);
	if(r<0)
	{   
		printf("Can't find the path");

		return -1;
	}
	/*get i-node by its index*/
	getInodeByIndex(r,&oldnode);
	if(oldnode.fileOrFolder==1)
	{
		printf("The file of the inode is not a folder");
		return -1;
	}
	else
	{
		/*get folder of this i-node*/
		getFolderByBlockIndex(oldnode.directBlockPosition[0],&cl);
		for(i=0;i<10;i++)
		{

			if(strcmp(cl.name[i],last_name)==0)		/*check for the valid oldpath*/
			{
				break;
			}
			if(i==9)return -1;

		}
			
		cl.name[i][0]='\0';

		target=cl.inodePosition[i];
		
		cl.inodePosition[i]=0;
		/*Refresh the folder*/
		r=writeFolderToHardisk(oldnode.directBlockPosition[0],cl);
		if(r<0)
		{
			perror("Put new folder error");
			return -1;
		}

		r= resolveFilePath(newpath,argp);
		strcpy(last_name,argp[r]);
		/*r represents inode num here*/
		r= getInodeByFilePath(r,argp);
		if(r<0)
		{   
			printf("Can't find the path");
			return -1;
		}

		/*get i-node by index*/
		getInodeByIndex(r,&newnode);

		if(newnode.fileOrFolder==1)
		{
			printf("The file of the inode is not a folder");
			return -1;
		}
		else
		{
			/*get folder of this i-node*/
			getFolderByBlockIndex(newnode.directBlockPosition[0],&cl);
			for(i=0;i<10;i++)
			{
                if(strlen(cl.name[i])==0)break;
				if(strcmp(cl.name[i],last_name)==0)		/*if the new name attempting to rename is already used*/
				{
					printf("There is a file with the same name");
					return -1;
				}
				if(i==9)return -1;

			}
			
			strcpy(cl.name[i],last_name);
			cl.inodePosition[i]=target;

			/*Refresh the folder*/
			r=writeFolderToHardisk(newnode.directBlockPosition[0],cl);
			if(r<0)
			{
				perror("Put new folder error");
				return -1;
			}

			return 0;
		}

	}
}

/* only works if all but the last component of the path already exists */
int my_mkdir (char * path)
{

	int r;
	int i;
	char* last_name;
	struct inode id;
	struct folder cl;
	struct inode newinode;
	struct folder newcatalogue;
	int     newinodeindex;

	char * argp[MAX_PATH_DEPTH];
	for(i=0;i<MAX_PATH_DEPTH;i++)
		argp[i]=malloc(MAX_FILENAME);

	/*parse path and get information*/
	r= resolveFilePath(path,argp);

	last_name=malloc(MAX_FILENAME);
	strcpy(last_name,argp[r]);


	/*get i-node number*/
	r= getInodeByFilePath(r,argp);

	/*Can't find */
	if(r<0)
	{   
		printf("Can't find the path");
		return -1;
	}
	/*get i-node by its index*/
	getInodeByIndex(r,&id);


	if(id.fileOrFolder==1)
	{
		printf("The file of the inode is not a folder");
		return -1;
	}
	else
	{


		getFolderByBlockIndex(id.directBlockPosition[0],&cl);
		for(i=0;i<10;i++)
		{
			if(strlen(cl.name[i])==0)break;
			if(strcmp(cl.name[i],last_name)==0)	/*if the new directory attempting to create is already in used*/
			{
				printf("there is a file having the same name as %s",last_name);	
				return -1;
			}

		}
		strcpy(cl.name[i],last_name);

		initInode(&newinode);
		newinode.fileOrFolder=0;
		newinode.fileLength=1;
		newinode.directBlockPosition[0]=findAnEmptyBlock();	/*different from my_create. An empty block position is assign to the catalog's position of the new i-node*/

		/*Find where to put the inode*/
		newinodeindex=findAnEmptyInode();


		cl.inodePosition[i]=newinodeindex;
		/*Refresh the folder*/
		r=writeFolderToHardisk(id.directBlockPosition[0],cl);
		if(r<0)
		{
			perror("Put new folder error");
			return -1;
		}

		/*set the map*/
		r=inodeCommand(newinodeindex,SET);
		if(r<0)
		{
			perror("Set new inode map error");
			return -1;
		}


		/*put inode*/
		r=writeInodeToHardisk(newinodeindex,newinode);
		if(r<0)
		{
			perror("Put new inode error");
			return -1;
		}

		/*put the new folder*/
		r=blockCommand(newinode.directBlockPosition[0],SET);
		if(r<0)
		{
			perror("Set new inode map error");
			return -1;
		}

		initFolder(&newcatalogue);
		r=writeFolderToHardisk(newinode.directBlockPosition[0],newcatalogue);
		if(r<0)
		{
			perror("Put new folder Error");
			return -1;
		}

		return 0;

	}
	return 0;
}

int my_rmdir (char * path)
{	

	int r;
	int i;
	char* last_name;
	struct inode id;
	struct folder cl;
	struct inode getinode;
	struct folder getcatalogue;

	char * argp[MAX_PATH_DEPTH];
	for(i=0;i<MAX_PATH_DEPTH;i++)
		argp[i]=malloc(MAX_FILENAME);

	/*parse path and get information*/
	r= resolveFilePath(path,argp);

	last_name=malloc(MAX_FILENAME);
	strcpy(last_name,argp[r]);

	/*get i-node number*/
	r= getInodeByFilePath(r,argp);
	/*Can't find */
	if(r<0)
	{   
		printf("Can't find the path");
		return -1;
	}
	/*get i-node by its index*/
	getInodeByIndex(r,&id);

	if(id.fileOrFolder==1)
	{
		printf("The file of the inode is not a folder");
		return -1;
	}
	else
	{

		getFolderByBlockIndex(id.directBlockPosition[0],&cl);
		for(i=0;i<10;i++)
		{
			if(strcmp(cl.name[i],last_name)==0)	/*directory which attempting to be removed exists */
			{
				break;
			}
			if(i==9)
			{
				printf("Can't find the dir");	/*attempting to remove unexisted directory*/
				return -1;
			}
		}
	
		freeInodeAndBlock(cl.inodePosition[i]);


		cl.name[i][0]='\0';
		cl.inodePosition[i]=0;


		r=writeFolderToHardisk(id.directBlockPosition[0],cl);
		if(r<0)
		{
			perror("Put new folder error");
			return -1;
		}

		return 0;

	}
}

/* check to see if the device already has a file system on it,
* and if not, create one. */
void my_mkfs ()
{
	int i,r;
	char clean[1024];
	char temp[1024]; 
	struct folder firstCatalogue;
	struct inode firstNode;
	struct inode readNode;

	openedFileNum=0;
	for(i=0;i<10;i++)
	{
		openedFiles[i]=-1;
		blockbuf[i]=0;
	}

	block_size=dev_open ();	/*open device*/

	r=write_block (INODE_MAP,clean);	/*init i-node bitmap*/
	if(r<0)perror("Clean the inode map Error");

	for(i=BLOCK_MAP_START;i<=BLOCK_MAP_END;i++)
	{
		r=write_block (i,clean);	/**/
		if(r<0)perror("Clean the inode map Error");
	}

	firstNode.fileOrFolder=0;
	firstNode.fileLength=1;
	firstNode.directBlockPosition[0]=BLOCK_START;
	memcpy(temp,&firstNode,sizeof(struct inode));   

	r=write_block (INODE_START,temp);
	if(r<0)perror("Write First Node Error");

	r=inodeCommand(0,SET);
	if(r<0)perror("Set First Node In Map Error");

	initFolder(&firstCatalogue);
	memcpy(temp,&firstCatalogue,sizeof(struct folder));
	r=write_block (BLOCK_START,temp);
	if(r<0)perror("Write First Block Error");

	r=blockCommand(BLOCK_START,SET);
	if(r<0)perror("Set First Block In Map Error");

	highwatermarkforInode=0;
	highwatermarkforBlock=200;

	getInodeByIndex(0,&readNode);

}

/*return the i-node coressponds to the given path(argp) */
/*This function traversals along the argp, return the i-node of the specified path (if exist).*/
int getInodeByFilePath(int depth,char **argp)
{

	int result;
	int newresult;
	int i,j;
	struct inode id;
	struct folder cl;

	result=0;
	newresult=0;
	if(depth==1)return result;

	j=1;
	while(depth!=1)
	{
		/*initially, result equals 0, so it gets the first i-node (root)*/
		getInodeByIndex(result,&id);
		if(id.fileOrFolder==1)
		{
			printf("%s is not a folder",argp[j-1]);	/*assign i-node's index to 'id'*/
			return -1;
		}
		else
		{
			getFolderByBlockIndex(id.directBlockPosition[0],&cl);			/*read the block number id.direct_blk[0], assign to folder cl*/

			for(i=0;i<10;i++)						/*traversals all names in the current folder*/
			{
				if(strcmp(cl.name[i],argp[j])==0)	/*found a match in path name*/
				{
					newresult=cl.inodePosition[i];		/*jump to the matched path*/
					break;
				}
			}
			if(newresult==result)					/*cannot find a matched path*/
			{
				printf("Can't find %s\n",argp[j]);
				return -1;
			}
			else
			{
				result=newresult;
			}
		}
		j++;			/*next level in the target path name (argp)*/
		depth--;		/*current depth decreased*/
	}

	return result;
}


/*write a specified i-node to newInode*/
int writeInodeToHardisk(int index,struct inode newInode)
{
	int r;
	char mybuffer[1024];

	r=read_block(INODE_START+index/INODES_IN_BLOCK,mybuffer);
	if(r<0)
	{
		perror("Read Node Error");
		return -1;
	}
	memcpy(mybuffer+(index%INODES_IN_BLOCK)*32,&newInode,sizeof(struct inode));

	r=write_block(INODE_START+index/INODES_IN_BLOCK,mybuffer);
	if(r<0)
	{
		perror("Write Node Error");
		return -1;
	}
	return 0;

}



/*write a folder to a specified position(block)
  */
int writeFolderToHardisk(int index,struct folder Folder)
{
	int r;
	char buffer[1024];

	memcpy(buffer,&Folder,sizeof(struct folder));	/*copy folder to temporary buffer*/


	r=write_block(index,buffer);	/*write to specified block*/
	if(r<0)
	{
		perror("Write Folder Error");
		return -1;
	}

	return 0;

}

/*assign specified i-node to node
  * return 0 if succeeded, otherwise return -1.
  */
int getInodeByIndex(int index,struct inode* iNode)
{
	int r;
	char buffer[1024];

	r=read_block(INODE_START+index/INODES_IN_BLOCK,buffer);
	if(r<0)
	{
		perror("Read Node Error");
		return -1;
	}
	memcpy(iNode,(struct inode*)(buffer+(index%INODES_IN_BLOCK)*32),sizeof(struct inode));

	return 0;
}



/*assign the specified folder to cata.
 * return 0 if succeeded, otherwise return -1.
 */
int getFolderByBlockIndex(int index,struct folder* Folder)
{
	int r;
	char buffer[1024];

	r=read_block(index,buffer);
	if(r<0)
	{
		perror("Error in block reading!");
		return -1;
	}
	memcpy(Folder,buffer,sizeof(struct folder));	/*assign obtained folder to Folder*/
	return 0;

}



/*free specified i-node and its block*/
int freeInodeAndBlock(int index)
{
	struct inode freenode;
	struct folder freecatalogue;
    struct bufferBlock inter;
	struct bufferBlock inter2;
	int i;
	int j;
	int t;
	int r;
	char* buffer1;
	char* buffer2;
	int block;

	getInodeByIndex(index,&freenode);

	if(freenode.firstLevelIndirectBlock!=0)	/*if is a folder*/
	{
		buffer1=malloc(BLOCK_SIZE);
		r=read_block(freenode.firstLevelIndirectBlock,buffer1);
		if(r<0)
		{
			perror("Error in block reading");
			return -1;
		}
		blockCommand(freenode.firstLevelIndirectBlock,RESET);
	}
	/*It is a normal file*/
	if(freenode.fileOrFolder==1)
	{
		for(i=0;i<freenode.fileLength;i++)
		{
			if(i<4)blockCommand(freenode.directBlockPosition[i],RESET);
			else if(i<260)
			{
				if(i==4)
				{
		        r=read_block(freenode.firstLevelIndirectBlock,buffer1);
				memcpy(&inter,buffer1,sizeof(struct bufferBlock));
				}
		        blockCommand(inter.bufferInt[i-4],RESET);
			}
			else
			{
				if(i==260)
				{
		        r=read_block(freenode.secondLevelIndirectBlock,buffer1);
				memcpy(&inter,buffer1,sizeof(struct bufferBlock));
				}
				j=(i-260)/256;
                r=read_block(inter.bufferInt[j],buffer1);
				memcpy(&inter2,buffer1,sizeof(struct bufferBlock));
				for(t=0;t<256;t++)
				{
				 if(inter2.bufferInt[t]==0)break;
                 blockCommand(inter2.bufferInt[t],RESET);
				}
				i=i+t;
			}
 
		}
		inodeCommand(index,RESET);
		return 0;
	}
	/*this is dir file*/
	else
	{
		getFolderByBlockIndex(freenode.directBlockPosition[0],&freecatalogue);

		for(i=0;i<10;i++)
		{
			if(strlen(freecatalogue.name[i])!=0)

			{  
				freeInodeAndBlock(freecatalogue.inodePosition[i]);
			}
		}
		blockCommand(freenode.directBlockPosition[0],RESET);
		inodeCommand(index,RESET);
		return 0;
	}
}


/*apply set/reset on an I-node*/
int inodeCommand(int Position,int Cmd)
{
	int index1;/*the position of the target Byte*/
	int index2;/*the position in the target Byte*/
	int r;/*the result of the read of block ,only used for debug*/
	char mybuffer[BLOCK_SIZE];/*the buffer is used to store the map*/
	char code;/*used for SET and RESET*/


	/*only 5000 files can be stored in the file system*/
	if(Position>=INODE_NUM || Position <0 )return -1;


	if(Cmd==SET)
		code=SET_CODE;
	else
		code=RESET_CODE;

	r=read_block(INODE_MAP,mybuffer);
	if(r<0)
	{
		perror("Read Block_Map Error");
		return -1;
	}

	index1=Position/8;
	index2=Position%8;

	code =code>>index2;

	if(Cmd==SET)
		*(mybuffer+index1)=*(mybuffer+index1) | code;
	else
		*(mybuffer+index1)=*(mybuffer+index1) & code;

	r=write_block(INODE_MAP,mybuffer);
	if(r<0)
	{
		perror("Write Block_Map Error");
		return -1;
	}



	return 0;
}

/*	From the i-node bitmap find a free i-node.
 * 	return the free i-node position.
 * 	Update the bitmap and write back.
 */
int findAnEmptyInode()
{
	int index1;					/*the position of the target Byte*/
	int index2;					/*the position in the target Byte*/
	int r;						/*the result of the read of block, for debug*/
	char mybuffer[BLOCK_SIZE];	/*the buffer is used to store the map*/
	char code;					/*used for check bit*/
	int i,j;

	/*Find from the watermark */
	index1=highwatermarkforInode/8;
	index2=highwatermarkforInode%8;


	r=read_block(INODE_MAP,mybuffer);	/*read i-node's bitmap (at 1st block)*/

	if(r<0)
	{
		perror("Read Block_Map Error");
		return -1;
	}

	for(i=index1;i<INODE_NUM/8;i++)		/*divide the whole bitmap into 8-node groups. Compare each 8-node group */
	{
		for(j=index2;j<8;j++)	/*inside a 8-node group*/
		{
			code=0x80>>j;
			if((code & *(mybuffer+i))==0)	/*compare with bitmap. If matched*/
			{
				highwatermarkforInode=i*8+j+1;
				return i*8+j;	/*return the free i-node position*/
			}
		}
	}

	/*Find from the beginning*/
	for(i=0;i<INODE_NUM/8;i++)
	{
		for(j=0;j<8;j++)	/*from beginning*/
		{
			code=0x80>>j;
			if((code & *(mybuffer+i))==0)	/*found a free i-node*/
			{
				highwatermarkforInode=i*8+j+1;
				return i*8+j;	/*return the free i-node position*/
			}
			/*Can't find a free i-node*/
			if(highwatermarkforInode==i*8+j)
			{
				return -1;
			}
		}
	}
	/*write back the new i-node bitmap*/
	r=write_block(INODE_MAP,mybuffer);

	if(r<0)
	{
		perror("Write Block_Map Error");
		return -1;
	}


}

/*apply set/reset command(Cmd) to a block*/
int blockCommand(int Position,int Cmd)
{
	int block_num;
	int index;
	int r;
	char mybuffer[1024];
	char code;

	if(Cmd==SET)
		code=SET_CODE;
	else
		code=RESET_CODE;

	block_num=Position/BLOCK_SIZE_BIT;


	r=read_block(BLOCK_MAP_START+block_num,mybuffer);
	if(r<0)
	{
		perror("Read Block_Map Error");
		return -1;
	}
	block_num=Position-block_num*BLOCK_SIZE_BIT;
	index=block_num%8;
	code =code>>index;

	block_num=block_num/8;


	if(Cmd==SET)
		*(mybuffer+block_num)=*(mybuffer+block_num) | code;	/*set, set all bits to 1*/
	else
		*(mybuffer+block_num)=*(mybuffer+block_num) & code;	/*reset, clear all bits to zero*/

	block_num = Position/BLOCK_SIZE_BIT;
	r = write_block(BLOCK_MAP_START+block_num,mybuffer);	/*write back*/
	if(r<0)
	{
		perror("Write Block_Map Error");
		return -1;
	}
	return 0;

}

/*	From the block bitmap find a free block
 * 	return the free block position.
 * 	Update the bitmap and write back.
 *	Implementation is similar with findAnEmptyInode.
 */
int findAnEmptyBlock()
{
	int block_num;
	int index;
	int r;
	int i,j;
	char mybuffer[1024];
	char code;

	code = 0x80;
	block_num = highwatermarkforBlock / BLOCK_SIZE_BIT;
	index = highwatermarkforBlock % BLOCK_SIZE_BIT;

	r=read_block(BLOCK_MAP_START+block_num,mybuffer);	/*read block's bitmap (at 1st block)*/
	
	if(r<0)
	{
		perror("Read Block_Map Error");
		return -1;
	}

	for(i=index/8;i<BLOCK_SIZE;i++)						/*divide the whole bitmap into 8-block groups. Compare each group */
	{
		for(j=index%8;j<8;j++)
		{
			code=0x80>>j;
			if((code & *(mybuffer+i))==0)			/*compare with bitmap. If matched*/
			{
				highwatermarkforBlock=block_num*BLOCK_SIZE_BIT+i*8+j+1;
				return block_num*BLOCK_SIZE_BIT+i*8+j;
			}
		}
	}
	
	/*Traversal from the beginning of block segment */
	while(block_num<=BLOCK_MAP_END-BLOCK_MAP_START-2)	
	{
		block_num++;
		/*printf("read block map %d\n",BLOCK_MAP_START+block_num);*/
		r=read_block(BLOCK_MAP_START+block_num,mybuffer);
		if(r<0)
		{
			perror("Read Block_Map Error");
			return -1;
		}

		for(i=0;i<BLOCK_SIZE;i++)	/*traversal from segment's beginning*/
		{
			for(j=0;j<8;j++)
			{
				code=0x80>>j;
				if((code & *(mybuffer+i))==0)
				{
					highwatermarkforBlock=block_num*BLOCK_SIZE_BIT+i*8+j+1;
					return block_num*BLOCK_SIZE_BIT+i*8+j;
				}
			}
		}

	}
	
	/*Afterward, for sure, traversal from the very beginning*/
    block_num=-1;
	while(block_num<=BLOCK_MAP_END-BLOCK_MAP_START-2)
	{
		block_num++;
		r=read_block(BLOCK_MAP_START+block_num,mybuffer);
		if(r<0)
		{
			perror("Read Block_Map Error");
			return -1;
		}

		for(i=0;i<BLOCK_SIZE;i++)
		{
			for(j=0;j<8;j++)
			{
				code=0x80>>j;
				if(block_num*BLOCK_SIZE_BIT+i*8+j+1==highwatermarkforBlock)return -1;
				if((code & *(mybuffer+i))==0)
				{
					highwatermarkforBlock=block_num*BLOCK_SIZE_BIT+i*8+j+1;
					return block_num*BLOCK_SIZE_BIT+i*8+j;
				}
			}
		}

	}
}
