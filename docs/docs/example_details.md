# 示例详解

## 简介

这个示例是为了

1. 给出完整的在线程序的用法
2. 测试项目是否安装成功

为了满足第 1 点，示例中的在线程序也是在模板的基础上修改的。为了满足第 2 点，示例还包含了一个模拟程序，模拟真实的获取系统的运行，使得程序可以轻量化运行，避免了接入获取系统的麻烦。当然，真正的测试还是要接入探测器和获取系统。

## 情景介绍

示例中模拟了用正背面都是 32 条的双面硅微条（DSSD）测量 3 $\alpha$ 源。由于是模拟，源的大小是无限小的一个点。DSSD 竖直摆放（硅面平行于竖直方向的向量），形状是变长为 64 mm 的正方形，DSSD 的中心点距离源 10 mm。

实验室坐标系用的笛卡尔三维坐标系，原点设置在源的位置，y 的正方向是竖直向上，z 的正方向是源到 DSSD 的正中心的，x 就是正交于 y 和 z，并满足右手定则（$\vec x \times \vec y = \vec z$）。

DSSD 是一面 32 路的，两面就是 64 路，需要 4 个 pixie16 插件。这里假设映射如下表

| 模块 | DSSD 条    |
| ---- | ---------- |
| 0    | 正面 0-15  |
| 1    | 正面 16-31 |
| 2    | 背面 0-15  |
| 3    | 背面 16-31 |

在线程序的目的是画 5 个图，观测 5 个变量。

1. DSSD 中触发的 pixel，用横轴为 x 方向条的序号，纵轴为 y 方向条的序号的二维直方图表示
2. 测到的能量，用横轴为能量（MeV）的一维直方图表示
3. DSSD 正背面能量差，用横轴为能量 （MeV） 的一维直方图表示
4. 相邻事件的时间差，用横轴为时间差 （ns）的一维直方图表示
5. DSSD 正背面的时间差，用横轴为时间差（ns）的一维直方图表示

## 在线

我们直接按照[上手指南](getting_started.md)的步骤来写在线程序。

1. 复制源文件和 CMakeListst.txt，这里已经复制好了，就放在 exapmles 文件夹下，并且源文件叫 `online_example.cpp`
2. 修改 `graph_num` 为 5，因为要画 5 个图
3. 修改创建直方图部分，创建了 1 个二维直方图和 4 个一维直方图，然后把指针填到 `histograms` 里
4. 修改插件信息，这里不用修改，恰巧和模板一样
5. 补充 `FillOnlineGraph` 函数
6. 修改 `time_window`，懒得改了，反正 1000 也能用
7. 修改 `CMakeLists.txt`，将 online_template 换成 online_example

### 数据分析

前面提到，核心部分实质就是 `FillOnlineGraph` 的数据处理部分，这里摘出来解释一下。

```cpp
void FillOnlineGraph(
	const std::vector<DecodeEvent> &decode_event,
	double last_time,
	TH2F &hist_strip,
	TH1F &hist_energy,
	TH1F &hist_energy_difference,
	TH1F &hist_interval,
	TH1F &hist_time_difference,
	double &current_time
);
```

首先函数的定义就大幅更改了。第一个参数是 `OnlineDataReceiver` 打包好的事件。然后因为要画 5 个图，所以直接把 5 个图的引用都当作参数传入，如果有更多的图要画可以考虑用数组概括一下。最后是第二个参数和最后一个参数，用来记录当前事件的时间，并读取上一个事件的时间，方便用来画相邻事件时间间隔的那个图。

```cpp
if (decode_event.size() != 2) return;
```

一开始就是简单粗暴的排除符合不上的事件。由于示例中的探测模型非常简单，就是一个 $\alpha$ 粒子打在 DSSD 上，并且认为硅的效率是 100%，也不会产生噪声和相邻条事件，所以是非常干净的数据。那么这里就可以直接排除掉不是 DSSD 两面都有相应的事件。

```cpp
for (const DecodeEvent &event : decode_event) {
    if (event.module < 2) {
        front_strip = event.module * 16 + event.channel;
        front_energy = double(event.energy);
        front_time = event.time;
        double cali_p0 = calibration_parameters[front_strip][1];
        double cali_p1 = calibration_parameters[front_strip][0];
        front_energy = cali_p0 + cali_p1 * front_energy;
    } else {
        back_strip = (event.module-2) * 16 + event.channel;
        back_energy = double(event.energy);
        back_time = event.time;
        double cali_p0 = calibration_parameters[back_strip+32][1];
        double cali_p1 = calibration_parameters[back_strip+32][0];
        back_energy = cali_p0 + cali_p1 * back_energy;
    }
}
```

上面的代码是核心部分，干了映射、归一的活。ifelse 分支（第 2 行和第 9 行）是用来区分正背面，然后根据通道和模块确定条编号（第 3 行和第 10 行），实际就是映射。然后从 `event`中读取能量和时间信息（第 4-5 行和第 11-12 行），同样是映射的一部分。最后是归一（第 6-8 行和第 13-15 行），因为是模拟，其中的归一参数都是编的。如此，简单的 DSSD 映射和归一就完成了。

```cpp
if (front_strip < 0 || back_strip < 0) return;
hist_energy_difference.Fill(front_energy - back_energy);
// check front-back energy correlation
if (fabs(front_energy - back_energy) > 0.5) return;
hist_strip.Fill(front_strip, back_strip);
hist_energy.Fill(front_energy);
hist_interval.Fill(front_time - last_time);
hist_time_difference.Fill(front_time - back_time);
current_time = front_time;
```

最后一段代码主要是往图里填数据，除了第 1 行和第 4 行。第 1 行是判断映射后能不能找到正面条和背面条，如果之前的循环里没有找到的话，正面和背面条应该是初值 -1，所以这两者其中一个小于 0 时这个事件可以直接扔掉。第 4 行是正背面符合，要求正背面能量差小于 0.5 MeV 才算符合上，不然就直接扔掉。

以上就是这个简单示例中的数据分析的全部内容了，因为示例中的模型十分简单，所以代码显得很短。而实际情况往往复杂得多，需要写的代码也多得多。

## 模拟获取

感觉有点复杂，对于写在线程序好像也没有帮助，以后再写。