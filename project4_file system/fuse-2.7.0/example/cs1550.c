/*
	FUSE: Filesystem in Userspace
	Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

	This program can be distributed under the terms of the GNU GPL.
	See the file COPYING.
*/

#define	FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

//size of a disk block
#define	BLOCK_SIZE 512

//we'll use 8.3 filenames
#define	MAX_FILENAME 8
#define	MAX_EXTENSION 3

//How many files can there be in one directory?
#define MAX_FILES_IN_DIR (BLOCK_SIZE - sizeof(int)) / ((MAX_FILENAME + 1) + (MAX_EXTENSION + 1) + sizeof(size_t) + sizeof(long))

//The attribute packed means to not align these things
struct cs1550_directory_entry
{
	int nFiles;	//How many files are in this directory.
				//Needs to be less than MAX_FILES_IN_DIR

	struct cs1550_file_directory
	{
		char fname[MAX_FILENAME + 1];	//filename (plus space for nul)
		char fext[MAX_EXTENSION + 1];	//extension (plus space for nul)
		size_t fsize;					//file size
		long nStartBlock;				//where the first block is on disk
	} __attribute__((packed)) files[MAX_FILES_IN_DIR];	//There is an array of these

	//This is some space to get this to be exactly the size of the disk block.
	//Don't use it for anything.  
	char padding[BLOCK_SIZE - MAX_FILES_IN_DIR * sizeof(struct cs1550_file_directory) - sizeof(int)];
} ;

typedef struct cs1550_root_directory cs1550_root_directory;

#define MAX_DIRS_IN_ROOT (BLOCK_SIZE - sizeof(int)) / ((MAX_FILENAME + 1) + sizeof(long))

struct cs1550_root_directory
{
	int nDirectories;	//How many subdirectories are in the root
						//Needs to be less than MAX_DIRS_IN_ROOT
	struct cs1550_directory
	{
		char dname[MAX_FILENAME + 1];	//directory name (plus space for nul)
		long nStartBlock;				//where the directory block is on disk
	} __attribute__((packed)) directories[MAX_DIRS_IN_ROOT];	//There is an array of these

	//This is some space to get this to be exactly the size of the disk block.
	//Don't use it for anything.  
	char padding[BLOCK_SIZE - MAX_DIRS_IN_ROOT * sizeof(struct cs1550_directory) - sizeof(int)];
};


typedef struct cs1550_directory_entry cs1550_directory_entry;

//How much data can one block hold?
#define	MAX_DATA_IN_BLOCK (BLOCK_SIZE)

struct cs1550_disk_block
{
	//All of the space in the block can be used for actual data
	//storage.
	char data[MAX_DATA_IN_BLOCK];
};

typedef struct cs1550_disk_block cs1550_disk_block;

cs1550_root_directory get_root()
{
	FILE *f=fopen(".disk","r");
	cs1550_root_directory root;
	fseek(f, 0,SEEK_SET);
	fread(&root,BLOCK_SIZE,1,f);
	fclose(f);
	//printf("getroot->dir[i]:%s nDir:%d\n",root.directories[0].dname,root.nDirectories);
	return root;
}

cs1550_directory_entry get_block_at(long n)
{
	FILE *f=fopen(".disk","rb");
	cs1550_directory_entry e;
	fseek(f, n*BLOCK_SIZE,SEEK_SET);
	fread(&e,BLOCK_SIZE,1,f);
	
	printf("get_block->nFiles:%d blk_num:%d\n",e.nFiles,n);
	
	fclose(f);
	return e;
}

cs1550_disk_block get_file_block_at(long n)
{
	FILE *f=fopen(".disk","rb");
	cs1550_disk_block e;
	fseek(f, n*BLOCK_SIZE,SEEK_SET);
	fread(&e,BLOCK_SIZE,1,f);
	
	printf("get_file_block_at:%d\n",n);	
	fclose(f);
	return e;
}

int
//the total num of blocks is 10000, bipmap needs 10000 bits to map the entire file system which can be allocated in 3 blocks
#define NUM_BLOCKS 10000
#define BITMAP_SIZE 3*BLOCK_SIZE

