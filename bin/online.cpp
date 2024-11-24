#include <iostream>
#include <memory>
#include <string>
#include <map>
#include <thread>
#include <mutex>

#include <iceoryx_hoofs/posix_wrapper/signal_watcher.hpp>
#include <iceoryx_posh/runtime/posh_runtime.hpp>

#if defined (__cplusplus)
extern "C" {
#endif
#include <iceoryx_binding_c/chunk.h>
#include <iceoryx_binding_c/subscriber.h>
#if defined (__cplusplus)
}
#endif

#include <TF1.h>
#include <TApplication.h>
#include <TCanvas.h>
#include <TRootCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TSystem.h>
#include <TROOT.h>

#include "include/daq_packet.h"
#include "include/termination_handler.h"
#include "include/event.h"


int gui_tick = 0;
void UpdateGui(TCanvas *canvas) {
	while (!iox::posix::hasTerminationRequested()) {
		gui_tick++;
		if (gui_tick == 10) {
			canvas->cd(1);
			canvas->Update();
			canvas->Pad()->Draw();
			canvas->cd(2);
			canvas->Update();
			canvas->Pad()->Draw();
			gui_tick = 0;
		}
		gSystem->ProcessEvents();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}


/// @brief decode single event and get information
/// @param[in] data data buffer to read
/// @param[inout] offset offset from the begining
/// @param[in] rate sampling rate
/// @param[out] event mapped event
/// @returns timestamp of current event
///
int64_t Decode(
	const unsigned int *data,
	size_t &offset,
	int rate,
	NotMapEvent &event
) {
	const DataHeader *header = (const DataHeader*)(data + offset);
	// get event length and increase offset
	int event_length = (header->data[0] >> 17) & 0x3fff;
// if (event_length % 4 || event_length == 0) {
// 	std::cout << "length: " << event_length << "\n";
// 	exit(EXIT_FAILURE);
// }
	offset += event_length;
	event.slot = (unsigned short)(((header->data[0] >> 4) & 0xf) - 2);
	event.channel = header->data[0] & 0xf;
	event.energy = header->data[3] & 0xffff;
	// cfd
	double cfd = 0.0;
	if (rate == 100) {
		bool cfd_force = (header->data[2] >> 31) != 0;
		if (!cfd_force) {
			cfd = double((header->data[2] >> 16) & 0x7fff);
			cfd = cfd / 32768.0 * 10.0;
		}
	} else if (rate == 250) {
		bool cfd_force = (header->data[2] >> 31) != 0;
		if (!cfd_force) {
			unsigned int cfds = (header->data[2] >> 30) & 0x1;
			cfd = double((header->data[2] >> 16) & 0x3fff);
			cfd = (cfd / 16384.0 - cfds) * 4.0;
		}
	} else { 					//500
		unsigned int cfds = (header->data[2] >> 29) & 0x7;
		bool cfd_force = cfds == 7;
		if (!cfd_force) {
			cfd = double((header->data[2] >> 16) & 0x1fff);
			cfd = (cfd / 8192.0 + cfds - 1) * 2.0;
		}
	}
	// timestamp
	int64_t timestamp = int64_t(header->data[2] & 0xffff) << 32;
	timestamp |= header->data[1];
	timestamp *= 8;
	// time
	event.time = double(timestamp) + cfd;
	return timestamp;
}


/// @brief convert data buffer into match map
/// @tparam MapEvent mapped event type
/// @param[in] data data buffer
/// @param[in] length data array length, in words (4 bytes)
/// @param[out] match_map events in map reference by timstamp
///
template<typename MapEvent>
void DecodeAndMap(
	const unsigned int *data,
	size_t length,
	std::multimap<int64_t, MapEvent> &match_map
) {
	size_t offset = 0;
	MapEvent event;
	// int count = 0;
	while (offset+4 < length) {
		int64_t timestamp = Decode(data, offset, 250, event);
		event.used = false;
		// ++count;
		// if (count < 10) {
		// 	std::cout << count << ", " << event.slot << ", " << event.channel
		// 		<< ", " << event.energy << ", " << event.time << "\n";
		// } else return;
		match_map.insert(std::make_pair(timestamp, event));
	}
}


template<typename MapEvent, typename FundamentalEvent>
void Match(
	std::multimap<int64_t, MapEvent> &map,
	int64_t window,
	void (*fill_fundamental_event)(const std::vector<MapEvent>&, FundamentalEvent&),
	std::vector<FundamentalEvent> &fundamental
) {
	// loop all events
	for (auto iter1 = map.begin(); iter1 != map.end(); ++iter1) {
		if (iter1->second.used) continue;
		// found correlated event
		std::vector<MapEvent> correlated_events;
		// fill to correlated events
		correlated_events.push_back(iter1->second);
		// mark used flag
		iter1->second.used = true;
		// search for events correlated with iter1->second
		for (auto iter2 = std::next(iter1); iter2 != map.end(); ++iter2) {
			// stop if it's out of search window
			if (iter2->first >= iter1->first + window) break;
			// ignore used events
			if (iter2->second.used) continue;
			// fill to correlated events
			correlated_events.push_back(iter2->second);
			// mark as used
			iter2->second.used = true;
		}

		// sort
		// std::sort(
		// 	correlated_events.begin(),
		// 	correlated_events.end(),
		// 	[](const MapEvent &x, const MapEvent &y) {
		// 		return x.energy > y.energy;
		// 	}
		// )

		// fill fndamental event
		FundamentalEvent event;
		fill_fundamental_event(correlated_events, event);
		fundamental.push_back(event);
	}
}


void FillOnlineTestFundamentalEvent(
	const std::vector<NotMapEvent> &correlate_events,
	OnlineTestFundamentalEvent &fundamental
) {
	int &num = fundamental.num;
	fundamental.num = 0;
	for (const auto & event : correlate_events) {
		fundamental.slot[num] = event.slot;
		fundamental.channel[num] = event.channel;
		fundamental.energy[num] = event.energy;
		fundamental.time[num] = event.time;
		++num;
		if (num >= 16) break;
	}
	return;
}


void FillOnineTestGraph(
	const OnlineTestFundamentalEvent &event,
	TH1F *ht,
	TH2F *h2
) {
	int slot0_index = -1;
	int slot1_ch0_index = -1;
	int slot1_ch1_index = -1;
	// found index
	for (int i = 0; i < event.num; ++i) {
		if (event.slot[i] == 0) {
			slot0_index = i;
		} else if (event.slot[i] == 1 && event.channel[i] == 0) {
			slot1_ch0_index = i;
		} else if (event.slot[i] == 1 && event.channel[i] == 1) {
			slot1_ch1_index = i;
		}
	}
	// fill
	if (slot0_index >= 0) {
		if (slot1_ch0_index >= 0) {
			ht->Fill(event.time[slot0_index] - event.time[slot1_ch0_index]);
		}
		if (slot1_ch0_index >= 0 || slot1_ch1_index >= 0) {
			h2->Fill(0.0, 1.0);
		} else {
			h2->Fill(0.0, 0.0);
		}
	}
	if (slot1_ch0_index >= 0) {
		if (slot0_index >= 0) h2->Fill(1.0, 1.0);
		else h2->Fill(1.0, 0.0);
	}
	if (slot1_ch1_index >= 0) {
		if (slot0_index >= 0) h2->Fill(2.0, 1.0);
		else h2->Fill(2.0, 0.0);
	}
}


struct RawData {
	unsigned int *data;
	size_t length;
};

void DecodeMatchFill(
	const RawData* raw_data,
	const int64_t window,
	TH1F *ht,
	TH2F *h2
) {
	// raw event variables
	// has data in raw data
	bool has_data = true;
	// decode offset in raw data
	size_t offsets[2] = {0, 0};

	// mapped event variables
	// first event in each module
	NotMapEvent first_events[2];
	first_events[0].used = true;
	first_events[1].used = true;
	int64_t timestamps[2];

	// fundamental event variables
	// fundamental event
	OnlineTestFundamentalEvent fundamental;
	// initialize
	fundamental.num = 0;
	// event number
	int &num = fundamental.num;
	// fundamental event reference timestamp
	int64_t ref_timestamp;


	// loop all raw data
	while (has_data) {
		// decode and map
		if (
			first_events[0].used
			&& offsets[0]+sizeof(DataHeader) < raw_data[0].length
		) {
			timestamps[0] = Decode(raw_data[0].data, offsets[0], 250, first_events[0]);
			first_events[0].used = false;
		}
		if (
			first_events[1].used
			&& offsets[1]+sizeof(DataHeader) < raw_data[1].length
		) {
			timestamps[1] = Decode(raw_data[1].data, offsets[1], 250, first_events[1]);
			first_events[1].used = false;
		}
// std::cout << "Read event 0: used " << first_events[0].used << ", slot " << first_events[0].slot
// 	<< ", ch " << first_events[0].channel << ", e " << first_events[0].energy
// 	<< ", t " << first_events[0].time << ", ts " << timestamps[0] << "\n";
// std::cout << "Read event 1: used " << first_events[1].used << ", slot " << first_events[1].slot
// 	<< ", ch " << first_events[1].channel << ", e " << first_events[1].energy
// 	<< ", t " << first_events[1].time << ", ts " << timestamps[1] << "\n";

		// map

		// match
		bool found_match_event = false;
		if (num == 0) {
			// new fundamental event, find the minimum timestamp
			// minimum timestamp
			int64_t min_ts = 0x7fff'ffff'ffff'ffff;
			// minimum timestamp module
			size_t min_ts_index = 16;
			for (size_t i = 0; i < 2; ++i) {
				if (first_events[i].used) continue;
				if (timestamps[i] < min_ts) {
					min_ts = timestamps[i];
					min_ts_index = i;
				}
			}
			if (min_ts_index == 16) {
				std::cerr << "Error: Could not find minimum timestamp.\n";
				break;
			}

			found_match_event = true;
			first_events[min_ts_index].used = true;
			ref_timestamp = timestamps[min_ts_index];
			// fill to fundamental event
			fundamental.slot[num] = first_events[min_ts_index].slot;
			fundamental.channel[num] = first_events[min_ts_index].channel;
			fundamental.energy[num] = first_events[min_ts_index].energy;
			fundamental.time[num] = first_events[min_ts_index].time;
			++num;

// std::cout << "Fundamental, num " << num-1 << ", found " << found_match_event
// 	<< ", min ts index " << min_ts_index << ", ref ts " << ref_timestamp
// 	<< ", used " << first_events[min_ts_index].used << ", slot " << fundamental.slot[0]
// 	<< ", ch " << fundamental.channel[0] << ", e " << fundamental.energy[0]
// 	<< ", t " << fundamental.time[0] << "\n";

		} else {
			for (size_t i = 0; i < 2; ++i) {
// std::cout << "Fundamental, num " << num-1 << ", found " << found_match_event
// 	<< ", i " << i << ", ref ts " << ref_timestamp
// 	<< ",  used " << first_events[i].used << "\n";
				if (first_events[i].used) continue;
				if (
					timestamps[i] - ref_timestamp > -window
					&& timestamps[i] - ref_timestamp < window
				) {
					found_match_event = true;
					first_events[i].used = true;
					if (num == 16) continue;
					fundamental.slot[num] = first_events[i].slot;
					fundamental.channel[num] = first_events[i].channel;
					fundamental.energy[num] = first_events[i].energy;
					fundamental.time[num] = first_events[i].time;
					++num;
				}
			}
		}

		// fill
		if (!found_match_event) {
			FillOnineTestGraph(fundamental, ht, h2);
			fundamental.num = 0;
		}

		// check data
		if (
			offsets[0]+4 < raw_data[0].length
			|| offsets[1]+4 < raw_data[1].length
			|| !first_events[0].used
			|| !first_events[1].used
		) {
			has_data = true;
		} else {
			has_data = false;
		}
	}
}


int main(int argc, char **argv) {
	if (argc < 4) {
		std::cout << "Usage: " << argv[0] << " run crate module\n"
			<< "  run       run number\n"
			<< "  crate     crate ID\n"
			<< "  module    module number\n";
		return -1;
	}
	int run = atoi(argv[1]);
	std::string run_name = "run" + std::to_string(run);
	int crate_id = atoi(argv[2]);
	int module_num = atoi(argv[3]);

	// create runtime
	iox::runtime::PoshRuntime::initRuntime("online");

	ROOT::EnableThreadSafety();

	// create ROOT application
	TApplication app("app", &argc, argv);

	// create canvas
	TCanvas* canvas = new TCanvas("c1", "Online", 0, 0, 1200, 600);

	// create histogram
	TH1F *ht = new TH1F("ht", "time difference of two channels", 1000, -50'000, 50'000);
	TH2F *h2 = new TH2F("h2", "correlated counts", 4, 0, 4, 2, 0, 2);
	canvas->Divide(2, 1);
	canvas->cd(1);
	ht->Draw();
	canvas->cd(2);
	h2->Draw("text colz");
	// update GUI
	std::thread update_gui_thread(UpdateGui, canvas);

	// handle termination
	TerminationHandler *termination_handler = new TerminationHandler;

	// connect close window and terminate program
	TRootCanvas *rc = (TRootCanvas*)canvas->GetCanvasImp();
	rc->Connect(
		"CloseWindow()",
		"TerminationHandler", termination_handler, "Terminate()"
	);

	// //refresh the histogram
	// canvas->Connect(
	// 	"ProcessedEvent(Int_t,Int_t,Int_t,TObject*)",
	// 	"TerminationHandler",
	// 	termination_handler,
	// 	"Refresh(Int_t,Int_t,Int_t,TObject*)"
	// );

	// subscriber options
	iox_sub_options_t options;
    iox_sub_options_init(&options);
    options.queueCapacity = 40U;
    options.historyRequest = 5U;
	options.queueFullPolicy = QueueFullPolicy_DISCARD_OLDEST_DATA;
    options.nodeName = "online-node";
	iox_sub_storage_t subscriber_storage[16];
	iox_sub_t subscriber[16];
	// initialize subscribers
	for (int i = 0; i < module_num; ++i) {
		std::string module_name =
			"c" + std::to_string(crate_id) + "m" + std::to_string(i);
		subscriber[i] = iox_sub_init(
			subscriber_storage+i,
			"DaqPacket", run_name.c_str(), module_name.c_str(),
			&options
		);
	}

	// initialize
	const void *user_payload[16];
	const PacketHeader *header[16];
	const DaqPacket *packet[16];
	for (int i = 0; i < module_num; ++i) {
		user_payload[i] = nullptr;
		header[i] = nullptr;
		packet[i] = nullptr;
	}

	// group index of each packet
	int group_index[16];
	for (int i = 0; i < 16; ++i) group_index[i] = -1;
	group_index[0] = 0;
	group_index[1] = 0;
	// size of each group
	int group_size[16];
	for (int i = 0; i < 16; ++i) group_size[i] = 0;
	for (int i = 0; i < 16; ++i) {
		if (group_index[i] >= 0) group_size[group_index[i]]++;
	}
	// valid number of packet in this group
	int group_valid_packet[16];
	for (int i = 0; i < 16; ++i) group_valid_packet[i] = 0;
	// expected packet of each group
	uint64_t group_expected_packet_id[16];
	for (int i = 0; i < 16; ++i) group_expected_packet_id[i] = 0;

	// auto idle_start = std::chrono::steady_clock::now();

    //! [receive and print data]
	while (!iox::posix::hasTerminationRequested()) {
		for (int i = 0; i < module_num; ++i) {
			// take
			enum iox_ChunkReceiveResult take_result = iox_sub_take_chunk(
				subscriber[i], user_payload+i
			);
			// no available chunk, continue
			if (take_result == ChunkReceiveResult_NO_CHUNK_AVAILABLE) continue;

			// take chunk error
			if (take_result != ChunkReceiveResult_SUCCESS) {
				std::cerr << "Warning: Take payload failed: "
					<< take_result << "\n";
				continue;
			}

			// take chunk success

			// get header
			header[i] = (const PacketHeader*)(
				iox_chunk_header_to_user_header_const(
					iox_chunk_header_from_user_payload_const(user_payload[i])
				)
			);
			// check packet id
			uint64_t &expected_id = group_expected_packet_id[group_index[i]];

			if (header[i]->id > expected_id) {
				// packet id larger than expected, expected id is out dated
				// update expected id and reset valid packet number to 1
				expected_id = header[i]->id;
				group_valid_packet[group_index[i]] = 1;
				// release out dated packet
				for (int j = 0; j < i; ++j) {
					// ignore other group
					if (group_index[j] != group_index[i]) continue;
					// ignore empty chunk
					if (!user_payload[j]) continue;
					// ignore valid packet
					if (header[j]->id >= expected_id) continue;
					// release out dated packet
					iox_sub_release_chunk(subscriber[j], user_payload[j]);
					user_payload[j] = nullptr;
					header[j] = nullptr;
					packet[j] = nullptr;
				}
			} else if (header[i]->id < expected_id) {
				// packet id smaller than expected, packet is out dated, release it
				iox_sub_release_chunk(subscriber[i], user_payload[i]);
				user_payload[i] = nullptr;
				header[i] = nullptr;
				packet[i] = nullptr;
				continue;
			} else {
				// packet id is equal to expected id
				++group_valid_packet[group_index[i]];
			}
			packet[i] = (const DaqPacket*)user_payload[i];
		}


		if (group_valid_packet[0] == group_size[0]) {
std::cout << "id: " << header[0]->id << ", " << header[1]->id << "\n";

			// auto idle_stop = std::chrono::steady_clock::now();
			// std::cout << "Idle time "
			// 	<< std::chrono::duration_cast<std::chrono::microseconds>(idle_stop-idle_start).count() << "\n";

			// auto start = std::chrono::steady_clock::now();

			// quick analysis
			RawData raw_data[2];
			raw_data[0].data = (unsigned int*)(packet[0]->data);
			raw_data[0].length = header[0]->length;
			raw_data[1].data = (unsigned int*)(packet[1]->data);
			raw_data[1].length = header[1]->length;
			DecodeMatchFill(raw_data, 50'000, ht, h2);

			// auto end = std::chrono::steady_clock::now();
			// std::cout << "Quick analysis time "
			// 	<< std::chrono::duration_cast<std::chrono::microseconds>(end-start).count() << " us\n";


			// // auto start = std::chrono::steady_clock::now();
			// // full analysis
			// std::multimap<int64_t, NotMapEvent> match_map;
			// // decode
			// DecodeAndMap(packet[0]->data, header[0]->length, match_map);
			// DecodeAndMap(packet[1]->data, header[1]->length, match_map);
			// // fundamental events
			// std::vector<OnlineTestFundamentalEvent> fundamental;
			// // insert into map and match
			// Match(match_map, 50'000, FillOnlineTestFundamentalEvent, fundamental);
			// // print to graph or histogram
			// for (const auto &event : fundamental) {
			// 	FillOnineTestGraph(event, ht, h2);
			// }

			// auto end = std::chrono::steady_clock::now();
			// std::cout << "Full analysis time "
			// 	<< std::chrono::duration_cast<std::chrono::microseconds>(end-start).count() << " us\n";
			// idle_start = std::chrono::steady_clock::now();

			// clean
			iox_sub_release_chunk(subscriber[0], user_payload[0]);
			iox_sub_release_chunk(subscriber[1], user_payload[1]);
			user_payload[0] = nullptr;
			user_payload[1] = nullptr;
			header[0] = nullptr;
			header[1] = nullptr;
			packet[0] = nullptr;
			packet[1] = nullptr;

			group_valid_packet[0] = 0;
			++group_expected_packet_id[0];
		}
    }

	update_gui_thread.join();

	for (int i = 0; i < module_num; ++i) {
	    iox_sub_deinit(subscriber[i]);
	}

	return (EXIT_SUCCESS);
}