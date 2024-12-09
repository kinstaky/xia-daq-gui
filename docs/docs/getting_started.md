# 上手指南

## 引入

应用本框架开始上手在线程序的编写，需要明白几点

1. 目标是什么，或者说在线程序要达成什么效果
2. 怎么在框架中写在线程序，需要写点什么
3. 该框架中有哪些需要修改的参数

要达成什么效果？简单来说，就是要在打开程序后，弹出一个窗口，窗口内有各种持续更新的直方图，数据来源是正在运行的获取。至于怎么写程序和框架中的参数，后续将详细描述。

## 怎么写程序

最直接的方法是复制 `template/` 文件夹中的模板，在此基础上写自己的在线分析程序。模板文件夹中包括了两个文件，一个是 `online_template.cpp`，即代码；另一个是 `CMakeLists.txt`文件，是编译的时候用的。

**推荐**用法是，在项目根目录中新建一个文件夹（后面假设这个文件夹叫 `my_online_src`），将 `template/` 中的两个文件都复制过去，然后再新的文件夹（`my_online_src`）中的代码基础上修改。

### 确定画几个图

1. 确定在线的窗口里要画几个图（后面假设为 N 个），
2. 确定每个图都画什么东西
3. 修改第 29 行的 `graph_num` 变量，修改为需要画的图的数量 N。

```cpp
constexpr int graph_num = N;
```

接着转到第  150 行

```cpp
// create canvas
TCanvas* canvas = new TCanvas(
    "canvas", "Online", 0, 0, window_width, window_height
);
TH1F hist1;
TH2F hist2;
// create histograms
// draw
canvas->Divide(2, 1);
canvas->cd(1);
hist1.Draw();
canvas->cd(2);
hist2.Draw();
```

按照上面的代码

1. 创建 N 个直方图（TH1，TH2）
2. 用 `Divide` 函数将 `canvas` 分割成好几份
3. 用 `Draw` 和 `cd` 分别在 `canvas` 每一份中画一个图

最后跳转到第 168 行

```cpp
std::vector<TH1*> histograms = {&hist1, &hist2};
```

将所有的直方图的指针放到 `histograms`中，这是为了可以在键入 **r** 或者 **F5** 的时候重置图像。

### 写数据分析的代码

接着是核心部分的，数据分析相关的代码。这也是不同实验中最大的差异。从第 182 行开始就是从共享内存中读取数据并分析、画图。

```cpp
OnlineDataReceiver receiver(app_name, "DaqPacket");
while (receiver.Alive()) {
    for (
        std::vector<DecodeEvent> *event = receiver.ReceiveEvent(time_window);
        event;
        event = receiver.ReceiveEvent(time_window)
    ) {
        FillOnlineGraph(*event, hist1, hist2);
    }
}
```

第 1 行实例化了类 `OnlineDataReceiver`，这个类负责从共享内存中读取数据，并打包成事件。

第 2 行的循环仅在用户按下 Ctrl+C 或者关闭窗口时终止。

第 3 -7 行的循环，不断向 `OnlineDataReceiver` 请求打包好的事件

第 8 行调用`FillOnlineGraph`，将 `event` 中的数据填到直方图中。模板中这个函数是空的，需要用户编写数据分析的内容。

**总之，这一部分用户只需要修改 `FillOnlineGraph` 函数的定义和调用即可。**

### 数据格式

写数据分析的代码依赖于 `OnlineDataReceiver` 给出的 `event` 的数据格式，即 `std::vector<DecodeEvent>`，一个`DecodeEvent`的数组。而 `DecodeEvent`的定义如下

```cpp
struct DecodeEvent {
	int module;
	int channel;
	int energy;
	double time;
	int64_t timestamp;
	double cfd;
	bool used;
};
```

+ module，模块序号，即插件在机箱中的位置，从左到右从 0 开始
+ channel，通道序号，通道在插件中的位置，从上到下从 0 开始
+ energy，能量道址
+ time，以这个 run 开始时刻为起点的时间，单位为 ns
+ *timestamp，以这个 run 开始时刻为起点的时间戳，单位为 ns*
+ *cfd，对 timestamp 的时间补充*
+ *used，这个事件是否被使用了，用户不需要关心*

其中后面 3 个斜体的变量，是我认为数据分析中大概率用不上的，不重要的量。也就是说前面 4 个变量就差不多了。

