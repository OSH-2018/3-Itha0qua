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
#define UNUSED -2

struct filenode {
    char filename[MAX_NAME_LENGTH];
    size_t content;
    struct stat st;
    struct filenode *next;
};
static const size_t block_size=8192;  //8kb
static const size_t blocknr=131072; //total size = 1GB
static void *mem[131072];
static const size_t head=131072*sizeof(size_t)/8192;
static const size_t head_size=8192/sizeof(size_t);
size_t tail=131072;
size_t tail_used=0;
struct filenode **root;

static size_t alc()
{
  size_t *a;
  for(size_t i=0;i<head;i++)
  {
     a=(size_t *) mem[i];
    for(size_t j=0;j<head_size;j++)
    {
      if(a[j]==UNUSED)
      {
	a[j]=NONE;
	mem[head_size*i+j]=mmap(NULL, block_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        return head_size*i+j;
      }
    }
  }    
  return -1;
}

static struct filenode *get_filenode(const char *name)
{
    printf("start get filenode!\n");
    struct filenode *node = *root;
    printf("root->name=%s\n",(*root)->filename);
    if(!*root) return NULL;
    while(node) {
	printf("size=%ld content=%ld\n",node->st.st_size,node->content);
	
        if(strcmp(node->filename, name + 1) != 0)
            node = node->next;
        else
            return node;
	printf("node=%p\n",node);
    }
    return NULL;
}

static int oshfs_rename(const char *name, const char *new)
{
    struct filenode* node = get_filenode(name);
    if(node==NULL)
        return -ENOENT;
    if(strlen(new)<=MAX_NAME_LENGTH)
        memcpy(node->filename,new+1,strlen(new));
    else memcpy(node->filename,new+1,MAX_NAME_LENGTH);
    return 0;
}


static struct filenode *alc_node()
{
  printf("start allocate node!\n");
  struct filenode *node;
  if(sizeof(struct filenode)+tail_used<block_size)
  {
     printf("2 case 1\n");
     node = (struct filenode*) (mem[tail]+tail_used);
     tail_used+=sizeof(struct filenode);
  }
  else
  {
    printf("2 case 2\n");
    ((size_t *)mem[tail/head_size])[tail%head_size]=tail-1;
    tail--;
    if(((size_t *)mem[tail/head_size])[tail%head_size]!=UNUSED)
	    return NULL;
    else ((size_t *)mem[tail/head_size])[tail%head_size]=NONE;
   mem[tail]=mmap(NULL, block_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    node = (struct filenode*) mem[tail];
    tail_used=sizeof(struct filenode);
  }
  printf("3 tail_used=%ld block_size=%ld node=%p\n",tail_used,block_size,node);
  node->next=NULL;
  printf("4\n");
  return (struct filenode*) node;
}

static void create_filenode(const char *filename, const struct stat *st)
{
    printf("start create!\n");
    size_t num=alc();
    printf("num=%ld\n",num);
    if(num==-1) return;
    struct filenode *new=alc_node();
    printf("allocate node success!\n");    
    if(strlen(filename)>MAX_NAME_LENGTH)
    memcpy(new->filename, filename,MAX_NAME_LENGTH);
    else memcpy(new->filename, filename,strlen(filename));
    printf("length of name=%ld\n",strlen(filename));
    memcpy(&new->st, st, sizeof(struct stat));
    new->next = *root;
    new->content = NONE;
    *root = new;
}

static void *oshfs_init(struct fuse_conn_info *conn)
{
   printf("start init!\n");
   memset(mem,0,sizeof(void *) * blocknr);
   size_t *a;
   size_t t=0,i=0;
   for(i=0;i<blocknr;i++)
   {
    if(i<head||i==blocknr-1)
    mem[i]= mmap(NULL, block_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
   if(i<head)
   {
     a=(size_t *) mem[i];
     for(size_t j=0;j<head_size;j++)
     if(t<head-1)
     {
	     a[j]=i*head_size+j+1;//connections in head
	     t++;
     }
     else if(t==head-1)
     {
             a[j]=NONE;//the end of head
	     t++;
     }
     else a[j]=UNUSED;//other connections
   }	   
   else if(i==blocknr-1)
   {
     tail=i;
     tail_used=sizeof(struct filenode*);
     root = (struct filenode**) mem[blocknr-1];
     *root=NULL;
   }
   }

   printf("init finish!\nhead=%ld\ntail=%ld\n",head,tail); 
   return NULL; 
}

static int oshfs_getattr(const char *path, struct stat *stbuf)
{
    printf("start getattr!\n");
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
    struct filenode *node = *root;
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
    printf("start mknod!\n");
    struct stat st;
    st.st_mode = S_IFREG | 0644;
    st.st_uid = fuse_get_context()->uid;
    st.st_gid = fuse_get_context()->gid;
    st.st_nlink = 1;
    st.st_size = 0;
    st.st_blksize = block_size;
    st.st_atime = time(NULL);
    st.st_mtime = time(NULL);
    st.st_ctime = time(NULL);
    create_filenode(path + 1, &st);
    return 0;
}

static int oshfs_open(const char *path, struct fuse_file_info *fi)
{
    printf("start open!\n");
    if (get_filenode(path)==NULL)
	return -errno;
    return 0;
}


static int oshfs_write(const char *path,const char *buf, size_t size, off_t ofs, struct fuse_file_info *fi)
{
    printf("start write!\n");
    struct filenode *node = get_filenode(path);
    if (node == NULL) return -1;
    printf("ofs=%ld st_size=%ld\n",ofs,node->st.st_size);
    if(ofs>node->st.st_size) return -ENOENT;
    if(node->st.st_size<ofs+size)
    node->st.st_size = ofs + size;
    node->st.st_mtime = time(NULL);
    node->st.st_atime = time(NULL);
    if(node->content==NONE)
    {
       node->content = alc();
       if(node->content==NONE)return -ENOSPC;
    }
    size_t c=node->content;
    void *sta;
    size_t rec=NONE;
    printf("2\n");
    while(ofs>block_size)
    {
     // printf("c=%ld ofs=%ld\n",c,ofs);
      if(c==NONE)
      {
	  printf("rec=%ld ofs=%ld\n",rec,ofs);
	  return -ENOSPC;
      }
      rec=c;
      c = ((size_t *)mem[c/head_size])[c%head_size];
      if(c==NONE)
      {
        c=alc();
	 ((size_t *)mem[rec/head_size])[rec%head_size]=c;
      }
      ofs-=block_size;
    }//forward  to find where to start
    printf("3 c=%ld rec=%ld ofs=%ld\n",c,rec,ofs);
    if(c==NONE)
    {
      c=alc();
      if(c==NONE) return -ENOSPC;
      if(rec!=-1) ((size_t *)mem[rec/head_size])[rec%head_size]=c;
    }
     size_t total =  (ofs+size-1)/block_size+1; 
     node->st.st_blocks = total;
    sta=mem[c]+ofs;
    char *tem=(char *)buf;  
    if((block_size-ofs)>size)
    {
      memcpy(sta,tem,size );
      return size;
    }
    printf("4\n");
    size_t lf=size-block_size+ofs;
    memcpy(sta,tem, block_size-ofs);
    tem+=block_size-ofs;
    while(lf>block_size)
    {
       if(((size_t *)mem[c/head_size])[c%head_size]==NONE)
       {
       rec=c;
       c=alc();
       ((size_t *)mem[rec/head_size])[rec%head_size]=c;
       }
       else
       {
       rec=c; 
       c=((size_t *)mem[rec/head_size])[rec%head_size];
       }
       memcpy(mem[c],tem,block_size);
       lf-=block_size;
       tem+=block_size;
       if(rec==c)
       {
         printf("error!/n");
	 return -ENOSPC;
       }
    }//whole  blocks
    printf("4.5\n");
    rec=c;
    if(((size_t *)mem[rec/head_size])[rec%head_size]==NONE)
    {
    c=alc();
    ((size_t *)mem[rec/head_size])[rec%head_size]=c;
    }
    else c=((size_t *)mem[rec/head_size])[rec%head_size];
    printf("4.7 c=%ld\n",c);
    memcpy(mem[c],tem,lf);//the last block 	
    printf("5 end in %ld\n",c);
    return size;
}

static int oshfs_truncate(const char *path, off_t size)
{
    printf("start truncate!\n");
    struct filenode *node = get_filenode(path);
    node->st.st_blocks = (size-1) / block_size+1; 
    printf("size:%ld->%ld\n",node->st.st_size,size);
    node->st.st_size = size;
    size_t c = node->content;
    if(c==NONE) {c=alc(); node->content=c;}
    size_t ofs=size,rec;
    while(ofs>block_size)
    {
        if(c==NONE) 
	{
	  c=alc();
	  ((size_t *)mem[rec/head_size])[rec%head_size]=c;
	}
        rec=c;
        c=((size_t *)mem[c/head_size])[c%head_size];
	ofs-=block_size;
	//printf("1 c=%ld rec=%ld\n",c,rec);
    }	
    rec=c;
    if(c!=NONE)
    {
           c=((size_t *)mem[c/head_size])[c%head_size];
           ((size_t *)mem[rec/head_size])[rec%head_size]=NONE;
    } 
    printf("2 c=%ld\n",c);
    while(c!=NONE&&c!=UNUSED)
    {
      rec=((size_t *)mem[c/head_size])[c%head_size];
      munmap(mem[c],block_size);
      mem[c]=NULL;
      ((size_t *)mem[c/head_size])[c%head_size]=UNUSED;
      c=rec;
    }
    printf("3 c=%ld\n",c);
    return 0; 
}

static int oshfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    printf("start read!\n");
    struct filenode *node = get_filenode(path);
    int ret = size;
    if(offset>node->st.st_size) return 0; 
    if(offset + size > node->st.st_size)
        ret = node->st.st_size - offset;
    size_t c=node->content;
    void *sta;
    size_t ofs=offset,rec;
    while(ofs>block_size)
    {
      if(c==NONE) return 0;
      rec=c;
      c=((size_t *)mem[c/head_size])[c%head_size];
      ofs-=block_size;
    }//forward  to find where to start
    sta=mem[c]+ofs;
    if(block_size-ofs>ret)
    {
      memcpy(buf,sta, ret);
      return ret;
    }
    char* tem=buf;
    memcpy(tem, sta, block_size-ofs);
    tem+=block_size-ofs;
    size_t lf=ret-block_size+ofs;
    while(lf>block_size)
    {
       c=((size_t *)mem[c/head_size])[c%head_size];
       memcpy(tem,mem[c],block_size);
       lf-=block_size;
       tem+=block_size;
    }
    memcpy(tem,mem[((size_t *)mem[c/head_size])[c%head_size]] , lf);
    return ret;
}

static int oshfs_unlink(const char *path)
{
    printf("start unlink!\n");
    struct filenode *node = *root;
    struct filenode *parent=NULL;
    printf("root->filename=%s\n",(*root)->filename);
    if(strcmp((*root)->filename,path+1)==0)
    { 
	 node=*root;
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
    printf("1\n");
    if(node==NULL)
    return -ENOENT;    
    if(parent==NULL)
    *root=(*root)->next;
    else
    parent->next=node->next;
    size_t c=node->content,rec;
    munmap(node,sizeof(struct filenode));
    printf("2 c=%ld\n root=%p\n",c,*root);
    while(c!=NONE)
    {
      rec=((size_t *)mem[c/head_size])[c%head_size];
      munmap(mem[c],block_size);
      ((size_t *)mem[c/head_size])[c%head_size]=UNUSED;
      printf("loop\n");
      mem[c] = NULL;
      c=rec;
      printf("c->next=%ld\n",c);
    }

    printf("3\n");
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
    .rename = oshfs_rename
};

int main(int argc, char *argv[])
{
    return fuse_main(argc, argv, &op, NULL);
}