#define FUSE_USE_VERSION 26
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fuse.h>
#include <sys/mman.h>
#include <math.h>
#include <stdio.h>
#define MAX_NAME_LENGTH 256
#define NONE -1

struct filenode {
    char filename[MAX_NAME_LENGTH];
    size_t size;
    size_t content;
    struct stat st;
    struct filenode *next;
};
static const size_t blocksize=4096;  //4kb
static const size_t blocknr=262144; //total size = 1GB
static struct filenode *root = NULL;
static void *mem[262145];
static size_t *tail,*ocp;//the tailer of blocks, and the number of ocupied blocks
static size_t ava=4096-sizeof(size_t); //the size actually available

static size_t alc(size_t num)
{
  if(num>blocknr) return -1;
  for(size_t i=num;i<blocknr;i++)
	  if(!mem[i])
	  {
	    *tail=i;
	    *ocp+=1;
	    mem[i]=mmap(NULL,blocksize,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
            *(size_t*)mem[i]=NONE;//use the first size_t space of the block to store
	    return i;
	  }  
  return -1;
}
static struct filenode *get_filenode(const char *name)
{
    struct filenode *node = root;
    while(node) {
        if(strcmp(node->filename, name + 1) != 0)
            node = node->next;
        else
            return node;
    }
    return NULL;
}


static void create_filenode(const char *filename, const struct stat *st)
{
    size_t num=alc(*tail);
    if(num==-1) return;
    struct filenode *new=(struct filenode *)mem[num];	
    if(sizeof(filename)>MAX_NAME_LENGTH)
    memcpy(new->filename, filename,MAX_NAME_LENGTH+1);
    else memcpy(new->filename, filename,sizeof(filename)+1);
    memcpy(&new->st, st, sizeof(struct stat));
    new->next = root;
    new->size=num;
    new->content = NONE;
    root = new;
}

static void *oshfs_init(struct fuse_conn_info *conn)
{
   memset(mem,0,sizeof(void *) * blocknr);
   root=NULL;
   mem[0]= mmap(NULL, blocksize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
   tail =(size_t*) mem[0];
   ocp = (size_t*)(mem[0]+sizeof(size_t));
   *tail =0;
   *ocp=1;
   return NULL; 
}

static int oshfs_getattr(const char *path, struct stat *stbuf)
{
    int ret = 0;
    struct filenode *node = get_filenode(path);
    if(strcmp(path, "/") == 0)
    { 
        memset(stbuf,0,sizeof(struct stat));
        stbuf->st_mode = S_IFDIR | 0755;   
        stbuf->st_uid = fuse_get_context() -> uid;
        stbuf->st_gid = fuse_get_context() -> gid;
        return 0;
    }
    else if(node)
    {
        memcpy(stbuf,&node->st, sizeof(struct stat));
        return 0;
    } 
    else return  -ENOENT; 
}

static int oshfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    struct filenode *node = root;
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    while(node) {
        filler(buf, node->filename,& node->st, 0);
        node = node->next;
    }
    return 0;
}

static int oshfs_mknod(const char *path, mode_t mode, dev_t dev)
{
    struct stat st;
    st.st_mode = S_IFREG | 0644;
    st.st_uid = fuse_get_context()->uid;
    st.st_gid = fuse_get_context()->gid;
    st.st_nlink = 1;
    st.st_size = 0;
    st.st_blksize = blocksize;
    st.st_atime = time(NULL);
    st.st_mtime = time(NULL);
    st.st_ctime = time(NULL);
    create_filenode(path + 1, &st);
    return 0;
}

static int oshfs_open(const char *path, struct fuse_file_info *fi)
{

    if (get_filenode(path)==NULL)
	return -errno;
    return 0;
}

static int oshfs_write(const char *path,const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    struct filenode *node = get_filenode(path);
    if (node == NULL) return -1;
    if(offset>node->st.st_size+1) return -ENOENT;//when offset is too large
    node->st.st_size = offset + size;
    node->st.st_mtime = time(NULL);
    node->st.st_atime = time(NULL);
    if(node->content==NONE)
    {
       node->content = alc(*tail);
       if(node->content==NONE)return -ENOSPC;
    }//first write

    size_t c=node->content;
    void *sta;
    size_t ofs=offset,rec;
    while(ofs>ava)
    {
      if(c==NONE) return -ENOSPC;
      rec=c;
      c = *(size_t*) mem[c];
      ofs-=ava;
    }//forward  to find where to start
    if(c==NONE)
    {
      c=alc(*tail);
      if(c==NONE) return -ENOSPC;
      *(size_t *)mem[rec]=c;
    }
     size_t total = (offset+size)/ava; 
     node->st.st_blocks += total;
     if(total > (blocknr - *ocp)) return -ENOSPC;
    sta=mem[c]+ofs+sizeof(size_t);
 
    if((ava-ofs)>size)
    {
      memcpy(sta,buf,size );
      return size;
    }
    char* tem=(char *)buf;
    size_t lf=size-ava+ofs;
    memcpy(sta,tem, ava-ofs);
    tem+=ava-ofs;
    while(lf>ava)
    {
       rec=c;
       c=alc(*tail);
       *(size_t *) mem[rec]=c;
       memcpy(mem[c]+sizeof(size_t),tem,ava);
       lf-=ava;
       tem+=ava;
    }//whole  blocks
    rec=c;
    c=alc(*tail);
    *(size_t *) mem[rec]=c;
    memcpy(mem[c]+sizeof(size_t),tem,lf);//the last block 		 
    return size;
}

static int oshfs_truncate(const char *path, off_t size)
{
    struct filenode *node = get_filenode(path);
    if(size > node->st.st_size) return 0;//can not make file larger
    node->st.st_blocks = (size+ava-1) / ava; 
    node->st.st_size = size;
    size_t c = node->content;
    size_t ofs=size,rec;
    while(ofs>ava)
    {
        if(c==NONE) return 0;
        rec=c;
        c = *(size_t*) mem[c];
        ofs-=ava;
    }	
    rec=c;
    c = *(size_t*) mem[c];
    *(size_t *) mem[rec]=NONE;
    while(*(size_t *) mem[c]!=NONE)
    {
      rec=*(size_t *)mem[c];
      munmap(mem[c],blocksize);
      mem[c]=NULL;
      ocp-=1;
      c=rec;
    }
    return 0; 
}

static int oshfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    struct filenode *node = get_filenode(path);
    int ret = size;
    if(offset>node->st.st_size) return 0; 
    if(offset + size > node->st.st_size)
        ret = node->st.st_size - offset;
    size_t c=node->content;
    void *sta;
    size_t ofs=offset,rec;
    while(ofs>ava)
    {
      if(c==NONE) return 0;
      rec=c;
      c = *(size_t*) mem[c];
      ofs-=ava;
    }//forward  to find where to start
    sta=mem[c]+ofs+sizeof(size_t);
    if(ava-ofs>ret)
    {
      memcpy(buf,sta, ret);
      return ret;
    }
    char* tem=buf;
    memcpy(tem, sta, ava-ofs);
    tem+=ava-ofs;
    size_t lf=ret-ava+ofs;
    while(lf>ava)
    {
       c=*(size_t *) mem[c];
       memcpy(tem,mem[c]+sizeof(size_t),ava);
       lf-=ava;
       tem+=ava;
    }
    memcpy(tem,mem[*(size_t *) mem[c]]+sizeof(size_t) , lf);
    return ret;
}