另一方面，注意到 `OnlineDataReceiver` 中给出的事件是放在 `vector` 里的。放在同一个 `vector` 中的事件都是在同一个时间窗内的，或者说同一个 `vector` 中的事件的时间差都比小于一个给定的值，这个值就是时间窗的宽度。时间戳的宽度又 `OnlineDataReceiver::ReceiveEvent` 函数给定，模板中的 `time_window` 的值是 1000，表示的是 1000 ns 的时间窗，实际在线程序需要根据符合时间适当调整。

### 编译

不难发现，本项目及其依赖项目都是靠 cmake 编译的，所以也要会一点点 cmake。cmake 相比于 make 需要将构建方式写到 `Makefile` 中，需要将构建和依赖写入到 `CMakeLists.txt` 中。为了方便，本项目同样提供了一份 `CMakeLists.txt` 的模板。

用户需要做的是把 `CMakeLists.txt` 中的 **online_template** 都修改为实际的程序的名字。打个比方，如果源文件是 `my_online.cpp`，那么新的程序一般也叫做 `my_online`。那就可以把 **online_template** 都修改为 **my_online**，注意 CMakeLists.txt 的第 17 行有一个名字是 **online_template_dict**，同样需要修改为 **my_online_dict**（当然，不改可能也没问题，但还是推荐改）。

另一方面，如果是按照前面推荐的操作的，那么`my_online.cpp`和 `CMakeLists.txt` 应该在同一个文件夹下（比如说`my_online_src`），并且这个文件夹是在本项目根目录下的，那还要在根目录的 `CMakeLists.txt` 中最后一行添加

```cmake
add_subdirectory(my_online_src)
```

这样 cmake 从根目录开始编译时，才能找到这个新的文件夹。

**最后提示一下**，编译的方法是在根目录使用[简介](index.md)提到的用 4 个线程编译的命令

```bash
cmake --build build -- -j4
```

**小结**一下，需要做

1. 修改当前目录 `CMakeLists.txt` 中的所有 **online_template**
2. 在根目录 `CMakeLists.txt` 最后添加一行代码

### 总结

为了方便，这里再次总结需要修改和补充的代码

1. 复制 `online_template.cpp`和 `CMakeLists.txt`
2. 修改 `graph_num`
3. 修改创建直方图，把直方图画到画布的部分
5. 补充 `FillOnlineGraph` 函数，根据定义修改对应的调用参数
6. 修改 `time_window`，适配符合窗
7. 修改两个`CMakeLists.txt`

## 修改参数

>  [!WARNING]
>
> 必须注意这部分的内容，获取程序的组编号、对齐、共享内存大小和符合窗的宽度是**必须**根据实验情况调整的内容，会直接影响在线程序的正常运行。

### 模板参数

目前，模板中给出了 5 个可调节参数，放在了程序最前面（头文件后面），分别是

1. `fresh_rate`图像刷新率，单位为 Hz
2. `group_num` 画图的数量，单位为个
3. `window_width` 程序窗口的宽度，单位为像素
4. `window_height`程序窗口的高度，单位为像素
5. `time_window`符合窗的宽度，单位为 ns
6. `screenshot_path` 截图的保存路径

### DAQ 运行时参数

如果你已经比较熟悉 PKUXIADAQ，那么一定对 `parset/cfgPixie16.txt` 这个配置文件不陌生。本项目在其中加入了几行用于在线程序的配置，需要根据实际情况调整。需要该的参数包括 `ModuleGroupIndex`（第 35 行）和 `ModuleAlignment`（第 39 行）。其中 `ModuleGroupIndex`，即组编号，是和**组策略**机制相关的。而 `ModuleAlignment`，即模块对齐参数，是和共享内存传输数据的机制相关。这两个参数的设置都和 `ModuleBits` 类似，第一个数表示有多少个模块，后面的数依次表示每个模块对应参数的值。下面两小节将分别解释如何设置这两个参数。

#### 模块组编号

**组策略**是本项目为了保障不同模块数据能够匹配的机制，而模块的组编号就是**组策略**的核心参数。简单来说，因为不同的模块对应的探测器计数率有很大的差异，所以有的记录的东西多有的少。记得多的内存很快就满了，并被读取到硬盘中。而记得少的，数据可能一直留在模块的内存里。时间积累下，计算机同一时间从模块读取到的数据是不匹配的，记得少的插件只有后半截数据能和记得多的插件对上。

