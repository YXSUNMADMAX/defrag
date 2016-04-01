#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>

struct superblock{
	int size;
	int inode_offset;
	int data_offset;
	int swap_offset;
	int free_inode;
	int free_iblock;
}sb;

#define N_DBLOCKS 10
#define N_IBLOCKS 4

struct inode{
	int next_inode;
	int protect;
	int nlink;
	int size;
	int uid;
	int gid;
	int ctime;
	int mtime;
	int atime;
	int dblocks[N_DBLOCKS];
	int iblocks[N_IBLOCKS];
	int i2block;
	int i3block;
};

int h_data=-1,h_index=-1,cnt=0,num;


// read the num th blocks in buf from f 
void read_blocks(FILE *fin,int num,void *block){
	fseek(fin,(num+sb.data_offset)*sb.size+1024,SEEK_SET);
	fread(block,sb.size,1,fin);
}



void dfs(FILE *fin,FILE *fout,int deep,void *buf,int rank){
	if(cnt==num) return;
	if(deep==0){
		printf("%d\t",rank);
		// printf("%s\n",(char*)buf);
		fwrite(buf,sb.size,1,fout);
		++cnt;
		return;
	}
	int *buff=(int*)buf;
	int i;
	void *tem_buf=malloc(sb.size);
	for(i=0;i<sb.size/sizeof(int);++i){
		if(cnt==num) break;
		read_blocks(fin,buff[i],tem_buf);
		dfs(fin,fout,deep-1,tem_buf,buff[i]);
	}

	free(tem_buf);
	return ;
}


int main(int argc,char **argv){
	assert(argc==2);

	FILE *fin,*fout;
	fin=fopen(argv[1],"r");
	fout=fopen("out","w");
	assert(fin!=NULL);
	// output file name

	// copy datafile to output file
	void *buf;
	buf=malloc(512);
	assert(buf!=NULL);
	assert(fread(buf,512,1,fin)==1);
	assert(fread(buf,512,1,fin)==1);
	// get the superblock
	memcpy(&sb,buf,sizeof(struct superblock));
//	printf("size=%d\tinode_off=%d\t,data_off=%d\t,swap_offset=%d\t,free_ino=%d\t,free_blo=%d\n",sb.size,sb.inode_offset,sb.data_offset,sb.swap_offset,sb.free_inode,sb.free_iblock);

	// the number of inodes
	int n_inodes=(sb.data_offset-sb.inode_offset)*(sb.size/sizeof(struct inode));
	// printf("%d\n",n_inodes);
	size_t bytes_of_inodes=(sb.data_offset-sb.inode_offset)*sb.size;
	// to store the inodes array
	struct inode *inodes=(struct inode *)malloc(bytes_of_inodes);
	fseek(fin,1024,SEEK_SET);
	fread(inodes,bytes_of_inodes,1,fin);
	
	// defrag
	// the number of index blocks and the number used in the final index block
	int i,j,k,r;

	for(i=0;i<n_inodes;++i){
		// free inode
		if(!inodes[i].nlink)
			continue;
		cnt=0;
		printf("\nnode %d block num:%d\n",i,(int)ceil(1.0*inodes[i].size/sb.size));
		int i_size=inodes[i].size;
		// the number of the data-blocks to store data
		num=ceil(1.0*i_size/sb.size);
		// printf("%d\n",num);
		for(j=0;j<10;++j){
			printf("%d\t",inodes[i].dblocks[j]);
			read_blocks(fin,inodes[i].dblocks[j],buf);
			fwrite(buf,sb.size,1,fout);
			++cnt;
			if(cnt==num) break;
		}
		if(cnt==num) continue;
		for(j=0;j<4;++j){
			read_blocks(fin,inodes[i].iblocks[j],buf);
			dfs(fin,fout,1,buf,inodes[i].iblocks[j]);
			if(cnt==num) break;
		}
		if(cnt==num) continue;
		read_blocks(fin,inodes[i].i2block,buf);
		dfs(fin,fout,2,buf,inodes[i].i2block);
		if(cnt==num) continue;
		read_blocks(fin,inodes[i].i3block,buf);
		dfs(fin,fout,3,buf,inodes[i].i3block);
	}
	printf("\n");
	free(buf);
	free(inodes);
	fclose(fin);
	fclose(fout);
	return 0;
}