//find and return a location where the data of a block can be contiguously allocated
//and set that block to 1 in bitmap
long write_bmap(long size)
{
	FILE *f=fopen(".disk","r+");
	fseek(f, -BITMAP_SIZE,SEEK_END);
	char bmap[BITMAP_SIZE];
	fread(&bmap,BITMAP_SIZE,1,f);	
	print_map(bmap);
	
	bmap[0]=(bmap[0])|1;//the first bit, the root directory block is always occupied
	long i=0;
	while(i<=BITMAP_SIZE-3)
	{
		i++;
		//locate an empty block
		if (get_status(bmap,i)==0)
			break;	
	}
	printf("free block number: %d\n",i);
	
	//if reaches the end without empty blocks, return -1
	if (i==BITMAP_SIZE-2)
		return -1;

	set_status(bmap,1,i);
	//printf("set_status completed\n");
	print_map(bmap);
	fseek(f, -BITMAP_SIZE,SEEK_END);
	fwrite(bmap,sizeof(bmap),1,f);
	fclose(f);
	return i;
}

void get_bmap(char* bmap)
{
	FILE *f=fopen(".disk","r+");
	fseek(f, -BITMAP_SIZE,SEEK_END);
	fread(bmap,BITMAP_SIZE,1,f);	
}

void print_disk()
{
	FILE *f=fopen(".disk","rb");
	char buf[BLOCK_SIZE*NUM_BLOCKS];	
	fread(buf,sizeof(buf),1,f);
	int c=0;
	while (c<sizeof(buf))
		printf("%d",buf[c++]);
	fclose(f);

}

void print_map(char* bmap)
{
	int cnt=0;
	while (cnt<BITMAP_SIZE)
	{
		int ind=7;
		while(ind>=0)
			printf("%d",(bmap[cnt]>>ind--)&1);	
		cnt++;
	}
	printf("\n");	

}
void write_block(long n,void* buf)
{
	
	FILE *f=fopen(".disk","r+");
	printf("write_block blk_num: %d\n",n);
	fseek(f, n*BLOCK_SIZE,SEEK_SET);
	fwrite(buf,BLOCK_SIZE,1,f);
	fclose(f);

}

//get the space status of a specific block from bitmap
int get_status(char* bitmap,long blk_num)
{
	//attempt to access the bitmap
	if(blk_num < 0 ||blk_num>NUM_BLOCKS-3)
		return -1;	
	
	long byte_num=blk_num/8;
	int bit_num=blk_num%8;		
	return (bitmap[byte_num]>>bit_num) & 1;		

}
//set the space status of a specific block from bitmap
int set_status(char* bitmap,int val,long blk_num)
{
	//attempt to access the bitmap
	if(blk_num < 0 ||blk_num>NUM_BLOCKS-3)
		return -1;	
	
	long byte_num=blk_num/8;
	int bit_num=blk_num%8;
	//printf("byte_num:%d bit shift:%d \n",byte_num,bit_num);
	bitmap[byte_num]=bitmap[byte_num] | val << bit_num;		
	return 0;
}


//path parameters
static char directory[MAX_FILENAME+1];
static char filename[MAX_FILENAME+1];
static char extension[MAX_EXTENSION+1];
//index num of dir if found
static int dir_index;
//block num of dir if found
static int dir_num;
//block num of file if found
static int file_num;
//index of file if found
static int file_index;

/*
 * Called whenever the system wants to know the file attributes, including
 * simply whether the file exists or not. 
 *
 * man -s 2 stat will show the fields of a stat structure
 */

//used to check the status of the path
static int get_attr_res=0;

static int cs1550_getattr(const char *path, struct stat *stbuf)
{

	memset(stbuf, 0, sizeof(struct stat));
	printf("getattr ->path: %s\n",path);
	strcpy(directory,"");
	strcpy(filename,"");
	strcpy(extension,"");

	sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);		
