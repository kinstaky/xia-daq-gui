# Ubuntu 中编译 PLXSDK

由于 Ubuntu 和 PLXSDK 不太匹配，所以需要修改 PLXSDK 的代码之后再重新编译。下面的修改大部分都会给出对应的网站（reference）作为支撑，如果遇到更多的问题，可以参考最后一节并尝试自己解决。

## Ubuntu 20

### 修改 Driver/Source.Plx9000/Drivier.c

在最前面加三行

```c
#ifndef INCLUDE_VERMAGIC
#define INCLUDE_VERMAGIC
#endif
```

不改的话会有编译错误，参见头文件 `linux/vermagic.h` 的代码可以发现，这个头文件似乎不允许被其它文件包含。只有定义了 `INCLUDE_VERMAGIC`之后才能包含这个头文件。按照 Linux 的设计，这个头文件不应该被包含，但是这里权且如此，似乎也不会出错。

### 修改 Driver/Source.Plx9000/SuppFunc.c

#### 修改 1

注释掉 402 行到 410 行。这个函数不会产生编译错误，但是在运行时会出错。这个函数尝试声明它占有了一部分的地址空间，在运行时会因为声明失败导致整个驱动出错。但是这个函数实际上什么都没做，所去掉这个函数似乎是没问题的。

参考网址：https://stackoverflow.com/questions/7682422/what-does-request-mem-region-actually-do-and-when-it-is-needed

#### 修改 2

将 956 行修改为 `downread(&current->mm->mmap_lock)`，将 968 行修改为 `upread(&current->mm->mmap_lock)`。

这两个地方都是将 `mmap_sem` 修改为 `mmap_lock`，因为 Linux 内核更新了对应的变量名，参考网址 https://lkml.org/lkml/2020/5/20/66

## Ubuntu 22

### 修改 Driver/Source.Plx9000/Dispatch.c

将 276 行的 `vma->vm_flags |= VM_RESERVED` 修改为 `vm_flags_set(vma, VM_RESERVED)`。将 281 行的 `vma->vm_flag |= VM_IO` 修改为 `vm_flags_set(vma, VM_IO)`。

这是因为 Linux 内核更新后，`vm_flag` 变成了私有变量，不允许直接修改，而是通过 `vm_flag_set` 函数修改。参考网址

+ https://forums.developer.nvidia.com/t/driver-470-182-03-fails-to-compile-with-linux-6-3-1/251992
+ https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=1036173
+ https://gist.github.com/vejeta/9078219f082d2bfd62b08b6eada780e6

### 修改 Driver/Source.Plx9000/Drivier.c

在最前面加三行

```c
#ifndef INCLUDE_VERMAGIC
#define INCLUDE_VERMAGIC
#endif
```

不改的话会有编译错误，参见头文件 `linux/vermagic.h` 的代码可以发现，这个头文件似乎不允许被其它文件包含。只有定义了 `INCLUDE_VERMAGIC`之后才能包含这个头文件。按照 Linux 的设计，这个头文件不应该被包含，但是这里权且如此，似乎也不会出错。

### 修改 Driver/Source.Plx9000/SuppFunc.c

#### 修改 1

修改函数 `Plx_get_user_pages`，

从

```c
rc = Plx_get_user_pages(
	UserVa & PAGE_MASK,
    TotalDescr,
    (bDirLocalToPci ? FOLL_WRITE : 0),
    pdx->DmaInfo[channel].PageList,
    NULL
);
```

修改成

```c
rc = Plx_get_user_pages(
	UserVa & PAGE_MASK,
    TotalDescr,
    (bDirLocalToPci ? FOLL_WRITE : 0),
    pdx->DmaInfo[channel].PageList
);
```



参考网址

+ https://gist.github.com/Fjodor42/cfd29b3ffd1d1957894469f2def8f4f6
+ https://gist.github.com/joanbm/dfe8dc59af1c83e2530a1376b77be8ba#file-nvidia-470xx-fix-linux-6-5-patch

#### 修改 2

将 956 行修改为 `downread(&current->mm->mmap_lock)`，将 968 行修改为 `upread(&current->mm->mmap_lock)`。

这两个地方都是将 `mmap_sem` 修改为 `mmap_lock`，因为 Linux 内核更新了对应的变量名，参考网址 https://lkml.org/lkml/2020/5/20/66

## 了解更多

现在用的 PLXSDK，无论是版本 7 还是版本 8，所考虑的 Linux 系统版本都比较老，换句话说是稳定。然而本人认为，Ubuntu 已经是目前最流行的 Linux 系统之一，比各种老旧的系统更好用。一是使用的人多，那么遇到问题在网上搜索大概率能够找到解答，二是新系统对新硬件的匹配更好。

不过，事情总是有利有弊。享受了 Ubuntu 便捷的同时，也需要承受某些稳定的驱动和新版本的 Linux 内核不适配的问题。PLXSDK 是驱动程序，经常会调用一些底层的 Linux 内核相关的代码。而 PLXSDK 考虑的是比较稳定的 Linux 版本，使用 Ubuntu 20 和 22 的话，其 Linux 内核版本相对于旧版本有一些改变，所以调用的函数也需要相应地修改。

综上，因为 Ubuntu 的 Linux 内核版本比较新，而 PLXSDK 考虑的老版本，所以对应的内核函数变化后，就不兼容了，最后表现为编译错误。解决的方法就是修改 PLXSDK 中对应的函数。

那么怎么修改呢？

1. 直接把错误信息复制到 Google 里面，看看有没有人遇到同样的问题。可以加上关键词 Linux kernel 更容易找到对应的解决方案。
2. 直接查看 Linux 内核代码的变迁，一般解决方法也在内核代码中
3. 直接问 chatGPT，估计也能解答