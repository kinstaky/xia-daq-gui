# 上手指南

## 引入

应用本框架开始上手在线程序的编写，需要明白几点

1. 目标是什么，或者说在线程序要达成什么效果
2. 怎么在框架中写在线程序，需要写点什么
3. 该框架中有哪些可以调整的参数

要达成什么效果？简单来说，就是要在打开程序后，弹出一个窗口，窗口内有各种持续更新的直方图，数据来源是正在运行的获取。至于怎么写程序和框架中的参数，后续将详细描述。

## 怎么写程序

最直接的方法是复制 `template/` 文件夹中的模板，在此基础上写自己的在线分析程序。模板文件夹中包括了两个文件，一个是 `online_template.cpp`，即代码；另一个是 `CMakeLists.txt`文件，是编译的时候用的。

**推荐**用法是，在项目根目录中新建一个文件夹（后面假设这个文件夹叫 `my_online_src`），将 `template/` 中的两个文件都复制过去，然后再新的文件夹（`my_online_src`）中的代码基础上修改。

### 确定画几个图

1. 确定在线的窗口里要画几个图（后面假设为 N 个），
2. 确定每个图都画什么东西
3. 修改第 28 行的 `graph_num` 变量，修改为需要画的图的数量 N。

```cpp
constexpr int graph_num = N;
```

接着转到第  145 行

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

最后跳转到第 162 行

```cpp
std::vector<TH1*> histograms = {&hist1, &hist2};
```

将所有的直方图的指针放到 `histograms`中，这是为了可以在键入 **r** 或者 **F5** 的时候重置图像。

### 写入获取插件信息

跳转到 175 行，告诉在线程序一些基本的插件信息。

```cpp
std::vector<int> module_sampling_rate = {100, 100, 100, 100};
std::vector<int> group_index = {0, 0, 0, 0};
```

1. 修改模块的采样率信息，只能填 100、250、500，根据实际情况填写
2. 修改模块的组编号，这是用于读取模块数据时的同步机制，详见[内存模型](memory_model.md)，下面也会简单阐述怎么写组编号。

#### 组策略

**组策略**是本项目为了保障不同模块数据能够匹配的机制，而模块的组编号就是**组策略**的核心参数。简单来说，因为不同的模块对应的探测器计数率有很大的差异，所以有的记录的东西多有的少。记得多的内存很快就满了，并被读取到硬盘中。而记得少的，数据可能一直留在模块的内存里。时间积累下，计算机同一时间从模块读取到的数据是不匹配的，记得少的插件只有后半截数据能和记得多的插件对上。

而本项目设计的组策略，保障了属于同一组模块中的匹配数据会被同时读取。而组编号是用来标记模块所属的组，同一个编号的模块就是属于同一个组。所以在设置组编号时，要把关联的模块设置为同一组。所谓关联的模块，一般来说是同属于一个探测器或者一套探测器的模块，比如说同一个 32 路的双面硅微条对应的 4 个模块，又比如说同一套 PPAC 对应的一两个模块。或者从另一个角度来说，如果在线程序中画某一个图需要至少两个模块，那么这两个模块应该属于同一组。

**简而言之**，设置模块的组编号时，把相互关联（需要在同一个图中使用的）的模块设置为同一组。

### 写数据分析的代码

接着是核心部分的，数据分析相关的代码。这也是不同实验中最大的差异。从第 177 行开始就是从共享内存中读取数据并分析、画图。

```cpp
OnlineDataReceiver receiver(
    app_name,
    "DaqPacket", run, crate,
    module_sampling_rate, group_index
);
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

第 1-5 行实例化了类 `OnlineDataReceiver`，这个类负责从共享内存中读取数据，并打包成事件。

第 6 行的循环仅在用户按下 Ctrl+C 或者关闭窗口时终止。

第 7 -11 行的循环，不断向 `OnlineDataReceiver` 请求打包好的事件

第 12 行调用`FillOnlineGraph`，将 `event` 中的数据填到直方图中。模板中这个函数是空的，需要用户编写数据分析的内容。

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
4. 修改插件信息，包括采样率和组编号
5. 补充 `FillOnlineGraph` 函数，根据定义修改对应的调用参数
6. 修改 `time_window`，适配符合窗
7. 修改两个`CMakeLists.txt`

## 可调整参数

目前，模板中给出了 5 个可调节参数，放在了程序最前面（头文件后面），分别是

1. `fresh_rate`图像刷新率，单位为 Hz
2. `group_num` 画图的数量，单位为个
3. `window_width` 程序窗口的宽度，单位为像素
4. `window_height`程序窗口的高度，单位为像素
5. `time_window`符合窗的宽度，单位为 ns

## 更多

+ 如果想要了解模板中其他部分的含义，参考[模板详解](template_walk_through.md)
+ 如果想要参考可运行的代码，或者数据分析和在线结合的例子，请看源代码中的 `examples` 目录和[示例详解](example_details.md)
+ 如果想要了解共享内存的机制，参考开发者指南