而本项目设计的组策略，保障了属于同一组模块中的匹配数据会被同时读取。而组编号是用来标记模块所属的组，同一个编号的模块就是属于同一个组。所以在设置组编号时，要把关联的模块设置为同一组。所谓关联的模块，一般来说是同属于一个探测器或者一套探测器的模块，比如说同一个 32 路的双面硅微条对应的 4 个模块，又比如说同一套 PPAC 对应的一两个模块。或者从另一个角度来说，如果在线程序中画某一个图需要至少两个模块，那么这两个模块应该属于同一组。

**简而言之**，设置模块的组编号时，把相互关联（需要在同一个图中使用的）的模块设置为同一组。

#### 模块对齐

XIA 输出的每个事件的长度不是固定的，取决于获取中设置的参数。在最下的情况下是 4 words，即 16 字节。在下列情况事件的长度会增加

+ 记录能量求和，增加 4 字（16 字节）
+ 记录外部时钟时间戳，增加 2 字（8字节）
+ 记录 QDC，增加 8 字（32 字节）
+ 记录波型，波型的每一个采样点增加 2 个字节

在同一个 run 中，同一个通道的事件长度保持不变，但是不同通道有可能因为配置不同而事件长度不一样。在计算机读取模块的数据时，是从一个模块中读取所有通道的二进制数据，只有在解码后才能知道每个事件有多长。然而一边获取一边解码有可能影响获取的性能，话句话说，获取的时候是不知道买个事件有多长的。所以这些二进制数据表面上是顺序结构，实际上是链表结构，需要一个一个顺序读取。所以除非从头开始读取，否则是难以知道每个事件的边界。

**所以如果在共享内存的时候，一旦中间有事件丢失，就会导致后面的事件都无法读取。**为了避免这个问题，可以让同一个模块的事件保持一个固定的长度，从链表结构变成顺序结构。这样就可以准确知道每个事件的边界。

所以，**对于可能改变时间长度的配置（上面列出的），同一个模块中的所有通道必须保持一致，也就是说要求同一个模块的所有通道的事件长度一致**。这样的要求虽然看似苛刻，但是实际情况是同一个模块大概率接入的时同一类型的探测器，使用相同的配置是可行的。设置好每一个模块后，计算出每个模块的事件的长度，以字（4个字节）为单位，填入配置文件`ModuleAlignment` 中。比如说，如果什么都没选，那么事件的长度就是默认的 4 字 16 字节，那么就填入数字 4。

#### 固件位置

> [!WARNING]
>
> 这部分非常重要，虽然和在线没有直接关系，但是直接关系到获取程序能否运行！！！

本项目没有附带任何的 pixie-16 的固件，因此需要从另外下载固件并修改 cfgPixie16.txt 中的固件路径（建议使用绝对路径）。

+ 从 [PKUXIADAQ](https://github.com/wuhongyi/PKUXIADAQ/tree/master/firmware) 中下载固件，如果已经安装 PKUXIADAQ 则忽略这一步
+ 修该 cfgPixie16.txt 中的固件路径

### 共享内存大小

在 `include/daq_packet.h` 中可以看到一下语句。

```cpp
// constexpr size_t PACKET_SIZE = 32768; // 128kB
// constexpr size_t PACKET_SIZE = 4096; // 16kB
constexpr size_t PACKET_SIZE = 1024; // 4kB
```

这就是用来调整共享内存的大小的参数。这里的大小，指的是单个模块一次共享的数据的大小。实际使用获取程序时，当且仅当一组内任意一个模块缓存的数据量接近 `PACKET_SIZE` 时，整一组模块的数据才会共享到内存中。简单来说，模块的计数率越高、`PACKET_SIZE` 越小，共享的频率就越高，反之则越低。

共享频率越高，那么在线程序读取数据就越频繁，好处是能够更及时先是数据，坏处是占用的 CPU 和内存资源更多，共享频率特别高时有堵塞获取的风险。与之相对，共享频率太低就不能及时展现实验条件的变化。

所以需要合理估算模块的数据量，已确定共享频率和共享内存大小。单个通道，在不记录波型的情况下，一个事件一般占用 16 字节。在记录波型时，波型一个采样点占用 2 个字节。可以根据这些数据估算需要的共享内存大小。

## 更多

+ 如果想要了解模板中其他部分的含义，参考[模板详解](template_walk_through.md)
+ 如果想要参考可运行的代码，或者数据分析和在线结合的例子，请看源代码中的 `examples` 目录和[示例详解](example_details.md)
+ 如果想要了解共享内存的机制，参考开发者指南
