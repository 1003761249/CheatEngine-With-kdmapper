# Cheat-Engine-with-kdmapper
黑客项目  通过kdmapper手动映射DBK驱动，并重构DBK原IoDispacher机制与Cheat Engine的通讯机制，绕过微软签名

我的bilibili主页 https://space.bilibili.com/131403708

本项目的内容和目的，是在基于kdmmaper的开源项目，手动映射Cheat Engine的DBKKernel驱动，实现DBK驱动的隐藏，与Cheat Engine正常通讯，实现DBVM加载、内存搜索等原CE全部功能

1、绕过微软驱动强制签名问题，无论官方，还是自编译Cheat Engine项目都无法绕过驱动签名问题，本项目核心解决如此
2、解决Cheat Engine DBKKernal驱动特征（如注册表、服务名，驱动模块）等特征，容易被特征识别
3、DBKKernal源码级别重构，精简官方DBK项目中多出逻辑Bug，和结构混乱，去除冗余文件代码，更新微软驱动开发库，排除编译问题
4、移植kdmapper在应用层，实现的内核模块遍历，和特征码搜索


原Cheat Engine DBKKernl通讯机制限制
1、Cheat Engine 的常规通讯方式 为调用Windows应用原生API读写内存，和其他的一些功能
2、第二种则是通过DBK驱动，向DBK内核驱动发送通讯控制码,和输入输出缓存，实现通讯并实现内核CE功能
3、第三种通过DBVM设备，即一个内核VT虚拟机设备来实现通讯，如果DBVM已经加载，则DBK的相关控制派遣函数会被通过DBVM转发到DBK，但是加载DBVM设备需要通过DBK
去启动，那么DBK就必须获得微软的签名，因此仍然无法解决这个问题


两种实现方式:
1、通过Hook内核驱动设备FastIoDeviceControl,，设备名称已在头文件中定义，实现隐蔽通讯，本项目示例为HookNull设备FastIoDeviceControl
2、通过ntoskrnl.exe的.data数据段来实现调用内核未导出函数，实现隐式通讯，目前已实现找到未导出函数地址，实现原理为，移植kdmmaper中C++方式应用层查找内核模块与特征码，转为Windows内核C语言模式的内核模块与特征码查找


dbk无签名加载机制的实现
1、DBK驱动DriverEntry中定义了应用层传递的四个参数、设备名、服务名、进程名、 线程名，通过注册表读写机制确认是否为Cheat Engine应用加载，否则加载失败，本项目去除了这一加载条件，在原加载机制和dbvm加载机制，中添加了第三种加载机制，
即手动无驱动对象加载，通过kdmapper传递入口参数，识别加载方式，并实现驱动可行初始化操作
2、应用层加载和通讯控制重构，应用层的需要先加载服务，再写入注册表4个配置项和驱动对应后才实现加载成功，本项目去除了所有应用层对注册表和控制服务的机制，和其他非Io通讯判断DBK成功加载特征，在原有的2中DeviceIoControl上添加了第三种通过Null设备的通讯，传递输入输出buffer和控制码
3、驱动卸载添加，驱动卸载不再通过系统服务控制，直接通过设备Io加卸载控制码实现，定义新操作码实现驱动卸载
4、内存隐藏机制，通过kdmmaper实现的加载独立Indepent内存机制实现无内存特征或PAGE tag标志
5、正常实现DBVM所有功能，初始化，调试器，内存扫描，实现真正的VT级别调试读写


项目调试心得

项目开发环境搭建心得
1、使用Visual Stdio 2022安装库
2、下载SDK的镜像文件即.iso的那个，方便重装操作系统后快速重建项目开发环境
3、WDK安装后也选择下载缓存，最好不是直接网络安装
4、最重要的一步，如果这些安装好后Visual Studio仍然没有出现驱动开发选项，请在Visual Studio installer中对IDE修改->添加单个组件->搜索WDK->重启IDE，可重建环境


Window内核项目通讯心得
1、深入理解Windows，DeviceIoControl在三环和零环的应用，结构和参数传递
2、内核调试和内核模块、理解各种地址转换、三环内存和零环内存机制，内存管理，如何重构原3环数据包，即控制码，和输入缓冲区，重构IRP通讯






