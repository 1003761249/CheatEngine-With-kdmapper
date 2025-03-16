# Cheat-Engine-with-kdmapper
黑客项目_通过kdmapper手动映射DBK驱动，并重构DBK与Cheat Engine的通讯机制，绕过微软签名


本项目的内容和目的，是在基于kdmmaper的开源项目，手动映射Cheat Engine的DBK驱动，实现DBK驱动的隐藏，与Cheat Engine正常通讯，实现所有内核功能

1、绕过微软驱动强制签名问题，无论官方，还是自编译Cheat Engine项目都无法绕过驱动签名问题，本项目核心解决如此
2、解决Cheat Engine DBK驱动特征（如注册表、服务名，驱动模块）等特征，容易被特征识别
3、DBK源码级别重构，去除官方DBK项目中各种逻辑bug



1、首先Cheat Engine 的常规通讯方式 为调用Windows应用原生API读写内存，和其他的一些功能

2、第二种则是通过DBK驱动，向DBK内核驱动发送通讯控制码，实现通讯

3、第三种通过DBVM设备，即一个内核VT虚拟机设备来实现通讯，如果DBVM已经加载，则DBK的相关控制派遣函数会被通过DBVM转发到DBK，但是加载DBVM设备需要通过DBK
去启动，那么DBK就必须获得微软的签名，因此仍然无法解决这个问题
