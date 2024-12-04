# 获取流程详解

> [!Warning]
>
> 这部分也是随便写的。

## 引入

以后写

## 整体工作流程

```mermaid
---
title: DAQ flow
---
flowchart TD
	Start -- initialize everything-->Initialized
	Initialized -- read config, boot --> Ready
	Ready -- start run, init shared memory, open save file --> ReadDataLoop
	ReadDataLoop -- stop run, clean shared memory --> Ready
```



共享内存的准备分为两部分

1. 启动程序和 Boot 的时候，初始化变量、读取组编号、读取模块对齐长度
2. 每个 run 开始的时候，申请共享内存、初始化 publisher

因为 publisher 的构造参数依赖于 run 序号，所以每个 run 开始都要初始化 publisher。而在 run 结束的时候，就要重置变量、释放 publisher 的资源。

## 读取数据流程

### 原版的读取数据流程

```mermaid
---
title: Origin ReadDataLoop 
---
flowchart TD
	id1[CheckReadableSize] --> branch1{over threshold?}
	branch1 -- No --> id1
	branch1 -- Yes --> branch2{buffer full?}
	branch2 -- Yes --> id2[SaveToFile]
	branch2 -- No --> id3[ReadData]
	id2 --> id3
	id3 --> id1
```



### 加入共享内存后的读取流程

```mermaid
---
title: Online version ReadDataLoop
---
flowchart TD
	id1[
		reset group_read
		CheckReadableSize
		CheckPayloadSize
	] --> branch1{
		group_read
			OR
		over threshold?
	}
	branch1 -- No --> id1
	branch1 -- Yes --> branch2{buffer full?}
	branch2 -- Yes --> id2[
		CopyToPayload
		SaveToFile
	]
	id2 --> id3[ReadData]
	branch2 -- No --> id3
	id3 --> branch3{group read?}
	branch3 -- No --> id1
	branch3 -- Yes --> id4[
		CopyToPayload
		PublishPayload
		IncreasePacketId
	]
	id4 --> id1
```

