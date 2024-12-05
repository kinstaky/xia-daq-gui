# 模板详解

用户可以从项目中的[模板文件](https://github.com/kinstaky/xia-daq-gui-online/blob/master/template/online_template.cpp) `template/online_template.cpp` 开始应用本项目的框架编写在线程序。如果只是简单的开发，只需要基本的 ROOT 相关的知识，熟练使用 TH1 和 TH2 直方图，熟练使用 `Fill` 函数填充直方图就足够了。在获取方面，只需要知道每一个事件对应于一个通道，而一个通道又对应与探测器的一路即可。

下面将详细解释模板文件中的代码的含义。

## 主要流程

从 `main()` 函数开始了解主要脉络。

```cpp
constexpr char app_name[] = "online_example";


// ROOT multi-thread preparation
ROOT::EnableThreadSafety();
// create ROOT application
TApplication app(app_name, &argc, argv);

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


// handle signal
std::unique_ptr<SignalHandler> signal_handler = HandleSignal(canvas);
// histograms
std::vector<TH1*> histograms = {
    &hist1, &hist2
};
st::string screenshot_name = GetScreenShotName();
// update GUI
std::thread update_gui_thread(
    UpdateGui,
    canvas,
    signal_handler.get(),
    histograms,
    screenshot_name
);


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

// wait for thread
update_gui_thread.join();
// terminate ROOT app
gApplication->Terminate();
```

上面的 `main()` 函数的代码，可以主要分为 4 块。

1. 4-21 行的 ROOT 相关的准备
2. 24-36 行的用户输入响应
3. 41-50 行的读取数据并处理
4. 52-55 行程序结束清理

在线程序首先读入输入参数，确定需要处理的是哪个 run 哪个机箱的数据。然后做一些 ROOT 相关的准备，准备好在线需要的直方图。用户输入的响应主要是两部分，一是按照设定的帧率刷新直方图，二是对键盘鼠标输入的响应。核心的部分是从共享内存中读取数据，然后根据用户编写的算法进行实质的在线分析，并填到直方图中。这部分也是经常做数据分析的人所擅长的。最后是清理一下程序中的东西，并优雅地结束程序。

**如果只是想要简单地开发基本能用的在线程序，实际上只需要关注第 3 部分数据处理相关的就足够了。**下面将详细解释每一部分的工作流程。

## ROOT 相关准备

为了方便，这里再一次展示 ROOT 准备的流程的代码。

```cpp
// ROOT multi-thread preparation
ROOT::EnableThreadSafety();
// create ROOT application
TApplication app(app_name, &argc, argv);

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

第 2 行启用了多线程安全。这是因为在线程序用到了两个线程，第一个线程负责读取数据并填入直方图，第二个线程将更新后的直方图画出来。由于两个线程同时调用了直方图（实际上一个线程读取，一个线程写入，并不矛盾），可能存在数据冲突，所以需要启用多线程安全。

第 4 行创建了 ROOT 的 TApplication，简单理解就是程序运行时的弹出的 ROOT 窗口，实质是包含窗口在内的一系列运行环境。ROOT 的包装非常全面，所以只需要创建这个类就不需要再操心了。在一般的教程中，这个类常常配合 `TApplication::Run` 函数来进入循环并处理用户响应，但由于在线程序需要在另一个循环中读取在线数据，所以只能放弃 `Run` 函数并在后面手动处理用户响应。

第 6 行到 18 行就是常规的创建画布并画图。虽然一般来说，都是把直方图填充完了再画图，这里只需要画空的直方图即可。这个程序的逻辑是后续读取在线数据后再填入直方图并更新画布。

## 用户输入响应

用户输入的响应包含两个部分

1. 对鼠标和键盘输入的响应。

2. 持续更新图像

### 对鼠标和键盘输入的响应

鼠标和键盘的输入，目前只考虑了两种情形

1. 点击窗口的关闭按钮终止在线程序
2. 按 **r** 或者 **F5** 重置图像
3. 按 **s** 或者 **F12** 保存截图

这两个功能都是依靠 `HandleSignal()`实现的。这个函数又依赖于 `SignalHandler`类，虽然听起来像套娃，但这都是有必要的。

```cpp
std::unique_ptr<SignalHandler> HandleSignal(TCanvas *canvas) {
	// signal handler
	std::unique_ptr<SignalHandler> signal_handler =
		std::make_unique<SignalHandler>();
	// connect close window and terminate program
	TRootCanvas *rc = (TRootCanvas*)canvas->GetCanvasImp();
	rc->Connect(
		"CloseWindow()",
		"SignalHandler", signal_handler.get(), "Terminate()"
	);
	canvas->Connect(
		"ProcessedEvent(Int_t, Int_t, Int_t, TObject*)",
		"SignalHandler",
		signal_handler.get(),
		"Refresh(Int_t, Int_t, Int_t, TObject*)"
	);
	return signal_handler;
}
```

上面的代码第 3-4 行使用智能指针创建了 `SignalHandler`类的实例，智能指针的好处就是不用自己管理内存，这个指针没用时会自动释放内存。`SignalHandler` 顾名思义，就是用来处理各种输入信号的，具体细节后面再看。

第 6-10 行，将窗口的关闭按钮和 `SignalHandler`的 `Terminate()` 函数关联起来，再用户点击关闭按钮同时，会调用`Terminate()`函数。该函数产生一个中断信号，类似于按 Ctrl+C。这是为了让管理共享内存的 iceoryx 停止工作，这个会在第三部分再次提到。

```cpp
void Terminate() {
    std::raise(SIGINT);
}
```

第 11-16 行，将 ROOT 自身的处理键盘和鼠标输入的函数 `ProcessedEvent`和 `SignalHandler::Refresh` 函数关联起来，也就是说每次用户点击鼠标或者敲键盘时，都会调用 `Refresh` 函数并给一些输入信息。`Refresh` 函数在发现输入信息是 **F5** 或者 **r** 时就会设置 `should_refresh_` 为真，这里不直接重置图像是因为 `SignalHandler` 本身并不知道应该重置哪些图像，而想要让它知道就要传递更多的参数，这样就不够灵活了。而这里设置一个变量给外面的函数或者类判断是否需要重置，就可以在这个类外面完成重置的事情。所谓的外面，其实就是指的用户编写的在线程序。同理，在检测到 **F12** 或者 **s** 时就会设置 `should_save_` 为真，告诉外面的程序是时候截图保存了。

```cpp
void Refresh(
    int event,
    int x,
    int y,
    TObject*
) {
    if (
        (event == 24 && x == 0 && y == 4148)
        || (event == 24 && x == 114 && y == 114)
    ) {
        // F5 and r
        should_refresh_ = true;
    } else if (
        (event == 24 && x == 0 && y == 4155)
        || (event == 24 && x == 115 && y == 115)
    ) {
        // F12 and s
        should_save_ = true;
    }
}
```

另一方面，程序中使用的 `Connect` 函数是 ROOT 提供的，要求关联的类必须是继承自 `RQ_Object`的，所以必须设置 `SignalHandler` 这个类。而且这个类编译时还需要关联 `LinkDef.h` 头文件，所以编译时还需要用到一些技巧，详见后面的**用cmake构建**一节。

### 持续更新图像

持续更新，顾名思义，需要做两件事，持续和更新。为了保证持续性，需要一个无限循环，所以创建了一个新的线程。

```cpp
// update GUI
std::thread update_gui_thread(
    UpdateGui,
    canvas,
    signal_handler.get(),
    histograms
);
```

这个线程调用的是 `UpdateGui` 函数。

```cpp 
void UpdateGui(
	TCanvas *canvas,
	SignalHandler *handler,
	const std::vector<TH1*> &histograms,
	const std::string &screenshot_name
) {
	while (!iox::posix::hasTerminationRequested()) {
		for (int i = 1; i <= graph_num; ++i) {
			canvas->cd(i);
			canvas->Update();
			canvas->Pad()->Draw();
		}
		if (handler->ShouldRefresh()) {
			for (size_t i = 0; i < histograms.size(); ++i) {
				histograms[i]->Reset();
			}
		}
		if (handler->ShouldSave()) {
			canvas->Print((
				screenshot_path + screenshot_name + GetTime() + ".png"
			).c_str());
		}
		gSystem->ProcessEvents();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000/fresh_rate));
	}
	canvas->Print((
		screenshot_path + screenshot_name + GetTime() + ".png"
	).c_str());
}
```

这个函数主体是一个循环，终止条件是用户按下 Ctrl+C。前面提到了用户点击关闭按钮时，会触发`SignalHandler::Terminate`函数并调用 `raise(SIGINT)` 函数，这同样会产生一个中断信号，和按下 Ctrl+C 是一样。换句话说，这个循环的终止条件是用户按下 Ctrl+C 或者点击窗口右上角的关闭按钮。

循环中的 8-12 行是遍历画布中的所有 pad，并更新图像，常规的 ROOT 代码。

第 13-17 行是问一下 `SignalHandler` 有没有收到重置图像的指令，有的话就重置所有的图像。因为需要知道要重置哪些图像，所以需要传入参数`const std::vector<TH1*> &histograms`，这里需要传入所有的需要重置的图像的指针。所以可以在 `main()`函数代码中的 27-29 行看到需要构建一个直方图的数组。

第 18-22 行时保存截图，保存的截图的路径是在一开始的参数中设置的，而每个文件会按照机箱编号-run-时间的形式来组织。所以一方面需要 `screenshot_name` 来输入包含机箱和 run 信息的字符串，另一方面需要用 `GetTime()` 函数来获取当前时间并转化为字符串。

第 23 行是 ROOT 周期性的刷新，可以简单理解成刷新窗口。

第 24 行是休眠，根据设置的刷新率休眠一定时间。注意到这里以 us 为单位，所以刷新率不能高于 1000 Hz，当然，实际使用时，10 Hz 以下的刷新率就绰绰有余了。

第 26-28 是在结束这个函数之前，再保存一次截图。结束这个函数也意味着整个程序的结束，换句话说就是在程序关闭前，迅速保存一次截图，再关闭程序。

## 读取内存并处理数据

详见[上手指南](getting_started.md)。

## 清理

在第二部分的用户输入响应中，我们创建了一个线程 `gui_thread` 来持续地更新图像并处理用户输入，所以在程序结束时，我们也要回收这个线程的资源，不然程序结束后显示 break。虽然这个 break 只会在程序结束后产生，理论上并不会造成任何影响，但是不够优雅。所以调用

```cpp
update_gui_thread.join();
```

来等待线程结束并回收。

另一方面还需要结束之前创建的 `TApplication`，所以调用

```cpp
gApplication->Terminate();
```

来终止 ROOT 的程序。

顺便一提，如果程序中有用到 `new` 或者 `malloc` 来申请内存，也应该在这一部分释放内存。

## 用 cmake 构建

这里简单解释模板 `CMakeLists.txt`中的代码的作用。当然，推荐还是系统学习 cmake，这里推荐 [HSF](https://hsf-training.github.io/hsf-training-cmake-webpage/) 上的教程。

```cmake
add_executable(online_template online_template.cpp)
target_include_directories(
	online_template PRIVATE ${PROJECT_SOURCE_DIR} ROOT_INCLUDE_DIRS
)
target_link_libraries(
	online_template PUBLIC
	online_data_receiver
	ROOT::Graf ROOT::Gpad
)
set_target_properties(
	online_template PROPERTIES
	CXX_STANDARD_REQUIRED ON
	CXX_STANDARD ${ICEORYX_CXX_STANDARD}
	POSITION_INDEPENDENT_CODE ON
)
root_generate_dictionary(
	online_template_dict ${PROJECT_SOURCE_DIR}/include/signal_handler.h
	MODULE online_template
	LINKDEF ${PROJECT_SOURCE_DIR}/include/linkdef.h
)
```

模板中一共包含 5 个命令

1. `add_executable` 新增一个可执行文件，包含 2 个参数
    1. 可执行文件的名字，即编译后的程序就叫 `online_template`
    2. 源代码文件的名字
2.  `target_include_directories`，给一个目标设置包含路径。所谓的目标即 `add_executable` 或者 `add_library` 之类的命令添加的东西，这里设置的目标是`online_template`，即上一个命令所声明的。包含路径就是源码中包含的头文件的所在目录，这里指定了两个目录，一个是项目的根目录 `${PROJECT_SOURCE_DIR}`，另一个是 ROOT 的头文件目录 `ROOT_INCLUDE_DIR`
3.  `target_link_libraries`，给一个目标设置链接的库，这里目标设置为 `online_template`。而链接的库是 `online_data_receiver`是[上手指南](getting_started.md)中提到 `OnlineDataReceiver` 类。另外两个链接的库是 ROOT 相关的，分别是 TH1、TH2、TGraph 相关的 `ROOT::Graf` 和 TCanvas 相关的 `ROOT::Gpad`
4.  `set_target_properties`，给一个目标设置一些编译参数，这里主要是依赖的 iceoryx 库要求的，实际就设置了两个参数，一个是 iceoryx 要求的 C++ 标准，另一个是需要位置无关的代码
5.  `root_generate_dictionary`，这个是 ROOT 所需要的。前面提到了 `SignalHandler` 比较特殊，继承自 `RQ_Object`，所以需要用这样的方式来链接动态库。
    1.  `online_template_dict` 是一个新的目标，本质是代码生成， `SignalHandler`继承自 `RQ_Object` 后被添加了一些新的代码，新的代码会编译出一个动态库，库的名字就是 `online_template_dict`（这一段有一部分是我猜的）
    2.  `${PROJECT_SOURCE_DIR}/include/signal_handler.h` 告诉 cmake `SignalHandler` 在哪里声明和定义
    3.  `MODULE online_template` 告诉 cmake，把 ROOT 生成的 `online_template_dict` 链接到 `online_template` 上
    4.  `LINKDEF ${PROJECT_SOURCE_DIR}/include/linkdef.h` 告诉 cmake 对应的 `linkdef.h` 文件在哪，这一部分应该是和 cint 相关的，我不太了解

实际使用时，只需要简单修改 `online_template` 即可。
