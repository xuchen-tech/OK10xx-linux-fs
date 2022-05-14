# 源码说明

> 此源码为保定飞凌嵌入式技术有限公司所开发的OK104x-Cx系列开发板的镜像编译源码
>
> 此源码基于NXP公司 Layerscape Software Development Kit - v18.06进行了深度定制
>
> 源码根目录下有各个开发板的懒人编译脚本，用户直接运行相对于脚本即可编译生成烧写用镜像文件



## 适用范围：

此源码适用于开发板型号如下：

- OK1046A-C2开发板 （包含一个FET1046A-C核心板和一个OK104xA-C2底板）
- OK1046A-C3开发板（包含一个FET1046A-C核心板和一个OK104xA-C3底板）
- OK1043A-C3开发板（包含一个FET1043A-C核心板和一个OK104xA-C3底板）



## 代码管理：

**#####提供给客户的OK10xx-linux-fs.tar.bz2源码包已经进行了此步操作，此部分忽略即可。#####**



为了方便开发，我们将LSDK分为四个部分进行管理。

**OK10xx-linux-fs：**此部分包含LSDK的编译开发环境，RCW源码，网络相关固件，安装相关固件等。

**OK10xx-linux-uboot：**此部分为开发板所使用的u-boot源码，已做部分闭源处理。

**OK10xx-linux-kernel：**此部分为开发板所使用的内核源码。

**OK10xx-linux-ubuntu：**此部分为开发板所使用的已编译好的未打包的Ubuntu文件系统目录。



在编译镜像之前，我们需要将这四部分源码进行组合。

将**OK10xx-linux-uboot**源码放入**OK10xx-linux-fs**源码的*flexbuild/packages/firmware/OK10xx-linux-uboot*路径下，

将**OK10xx-linux-kernel**源码放入**OK10xx-linux-fs**源码的*flexbuild/packages/linux/OK10xx-linux-kernel*路径下，

将**OK10xx-linux-ubuntu**目录放入**OK10xx-linux-fs**源码的*flexbuild/build/rfs/OK10xx-linux-ubuntu*路径下。



源码组合完成之后，记得删除每部分源码中包含的git信息以及权限处理脚本

```
rm OK10xx-linux-fs/.git -rf
rm OK10xx-linux-fs/flexbuild/packages/firmware/OK10xx-linux-uboot/.git -rf
rm OK10xx-linux-fs/flexbuild/packages/linux/OK10xx-linux-kernel/.git -rf
rm OK10xx-linux-fs/flexbuild/build/rfs/OK10xx-linux-ubuntu/.git -rf
rm OK10xx-linux-fs/flexbuild/build/rfs/OK10xx-linux-ubuntu/checkout.sh
rm OK10xx-linux-fs/flexbuild/build/rfs/OK10xx-linux-ubuntu/commit.sh
rm OK10xx-linux-fs/flexbuild/build/rfs/OK10xx-linux-ubuntu/permission.txt
```

然后对需要加密保护的部分进行闭源处理。



## 常用开发命令示例：

在源码的根目录下我们提供了支持的开发板的全编译懒人编译脚本，用户直接运行相对于脚本即可编译生成烧写用镜像文件，生成的镜像的位置在根目录下的**Image_output**目录下，客户可以直接将此目录下的全部内容拷贝到Fat32格式的U盘中制作烧写系统。

为了照顾部分客户需要单独编译部分镜像的需求，现将部分常用的单独编译的命令操作列举如下：

```shell
#进入编译源码路径
cd OK10xx-linux-fs/flexbuild
#设置环境变量
source setup.env
#清除之前编译操作的过程文件，避免报错
flex-builder clean

###############单独编译Firmware###############
#执行指令（-i <instruction>）clean-firmware，清除之前编译的firmware中间文件
flex-builder -i clean-firmware

#编译操作，指定需求的组件（-c <component>）为firmware，硬件结构（-a <arch>）为arm64，机器（-m <machine>）平台为ls1046ardb，启动方式（-b <boottype>）为qspi，Serdes1(-S <Serdes1>)配置为1040
flex-builder -c firmware -a arm64 -m ls1046ardb -b qspi -S 1040

#执行指令（-i <instruction>）mkfw,制作为firmware
flex-builder -i mkfw -a arm64 -m ls1046ardb -b qspi -S 1040

###############单独编译内核及模块###############
#执行指令（-i <instruction>）clean-linux，清除之前编译的linux中间文件
flex-builder -i clean-linux

#配置内核(如果使用默认配置可略过)
flex-builder -c linux:custom -m ls1046ardb -a arm64

#编译内核
flex-builder -c linux -a arm64 -m ls1046ardb

#编译cryptodev驱动
flex-builder -c cryptodev-linux -a arm64 -m ls1046ardb

#自动将驱动模块更新到文件系统
flex-builder -i merge-component -a arm64 -m ls1046ardb

#重新生成ubuntu镜像
flex-builder -i compressrfs -m ls1046ardb

#将编译好的内核、设备树文件更新到 build/images 目录
flex-builder -i genboot -m ls1046ardb

###############单独编译app程序###############
#执行指令（-i <instruction>）clean-apps，清除之前编译的apps中间文件
flex-builder -i clean-apps

#编译上层应用工具
flex-builder -c apps -m ls1046ardb

#更新app到文件系统中
flex-builder -i merge-component -a arm64 -m ls1046ardb

#重新生成ubuntu镜像
flex-builder -i compressrfs -m ls1046ardb
```