//	printf("dir_len:%d f_len:%d ext_len:%d\n",strlen(directory),strlen(filename),strlen(extension));
	printf("dir:%s fname:%s ext:%s\n",directory,filename,extension);
	if (strlen(directory)>MAX_FILENAME || strlen(filename)>MAX_FILENAME || strlen(extension)>MAX_EXTENSION)
	{
		get_attr_res=-ENAMETOOLONG;
		return -ENAMETOOLONG;
	}

	//is path the root dir?
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		get_attr_res=0;
		return 0;
	}
	
	//Check if name is subdirectory
	cs1550_root_directory root=get_root();
	
	int i =0;
	int dir_exists=0;
	long start_b=0;
	while (i < root.nDirectories)
	{
		printf("getattr->dir[i]:%s nDir:%d\n",root.directories[i].dname,root.nDirectories);
		
		if(strcmp(root.directories[i].dname,directory)==0)
		{
			dir_index=i;	
			dir_num=root.directories[i].nStartBlock;
			//Might want to return a structure with these fields
			dir_exists=1;
			start_b=root.directories[i].nStartBlock;
			break;
		}
		i++;
	}
	//dir doesn't exist
	if (dir_exists==0)
	{
		get_attr_res=-ENOENT;
		return -ENOENT;
	}

	printf("check if filename is empty(0 on empty): %d\n",strcmp(filename,""));	
	//dir exists and no filename presented
	if (strcmp(filename,"")==0)
	{
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;		
		get_attr_res=0;
		return 0; //no error
	}
 
	//there is file to check		
 	cs1550_directory_entry sub_dir=get_block_at(start_b);
	i=0;
	int f_exists=0;
	int fsize=0;
	while (i<sub_dir.nFiles)
	{
		struct cs1550_file_directory curr_f=sub_dir.files[i];
	
		printf("get_attr->currf.fname:%s fext:%s filename:%s  extension:%s\n",curr_f.fname,curr_f.fext,filename,extension);
		if (strcmp(filename,curr_f.fname)==0 && strcmp(extension,curr_f.fext)==0)
		{
			printf("file matched\n");
			file_num=curr_f.nStartBlock;
			fsize=curr_f.fsize;
			f_exists=1;
			file_index=i;
			break;	
		}
		i++;
	}
	//file not found
	if (f_exists==0)
	{
		get_attr_res=-ENOENT;	
		return -ENOENT;
	}
	//otherwise file is found
	//regular file, probably want to be read and write
	stbuf->st_mode = S_IFREG | 0666; 
	stbuf->st_nlink = 1; //file links
	stbuf->st_size = fsize; //file size - make sure you replace with real size!
	get_attr_res=0;
	return 0; // no error

}

/* 
 * Called whenever the contents of a directory are desired. Could be from an 'ls'
 * or could even be when a user hits TAB to do autocompletion
 */
static int cs1550_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	//Since we're building with -Wall (all warnings reported) we need
	//to "use" every parameter, so let's just cast them to void to
	//satisfy the compiler
	(void) offset;
	(void) fi;
	printf("readdir->path:%s\n",path);	

	//This line assumes we have no subdirectories, need to change
	//if (strcmp(path, "/") != 0)
	//	return -ENOENT;

	//the filler function allows us to add entries to the listing
	//read the fuse.h file for a description (in the ../include dir)
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	/*
	//add the user stuff (subdirs or files)
	//the +1 skips the leading '/' on the filenames
	filler(buf, newpath + 1, NULL, 0);
	*/
	struct stat stbuf;
	int res=cs1550_getattr(path,&stbuf);
	cs1550_root_directory root=get_root();	
	printf("readdir->res%d\n",res);
	int isRoot=0;
	if (strcmp(path,"/")==0)
		isRoot=1;

	if (res!=-ENOENT && isRoot==0)
	{
		printf("readdir->dir_name: %s  readdir->dir_index: %d",directory,dir_index);
		int f_blk=root.directories[dir_index].nStartBlock;
 		cs1550_directory_entry sub_dir;
		sub_dir= get_block_at(f_blk);
		printf("read_attr -> f_blk:%d  sub_dir.nFiles:%d\n",f_blk,sub_dir.nFiles);
		int c=0;
		
		while(c<sub_dir.nFiles)
		{
			char fname[MAX_FILENAME+MAX_EXTENSION+2];	
			memset(fname,0,MAX_FILENAME+MAX_EXTENSION+2);
			printf("sub_fname:%s sub_fext:%s\n",sub_dir.files[c].fname,sub_dir.files[c].fext);	
			printf("strcmp(\"\",ext):%d\n",strcmp(sub_dir.files[c].fext,""));	
			if(strcmp(sub_dir.files[c].fext,"")!=0)
			{
				strcat(fname,sub_dir.files[c].fname);
				strcat(fname,".");
				strcat(fname,sub_dir.files[c].fext);
			}
			else
				strcat(fname,sub_dir.files[c].fname);
		
			printf("ls: %s\n",fname);
			filler(buf, fname, NULL, 0);
			c++;
		}
	}
	else 
	{
		int c=0;
		while(c<root.nDirectories)	
		{
			if(isRoot==1 || strncmp(directory,root.directories[c].dname,strlen(directory))==0)
			{
				filler(buf, root.directories[c].dname, NULL, 0);
			}
			c++;
		}

	}
	
	return 0;
}

/* 
 * Creates a directory. We can ignore mode since we're not dealing with
 * permissions, as long as getattr returns appropriate ones for us.
 */


