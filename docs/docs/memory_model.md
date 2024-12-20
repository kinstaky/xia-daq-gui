# 内存模型

## 引入

> [!WARNING]
>
> 时间有限，下面都是乱写的，别太当真。

XIA 获取中的内存可以分为几个部分

+ 每个通道的内存
+ 每个插件（模块）的内存
+ 计算机中的内存

其中每个通道的内存应该是极小的，可能只是用于临时存储一到两个事件，并且很快（大概率是 us 量级）会被读取到模块的内存中。

而模块的内存可能有几 KB 到 几百 KB，不太清楚到底有多大，但应该不到 MB 级别。在不存储波形的情况下，模块中的内存可以存储大量的事件，这些事件来自于不同的通道。从时间顺序的角度来看，模块中存储的事件是整体有序而局部无序。在较大的时间尺度下是有序的，比如说三分钟后采集到的事件大概率会在现在采集的事件后面，这里的三分钟只是虚指，需要结合实际的计数率考虑。而较小的时间尺度下，事件可能是乱序的，10us 后采集的事件有可能在当前事件的前面。

而计算机中的内存是完全从模块内存中复制的，而最后存储到硬盘中的事件就是计算机内存中的事件。当然，从某种意义上来说，硬盘的空间也是广义的内存的一部分。所以这里的内存其实是狭义的内存，仅在程序运行周期内有效，由操作系统分配给进程的内存。

在使用数字化获取的时候，不难发现其中的事件的时间是无序的，这完全是由模块内存中事件的无序导致的。模块中记录的事件是来自于所有通道的，而不同通道中，即使是同一个事件，也有存储快慢的区别。因此模块通过轮询（我猜的）的方式读取通道中的数据时，并不会考虑到时间的先后顺序，而是按照通道序号排序。但是在通道只能临时存储一个事件的前提假设下（我猜的，但大概率是真的），同一个事件的信号，在模块的内存中也是排列在一起的，而不同的事件不会混杂在一起。

所以，以下两点是明确的

+ 同一个通道中的事件是按时间排序的
+ 同一个模块中，不同的事件不会混杂

这应该是 XIA 的内存的特征，也是之后设计在线程序的基础。