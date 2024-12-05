#include <iostream>
#include <memory>
#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <memory>

#include <TF1.h>
#include <TApplication.h>
#include <TCanvas.h>
#include <TRootCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TSystem.h>
#include <TROOT.h>

#include <iceoryx_hoofs/posix_wrapper/signal_watcher.hpp>

#include "include/signal_handler.h"
#include "include/online_data_receiver.h"


// GUI fresh rate(FPS), in Hz
constexpr int fresh_rate = 10;
// total number of graphs in online
constexpr int graph_num = 2;
// window width in pixel
int window_width = 1200;
// window height in pixel
int window_height = 600;
// correlation window, in nanoseconds
int64_t time_window = 1000;


/// @brief update ROOT GUI with specific FPS, read keyboard and refresh histograms
/// @param[in] canvas pointer to main ROOT canvas
/// @param[in] handler pointer to signal handler
/// @param[in] histograms vector of pointers to histograms
///
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


void FillOnlineGraph(
	const std::vector<DecodeEvent> &decode_event,
	TH1F &hist1,
	TH2F &hist2
) {
	// write your fill graph method here
}


/// @brief handle user input signal
/// @param[in] canvas pointer to canvas for drawing
/// @returns pointer to SignalHandler
///
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


int main(int argc, char **argv) {
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
	// update GUI
	std::thread update_gui_thread(
		UpdateGui,
		canvas,
		signal_handler.get(),
		histograms
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

	return 0;
}