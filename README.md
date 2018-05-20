# OSH——LAB03  
## 映射方法  

块大小4K，块个数131072，这两个为给定参数，共计1G（虚拟机性能较差，所以取的小了一些）  
前head个数的块储存块之间的关系（head由131072\*sizeof(size_t)8192计算得出），使用size_t整型储存:  
* UNUSED=-2 表示块未被使用，此时块并没有被分配内存
* NONE=-1 表示块被占用，但它并不指向任何块
* i（i>=0）表示块指向第i块

从最后一块开始记录节点信息，其中mem[blocknr-1]记录根节点\*root（占一个指针的位置），然后逐渐加入（struct filenode）节点类型来储存文件信息
节点包含：
* char filename[MAX_NAME_LENGTH];
* size_t content;
* struct stat st;
* struct filenode *next;

所有节点构成一个以\*root为根节点的链表，每次查询文件遍历链表即可
若记录节点信息的块被占满，则向前申请一块新块，若前一块被占用则报出内存不足的错误（为了稳定性，也可按照普通申请块方式申请一个内存块）


## 测试结果

```
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel# cd mountpoint
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# ls -al
总用量 4
drwxr-xr-x  0 root root    0 Jan  1  1970 .
drwxr-xr-x 17 root root 4096 May 20 18:55 ..
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# echo helloworld>testfile
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# ls -l testfile
-rw-r--r-- 1 root root 11 May 20 19:04 testfile
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# cat testfile
helloworld
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# cp ../oshfs.c oshfs.c
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# tar -zcvf oshfs.tar oshfs.c
oshfs.c
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# tar -zxvf oshfs.tar
oshfs.c
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# diff ../oshfs.c oshfs.c
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# ls -al
总用量 6
drwxr-xr-x  0 root root     0 Jan  1  1970 .
drwxr-xr-x 17 root root  4096 May 20 18:55 ..
-rw-r--r--  1 root root 11619 May 20 19:05 oshfs.c
-rw-r--r--  1 root root  3080 May 20 19:05 oshfs.tar
-rw-r--r--  1 root root    11 May 20 19:04 testfile
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# rm oshfs.c
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# rm oshfs.tar
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# ls
testfile
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# dd if=/dev/zero of=testfile bs=1M count=200
记录了200+0 的读入
记录了200+0 的写出
209715200 bytes (210 MB, 200 MiB) copied, 37.2392 s, 5.6 MB/s
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# ls -l testfile
-rw-r--r-- 1 root root 209715200 May 20 19:07 testfile
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# dd if=/dev/urandom of=testfile bs=1M count=1 seek=10
记录了1+0 的读入
记录了1+0 的写出
1048576 bytes (1.0 MB, 1.0 MiB) copied, 0.0859303 s, 12.2 MB/s
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# ls -al
总用量 5
drwxr-xr-x  0 root root        0 Jan  1  1970 .
drwxr-xr-x 17 root root     4096 May 20 18:55 ..
-rw-r--r--  1 root root 11534336 May 20 19:07 testfile
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# dd if=testfile of=/dev/null
记录了22528+0 的读入
记录了22528+0 的写出
11534336 bytes (12 MB, 11 MiB) copied, 0.0725905 s, 159 MB/s
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# rm testfile
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# ls -al
总用量 4
drwxr-xr-x  0 root root    0 Jan  1  1970 .
drwxr-xr-x 17 root root 4096 May 20 18:55 ..
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# cd -
/lib/modules/4.13.0-38-generic/kernel

```
