#OSH——LAB03
测试结果：

>
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel# cd  mountpoint
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# ls -al
总用量 4
drwxr-xr-x  0 root root    0 Jan  1  1970 .
drwxr-xr-x 17 root root 4096 May 20 18:29 ..
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# echo helloworld>testfile
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# ls -l testfile
-rw-r--r-- 1 root root 11 May 20 18:29 testfile
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# cat testfile
helloworld
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# cp ../oshfs.c oshfs.c
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# tar -zcvf oshfs.tar oshfs.c
oshfs.c
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# tar -zxvf oshfs.tar
oshfs.c
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# diff ../oshfs.c oshfs.c
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# vim oshfs.c
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# rm oshfs.c
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# ls
oshfs.tar  testfile
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# rm oshfs.tar
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# dd if=/dev/zero of=testfile bs=1M count=20
记录了20+0 的读入
记录了20+0 的写出
20971520 bytes (21 MB, 20 MiB) copied, 1.04408 s, 20.1 MB/s
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# ls -al
总用量 5
drwxr-xr-x  0 root root        0 Jan  1  1970 .
drwxr-xr-x 17 root root     4096 May 20 18:29 ..
-rw-r--r--  1 root root 20971520 May 20 18:33 testfile
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# dd if=/dev/urandom of=testfile bs=1M count=1 seek=10
记录了1+0 的读入
记录了1+0 的写出
1048576 bytes (1.0 MB, 1.0 MiB) copied, 0.0753865 s, 13.9 MB/s
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# ls -al
总用量 5
drwxr-xr-x  0 root root        0 Jan  1  1970 .
drwxr-xr-x 17 root root     4096 May 20 18:29 ..
-rw-r--r--  1 root root 11534336 May 20 18:33 testfile
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# dd if=testfile of=/dev/null
记录了22528+0 的读入
记录了22528+0 的写出
11534336 bytes (12 MB, 11 MiB) copied, 0.0561924 s, 205 MB/s
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# rm testfile
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# ls
root@ubuntu:/lib/modules/4.13.0-38-generic/kernel/mountpoint# cd -
/lib/modules/4.13.0-38-generic/kernel