static int cs1550_mkdir(const char *path, mode_t mode)
{
	(void) path;
	(void) mode;
	struct stat stbuf;
	
	int res=cs1550_getattr(path,&stbuf);
	
	//if the directory already exists
	if (res!=-ENOENT)
		return -EEXIST;
	if (res==-ENAMETOOLONG)
		return res;

	cs1550_root_directory root;
	root=get_root();
	
	//increment dir_count by 1, return error if exceeds max 
	root.nDirectories++;	
	printf("nDirs > max_ndirs: %d nDirs max_dirs:%d %lu\n",root.nDirectories > (int)MAX_DIRS_IN_ROOT,root.nDirectories,MAX_DIRS_IN_ROOT);

	if (root.nDirectories> MAX_DIRS_IN_ROOT)
		return -ENOSPC;	

	/*int i=7;
	while(i>=0)
	{
		printf("%d ",(6>>i)&1);
		i--;
	}	
	*/
	//write the directory in .disk
	long block_num=write_bmap(1);
	printf("bitmap written\n");	
	cs1550_directory_entry dir;
	
	memset(&dir, 0, sizeof(dir));
	dir.nFiles=0;
	write_block(block_num,&dir);
	
	//update the root struct and write root
	strcpy(root.directories[root.nDirectories-1].dname,directory);
	root.directories[root.nDirectories-1].nStartBlock=block_num;		
	printf("mkdir newly added subdir: %s start_blk: %d\n",root.directories[root.nDirectories-1].dname,root.directories[root.nDirectories-1].nStartBlock);
	printf("subdir-> first file name: %s\n",dir.files[0].fname);	
	printf("dir->nFiles: %d\n",dir.nFiles);
	write_block(0,&root);
	
	//print_disk();
	return 0;
}

/* 
 * Removes a directory.
 */
static int cs1550_rmdir(const char *path)
{
	(void) path;
    return 0;
}

/* 
 * Does the actual creation of a file. Mode and dev can be ignored.
 *
 */
static int cs1550_mknod(const char *path, mode_t mode, dev_t dev)
{
	(void) mode;
	(void) dev;
	
	
	printf("mknod->path: %s\n",path);
	struct stat stbuf;	
	int res=cs1550_getattr(path,&stbuf);
	printf("mknod->res: %d\n",res);
	if (res==0)
		return -EEXIST;
	if (res!=-ENOENT)
		return res;	
	else
	{	
		cs1550_root_directory root=get_root();	
		long dir_blk=root.directories[dir_index].nStartBlock;	
		struct cs1550_directory_entry dir;
		memset(&dir,0,sizeof(dir));
		dir=get_block_at(dir_blk);
		dir.nFiles++;
		printf("dir.nFiles:%d\n",dir.nFiles);
		if(dir.nFiles>MAX_FILES_IN_DIR)
			return -ENOSPC;
		//get a block for file data
		long f_blk=write_bmap(1);		
		
		//update the directory block and write back to .disk
		strcpy(dir.files[dir.nFiles-1].fname,filename);
		strcpy(dir.files[dir.nFiles-1].fext,extension);
		dir.files[dir.nFiles-1].fsize=0;	
		dir.files[dir.nFiles-1].nStartBlock=f_blk;
		
		write_block(dir_blk,&dir);	
		
		//now instantiate the file data block and write back to .disk
		struct cs1550_disk_block file;
		memset(&file,0,sizeof(file));
		write_block(f_blk,&file);
	}
	return 0;
}

/*
 * Deletes a file
 */
static int cs1550_unlink(const char *path)
{
    (void) path;

    return 0;
}

/* 
 * Read size bytes from file into buf starting from offset
 *
 */
static int cs1550_read(const char *path, char *buf, size_t size, off_t offset,
			  struct fuse_file_info *fi)
{
	(void) buf;
	(void) offset;
	(void) fi;
	(void) path;

	//check to make sure path exists
	struct stat stbuf;	
	int res=cs1550_getattr(path,&stbuf);
	printf("read -> size:%d offset:%d\n",size,offset);
	//check that size is > 0
	//nothing to read
	if (size<=0)
		return 0;
 
	//check that offset is <= to the file size
	if (offset>stbuf.st_size)
		return -EFBIG;
	
	//read in data
	
	//set size and return, or error

	size = 0;

	return size;
}

/* 
 * Write size bytes from buf into file starting from offset
 *
 */
