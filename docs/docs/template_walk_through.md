# 用户指南

用户可以从项目中的[模板文件](https://github.com/kinstaky/xia-daq-gui-online/blob/master/template/online_template.cpp) `template/online_template.cpp` 开始应用本项目的框架编写在线程序。如果只是简单的开发，只需要基本的 ROOT 相关的知识，熟练使用 TH1 和 TH2 直方图，熟练使用 `Fill` 函数填充直方图就足够了。在获取方面，只需要知道每一个事件对应于一个通道，而一个通道又对应与探测器的一路即可。

下面将详细解释模板文件中的代码的含义。

## 主要流程

从 `main()` 函数开始了解主要脉络。

```cpp
constexpr char app_name[] = "online_example";
// run number
int run = -1;
// crate ID
int crate = -1;
ParseArguments(argc, argv, app_name, run, crate);


// ROOT multi-thread preparation
ROOT::EnableThreadSafety();
// create ROOT application
TApplication app(app_name, &argc, argv);


// create canvas
TCanvas* canvas = new TCanvas("canvas", "Online", 0, 0, 1200, 600);
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
// update GUI
std::thread update_gui_thread(
    UpdateGui,
    canvas,
    signal_handler.get(),
    histograms
);


// setup moduel information
std::vector<int> module_sampling_rate = {100, 100, 100, 100};
std::vector<int> group_index = {0, 0, 0, 0};
OnlineDataReceiver receiver(
    app_name,
    "AppName", run, crate,
    module_sampling_rate, group_index
);
while (receiver.Alive()) {
    for (
        std::vector<DecodeEvent> *event = receiver.ReceiveEvent(1000);
        event;
        event = receiver.ReceiveEvent(1000)
    ) {
        FillOnlineGraph(*event, hist1, hist2);
    }
}

// wait for thread
update_gui_thread.join();
```

上面的 `main()` 函数的代码，可以主要分为 5 块。

1. 1-6 行的输入参数处理
2. 9-25 行的 ROOT 相关的准备
3. 29-40 行的用户输入响应
4. 43-59 行的读取数据并处理
5. 62 行程序结束清理

在线程序首先读入输入参数，确定需要处理的是哪个 run 哪个机箱的数据。然后做一些 ROOT 相关的准备，准备好在线需要的直方图。用户输入的响应主要是两部分，一是按照设定的帧率刷新直方图，二是对键盘鼠标输入的响应。核心的部分是从共享内存中读取数据，然后根据用户编写的算法进行实质的在线分析，并填到直方图中。这部分也是经常做数据分析的人所擅长的。最后是清理一下程序中的东西，并优雅地结束程序。

如果只是想要简单地开发基本能用的在线程序，实际上只需要关注第 4 部分数据处理相关的就足够了。下面将详细解释每一部分的工作流程。

## 输入参数处理

为什么需要处理输入参数（arguments）？考虑到实验的过程中，在线处理的对象是变化的，比如说 run 是变化的，所以需要指定参数来让在线程序知道应该处理哪些数据。当然，在程序设计中，目前只是考虑到了两个参数，run 序号和机箱序号。换句话说，用户可以选择在线处理特定的 run 和特定机箱的数据。所以在第 3 行和第 5 行初始化了 `run` 和 `crate` 参数，进行参数处理后会给两者对应的值。

当然，这里的在线处理是实时读取共享内存内的数据的，也就是说是读取的正在运行的 run 的数据。所以虽然可以选择不同的 run 的数据，但是实验中同一时刻只有一个 run 可选，就是正在运行的 run。另一方面，实验中预设是一台电脑对应一个机箱，在线程序通过共享内存只能读取对应机箱的数据。总而言之，表面上有得选，实际上没得选。

回归正题，在程序实现中，输入参数处理依赖于函数 `ParseArguemnts()`。该函数从 `argc` 和 `argv` 读入参数信息，并读取用户设置的程序名 `app_name`，最后返回对应的 `run` 和 `crate`。

```cpp
void ParseArguments(
	int argc,
	char **argv,
	const char *app_name,
	int &run,
	int &crate
) {
	cxxopts::Options options(app_name, "Example of DAQ online simulation");
	options.add_options()
		("h,help", "Print this help information") // a bool parameter
		("r,run", "run number", cxxopts::value<int>())
		("c,crate", "crate number", cxxopts::value<int>())
	;
	try {
		auto result = options.parse(argc, argv);
		if (result.count("help")) {
			std::cout << options.help() << std::endl;
			exit(0);
		}
		// get run number and crate ID
		run = result["run"].as<int>();
		crate = result["crate"].as<int>();
	} catch (cxxopts::exceptions::exception &e) {
		std::cerr << "[Error]: " << e.what() << "\n";
		std::cout << options.help() << std::endl;
		exit(-1);
	}
}
```

在处理 `argc` 和 `argv` 时，主要是调用了 [cxxopts](https://github.com/jarro2783/cxxopts)，具体用法可以参见其说明文档，这里不详细展开了。

简单来说，第 10 行设置了选项 `-h`和 `--help`，第 16-19 行检查了用户是否输入了这两个帮助选项，如果是的话就会输出帮助信息并退出程序。

第 11 行设置了 `-r` 和 `--run` 选项，要求后面跟着一个整数，并在 21 行读取该值并对输出参数 `run` 赋值。第 12 行和 22 行同理，只是对象变成了机箱序号。

第 15 行是尝试解析用户输入的参数，解析成功才有后面的 16-22 行的读取参数。如果解析失败，就会抛出异常，并跳转到 23-26 行的异常处理，这里的处理方法是不处理，直接报错并退出。毕竟用户输入都是错的，程序也不知道应该做什么。

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

第 8 行到 19行就是常规的创建画布并画图。虽然一般来说，都是把直方图填充完了再画图，这里只需要画空的直方图即可。这个程序的逻辑是后续读取在线数据后再填入直方图并更新画布。

## 用户输入响应

用户输入的响应包含两个部分

1. 对鼠标和键盘输入的响应。

2. 持续更新图像

### 对鼠标和键盘输入的响应

鼠标和键盘的输入，目前只考虑了两种情形

1. 点击窗口的关闭按钮终止在线程序
2. 按 **r** 或者 **F5** 重置图像

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

第 6-10 行，将窗口的关闭按钮和 `SignalHandler`的 `Terminate()` 函数关联起来，再用户点击关闭按钮同时，会调用`Terminate()`函数。该函数做两件事，第一是产生一个中断信号，类似于按 Ctrl+C；第二是告诉 ROOT 可以终止程序了。产生中断信号是为了让管理共享内存的 iceoryx 停止工作，这个会在第四部分再次提到。

```cpp
void Terminate() {
    std::raise(SIGINT);
    gApplication->Terminate();
}
```

第 11-16 行，将 ROOT 自身的处理键盘和鼠标输入的函数 `ProcessedEvent`和 `SignalHandler::Refresh` 函数关联起来，也就是说每次用户点击鼠标或者敲键盘时，都会调用 `Refresh` 函数并给一些输入信息。`Refresh` 函数在发现输入信息是 **F5** 或者 **r** 时就会设置 `should_refresh_` 为真，这里不直接重置图像是因为 `SignalHandler` 本身并不知道应该重置哪些图像，而想要让它知道就要传递更多的参数，这样就不够灵活了。而这里设置一个变量给外面的函数或者类判断是否需要重置，就可以在这个类外面完成重置的事情。所谓的外面，其实就是指的用户编写的在线程序。

```cpp
void Refresh(int event, int x, int y, TObject*) {
    if (event == 24 && x == 0 && y == 4148) {
        should_refresh_ = true;
    } else if (event == 24 && x == 114 && y == 114) {
        should_refresh_ = true;
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
	const std::vector<TH1*> &histograms
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
		gSystem->ProcessEvents();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000/fresh_rate));
	}
}
```

这个函数主体是一个循环，终止条件是用户按下 Ctrl+C。前面提到了用户点击关闭按钮时，会触发`SignalHandler::Terminate`函数并调用 `raise(SIGINT)` 函数，这同样会产生一个中断信号，和按下 Ctrl+C 是一样。换句话说，这个循环的终止条件是用户按下 Ctrl+C 或者点击窗口右上角的关闭按钮。

循环中的 7-11 行是遍历画布中的所有 pad，并更新图像，常规的 ROOT 代码。

第 12-16 行是问一下 `SignalHandler` 有没有收到重置图像的指令，有的话就重置所有的图像。因为需要知道要重置哪些图像，所以需要传入参数`const std::vector<TH1*> &histograms`，这里需要传入所有的需要重置的图像的指针。所以可以在 `main()`函数代码中的 31-33 行看到需要构建一个直方图的数组。

第 17 行是 ROOT 周期性的刷新，可以简单理解成刷新窗口。

第 18 行是休眠，根据设置的刷新率休眠一定时间。注意到这里以 us 为单位，所以刷新率不能高于 1000 Hz，当然，实际使用时，10 Hz 以下的刷新率就绰绰有余了。

## 读取内存并处理数据

详见[上手指南](getting_started.md)。

## 清理

在第三部分的用户输入响应中，我们创建了一个线程 `gui_thread` 来持续地更新图像并处理用户输入，所以在程序结束时，我们也要回收这个线程的资源，不然程序结束后显示 break。虽然这个 break 只会在程序结束后产生，理论上并不会造成任何影响，但是不够优雅。所以调用

```cpp
update_gui_thread.join();
```

来等待线程结束并回收。

顺便一提，如果程序中有用到 `new` 或者 `malloc` 来申请内存，也应该在这一部分释放内存。



## 用 cmake 构建