static int oshfs_unlink(const char *path)
{
    struct filenode *node = root;
    struct filenode *parent=NULL;
    if(strcmp(root->filename,path+1)!=0)
    { 
	 node=root;
         parent=NULL;
    }
    else while(node) {
        if(strcmp(node->filename, path + 1) != 0)
            {
               parent=node;
               node = node->next;
            }
        else
            break;
    }
    if(node==NULL)
    return -ENOENT;    
    if(parent==NULL)
    root=root->next;
    else
    parent->next=node->next;
    size_t c=node->content,rec;
    while(c!=NONE)
    {
      rec=*(size_t *) mem[c];
      munmap(mem[c],blocksize);
      *ocp-=1;
      mem[c] = NULL;
      c=rec;
    }
    return 0;
}

static int oshfs_chown(const char * path, uid_t uid,gid_t gid){
    struct filenode* node = get_filenode(path);
    if(!node) return -ENOENT;
    node->st.st_uid = uid;
    node->st.st_gid = gid;
    return 0;
}
static int oshfs_utimens(const char * path,const struct timespec tv[2]){
    struct filenode* node = get_filenode(path);
    if(!node) return -ENOENT;
    node->st.st_mtime = node->st.st_atime = tv->tv_sec;
    return 0;
}
static int oshfs_chmod(const char *path,mode_t mode){
    struct filenode * node = get_filenode(path);
    if(!node) return -ENOENT;
    node->st.st_mode = mode;
    return 0;
}
static const struct fuse_operations op = {
    .init = oshfs_init,
    .getattr = oshfs_getattr,
    .readdir = oshfs_readdir,
    .mknod = oshfs_mknod,
    .open = oshfs_open,
    .write = oshfs_write,
    .truncate = oshfs_truncate,
    .read = oshfs_read,
    .unlink = oshfs_unlink,
    .chown = oshfs_chown,
    .chmod = oshfs_chmod,
    .utimens = oshfs_utimens,
};

int main(int argc, char *argv[])
{
    return fuse_main(argc, argv, &op, NULL);
}