static int cs1550_write(const char *path, const char *buf, size_t size, 
			  off_t offset, struct fuse_file_info *fi)
{
	(void) buf;
	(void) offset;
	(void) fi;
	(void) path;
	
	//check to make sure path exists
	struct stat stbuf;	
	int res=cs1550_getattr(path,&stbuf);
	if (res==-ENOENT)
		return res;
	//check that size is > 0
	//nothing to read
	if (size<=0)
		return 0;
 	
	//check that offset is <= to the file size
	if (offset>stbuf.st_size)
		return -EFBIG;
	
	//write data
	printf("size:%d off_t:%d\n",size,offset);
	printf("file_num:%d\n",file_num);
	cs1550_disk_block db=get_file_block_at(file_num);
	cs1550_directory_entry de=get_block_at(dir_num);
	char bmap[BITMAP_SIZE];
	get_bmap(bmap);
	printf("file_num:%d \nwrite->print bmap:\n",file_num);
	//printf("write->buf:%s (this line has no new line, buf may contain one)",buf);	
	if(size+offset<=BLOCK_SIZE)
	{
		int c=0;
		while(c<size)
		{
			db.data[c+offset]=buf[c];
			c++;	
		}
		if (size+offset>de.files[file_index].fsize)
			de.files[file_index].fsize=size+offset;
		
	}
	else
	{
		printf("write->append:\n");
		//calculate how many more blocks are needed, and then check check next n contiguous block status in bitmap, if one of the next n blocks is occupied, reject
		int added_blks=(offset+size)/BLOCK_SIZE-1;
		long add_cnt=0;
		while(add_cnt<added_blks)
		{	
			if(get_status(bmap,file_num+add_cnt+1)==1)
				return -ENOSPC;				
		}
		
		//if enough space is left, first write the first block, if necessary
		long c=0;
		while(c+offset<BLOCK_SIZE)
		{
			db.data[c+offset]=buf[c];
			c++;	
		}
		printf("finished writing the first block\n");
		//set_status(bmap,1,file_num+1);
		add_cnt=0;
		while(add_cnt<added_blks)
		{
			set_status(bmap,1,file_num+add_cnt+1);
			long tmp_c=c;
			cs1550_disk_block da;
			memset(&da,0,sizeof(da));
			
			while(c<tmp_c+BLOCK_SIZE && c<size)
			{
				da.data[c-tmp_c]=buf[c];
				c++;
			}
			write_block(file_num+add_cnt+1,&da);
		}
		printf("file_num:%d \nwrite->print(added) bmap:\n",file_num);
		print_map(bmap);
				

	}
			
	printf("write->dir.nFile: %d file_index:%d\n",de.nFiles,file_index);
	write_block(file_num,&db);//update the file block
	write_block(dir_num,&de);//update the directory block
	//set size (should be same as input) and return, or error
	return size;
}

/******************************************************************************
 *
 *  DO NOT MODIFY ANYTHING BELOW THIS LINE
 *
 *****************************************************************************/

/*
 * truncate is called when a new file is created (with a 0 size) or when an
 * existing file is made shorter. We're not handling deleting files or 
 * truncating existing ones, so all we need to do here is to initialize
 * the appropriate directory entry.
 *
 */
static int cs1550_truncate(const char *path, off_t size)
{
	(void) path;
	(void) size;

    return 0;
}


/* 
 * Called when we open a file
 *
 */
static int cs1550_open(const char *path, struct fuse_file_info *fi)
{
	(void) path;
	(void) fi;
    /*
        //if we can't find the desired file, return an error
        return -ENOENT;
    */

    //It's not really necessary for this project to anything in open

    /* We're not going to worry about permissions for this project, but 
	   if we were and we don't have them to the file we should return an error

        return -EACCES;
    */

    return 0; //success!
}

/*
 * Called when close is called on a file descriptor, but because it might
 * have been dup'ed, this isn't a guarantee we won't ever need the file 
 * again. For us, return success simply to avoid the unimplemented error
 * in the debug log.
 */
static int cs1550_flush (const char *path , struct fuse_file_info *fi)
{
	(void) path;
	(void) fi;

	return 0; //success!
}


//register our new functions as the implementations of the syscalls
static struct fuse_operations hello_oper = {
    .getattr	= cs1550_getattr,
    .readdir	= cs1550_readdir,
    .mkdir	= cs1550_mkdir,
	.rmdir = cs1550_rmdir,
    .read	= cs1550_read,
    .write	= cs1550_write,
	.mknod	= cs1550_mknod,
	.unlink = cs1550_unlink,
	.truncate = cs1550_truncate,
	.flush = cs1550_flush,
	.open	= cs1550_open,
};

//Don't change this.
int main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &hello_oper, NULL);
}
