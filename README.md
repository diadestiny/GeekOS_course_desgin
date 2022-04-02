# GeekOS (GOSFS文件系统)
桂电2021年操作系统课设源码，主要实现了project5:GOSFS文件系统和project6:管道操作

> project origin : GUET_Operation_Course_Design
>
> last update time : 2021.5.2
>
> modify author: GUET_diadestiny

* 完善分页机制中的缺页中断处理(project4)
* 实现GOSFS文件系统的基本操作(project5)
* 实现基于管道操作的进程通信(project6)

同时本项目支持课设报告中的project2、project5、project6、部分project3、部分project4的功能，具体演示图查看对应课设报告部分。

因为网上已经有不少往届学长学姐关于project0、project1、project2、project3、project4的博客和源码，因此本项目主要实现project5和project6

博客链接：https://blog.csdn.net/weixin_43723614/article/details/123926025

## 1. 运行环境

笔者运行环境：真机ubuntu16.04、gcc-5.4.0、nasm-2.08.2

* 经过测试，若读者环境为ubuntu9虚拟机，make出错有可能需要对makefile的部分地方进行修改，以及模拟器配置文件.bochsrc进行部分修改。

* 配置GOSFS文件系统需要重点注意：

    >ata0-master: type=disk, mode=flat, translation=auto, path="diskc.img"...
    >ata0-slave: type=disk, path="diskd.img" ...

  其中diskc.img是原生的PFAT文件系统挂载区域，注意配置加上第二行，diskd.img是GOSFS文件挂载区域。

## 2.运行步骤

* 编译步骤: 1.cd build 2.make depend 3.make
* 启动步骤: 1.cd build 2.输入bochs 3.接着输入c

## 3. 用户可运行功能进程位于src/user/目录下

> 只读PFAT文件系统位于/c路径下，区域为/ide0，可读写GOSFS文件系统位于/d路径下，区域为ide1

* ls /c 
* cat [file_path_name]
* mkdir /d/[directory_name] 
* rm [directory_path_name|file_path_name]
* ls /c | more //管道通信
* fsend --消息队列mqueue发送消息到缓冲区
* frecv  --消息队列mqueue从缓冲区接受消息
* 其他可支持的命令cp cat等(具体见src/user/目录)


## 4. 参考来源

  1. http://www.cs.uni-salzburg.at/~ckrainer/OS-Winter-2006/BOS/index.html


>本项目改自于国外bos教学操作系统
>