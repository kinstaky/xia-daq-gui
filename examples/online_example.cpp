#include <iostream>
#include <memory>
#include <string>
#include <map>
#include <thread>
#include <mutex>

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
#include "external/cxxopts.hpp"
#include "examples/alpha_source_dssd_global.h"

// GUI fresh rate(FPS), in Hz
int fresh_rate = 10;

void UpdateGui(
	TCanvas *canvas,
	SignalHandler *handler,
	const std::vector<TH1*> &histograms
) {
	while (!iox::posix::hasTerminationRequested()) {
		for (int i = 1; i < 6; ++i) {
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
	double last_time,
	TH2F &hist_strip,
	TH1F &hist_energy,
	TH1F &hist_energy_difference,
	TH1F &hist_interval,
	TH1F &hist_time_difference,
	double &current_time
) {
	// can't find enough events
	if (decode_event.size() != 2) return;

	int front_strip = -1;
	int back_strip = -1;
	double front_energy, back_energy;
	double front_time, back_time;
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

	// check strips
	if (front_strip < 0 || back_strip < 0) return;

	hist_energy_difference.Fill(front_energy - back_energy);

	// check front-back energy correlation
	if (fabs(front_energy - back_energy) > 0.5) return;

	hist_strip.Fill(front_strip, back_strip);
	hist_energy.Fill(front_energy);
	hist_interval.Fill(front_time - last_time);
	hist_time_difference.Fill(front_time - back_time);
	current_time = front_time;
}


int main(int argc, char **argv) {

	constexpr char app_name[] = "online_example";
	cxxopts::Options options(app_name, "Example of DAQ online simulation");
	options.add_options()
		("h,help", "Print this help information") // a bool parameter
		("r,run", "run number", cxxopts::value<int>())
		("c,crate", "crate number", cxxopts::value<int>())
	;
	// run number
	int run = -1;
	// crate ID
	int crate = -1;
	try {
		auto result = options.parse(argc, argv);
		if (result.count("help")) {
			std::cout << options.help() << std::endl;
			return 0;
		}
		// get run number and crate ID
		run = result["run"].as<int>();
		crate = result["crate"].as<int>();
	} catch (cxxopts::exceptions::exception &e) {
		std::cerr << "[Error]: " << e.what() << "\n";
		std::cout << options.help() << std::endl;
		return -1;
	}

	std::vector<int> module_sampling_rate = {100, 100, 100, 100};
	std::vector<int> group_index = {0, 0, 0, 0};
	OnlineDataReceiver receiver(
		app_name,
		"ExampleSimulateOnline", run, crate,
		module_sampling_rate, group_index
	);

	// ROOT multi-thread preparation
	ROOT::EnableThreadSafety();
	// create ROOT application
	TApplication app(app_name, &argc, argv);
	// create canvas
	TCanvas* canvas = new TCanvas("canvas", "Online", 0, 0, 1200, 600);
	// create histograms
	TH2F hist_strip("hs", "Pixel on DSSD", 32, 0, 32, 32, 0, 32);
	TH1F hist_energy("he", "energy", 100, 5, 6);
	TH1F hist_energy_difference("hde", "energy difference", 100, -1, 1);
	TH1F hist_interval("hit", "time interval", 1000, 0, 1000000);
	TH1F hist_time_difference("ht", "time difference of two sides", 100, -100, 100);
	// draw
	canvas->Divide(3, 2);
	canvas->cd(1);
	hist_strip.Draw("colz");
	canvas->cd(2);
	hist_energy.Draw();
	canvas->cd(3);
	hist_energy_difference.Draw();
	canvas->cd(4);
	hist_interval.Draw();
	canvas->cd(5);
	hist_time_difference.Draw();

	// histograms
	std::vector<TH1*> histograms = {
		&hist_strip,
		&hist_energy,
		&hist_energy_difference,
        &hist_interval,
        &hist_time_difference
	};

	// handle termination
	SignalHandler *signal_handler = new SignalHandler;

	// update GUI
	std::thread update_gui_thread(UpdateGui, canvas, signal_handler, histograms);


	// connect close window and terminate program
	TRootCanvas *rc = (TRootCanvas*)canvas->GetCanvasImp();
	rc->Connect(
		"CloseWindow()",
		"SignalHandler", signal_handler, "Terminate()"
	);
	canvas->Connect(
		"ProcessedEvent(Int_t, Int_t, Int_t, TObject*)",
		"SignalHandler",
		signal_handler,
		"Refresh(Int_t, Int_t, Int_t, TObject*)"
	);

	// time of last event
	double last_time;

	while (receiver.Alive()) {
		for (
			std::vector<DecodeEvent> *event = receiver.ReceiveEvent(1000);
			event;
			event = receiver.ReceiveEvent(1000)
		) {
			FillOnlineGraph(
				*event, last_time,
				hist_strip, hist_energy, hist_energy_difference,
				hist_interval, hist_time_difference,
				last_time
			);
		}
	}

	// wait for thread
	update_gui_thread.join();

	return 0;
}