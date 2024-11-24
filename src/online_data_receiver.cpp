#include "include/online_data_receiver.h"

#include <string>
#include <iostream>

OnlineDataReceiver::OnlineDataReceiver(
	const char *app_name,
	const char *service_name,
	int run,
	int crate,
	const std::vector<int> &module_sampling_rate,
	const std::vector<int> &group_index
) {
	char name[32];
	strcpy(name, app_name);
	// create runtime
	iox::runtime::PoshRuntime::initRuntime(name);

	// get module information
	module_num_ = module_sampling_rate.size();
	sampling_rate_ = module_sampling_rate;

	// subscriber options
	iox_sub_options_t options;
    iox_sub_options_init(&options);
    options.queueCapacity = 40U;
    options.historyRequest = 5U;
	options.queueFullPolicy = QueueFullPolicy_DISCARD_OLDEST_DATA;
    options.nodeName = "online-node";

	// initialize subscribers
	std::string run_name = "run" + std::to_string(run);
	for (size_t i = 0; i < module_num_; ++i) {
		std::string module_name =
			"c" + std::to_string(crate) + "m" + std::to_string(i);
		subscriber_[i] = iox_sub_init(
			subscriber_storage_+i,
			service_name, run_name.c_str(), module_name.c_str(),
			&options
		);
	}

	// initialize payload
	for (size_t i = 0; i < module_num_; ++i) {
		user_payload_[i] = nullptr;
		header_[i] = nullptr;
		packet_[i] = nullptr;
	}

	has_taken_ = false;

	for (size_t i = 0; i < module_num_; ++i) {
		first_events_[i].used = true;
	}


	// intialize group information
	for (size_t i = 0; i < module_num_; ++i) {
		group_index_[i] = group_index[i];
		group_info_[i].size = 0;
		group_info_[i].valid_packets = 0;
		group_info_[i].expect_id = 0;
	}
	for (size_t i = 0; i < module_num_; ++i) {
		if (group_index[i] >= 0) ++group_info_[group_index[i]].size;
	}
}


std::vector<DecodeEvent>* OnlineDataReceiver::ReceiveEvent(
	const int64_t window
) {
	if (!has_taken_) {
		for (size_t i = 0; i < module_num_; ++i) {
			if (user_payload_[i]) continue;
			// take
			enum iox_ChunkReceiveResult take_result = iox_sub_take_chunk(
				subscriber_[i], user_payload_+i
			);
			// no available chunk, continue
			if (take_result == ChunkReceiveResult_NO_CHUNK_AVAILABLE) continue;

			// take chunk error
			if (take_result != ChunkReceiveResult_SUCCESS) {
				std::cerr << "Warning: Take payload failed: "
					<< take_result << "\n";
				continue;
			}

			// get header
			header_[i] = (const PacketHeader*)(
				iox_chunk_header_to_user_header_const(
					iox_chunk_header_from_user_payload_const(user_payload_[i])
				)
			);
			// check packet id
			uint64_t &expected_id = group_info_[group_index_[i]].expect_id;
			if (header_[i]->id > expected_id) {
				// packet id larger than expected, expected id is out dated
				// update expected id and reset valid packet number to 1
				expected_id = header_[i]->id;
				group_info_[group_index_[i]].valid_packets = 1;
				// release out dated packet
				for (size_t j = 0; j < i; ++j) {
					// ignore other group
					if (group_index_[j] != group_index_[i]) continue;
					// ignore empty chunk
					if (!user_payload_[j]) continue;
					// ignore valid packet
					if (header_[j]->id >= expected_id) continue;
					// release out dated packet
					iox_sub_release_chunk(subscriber_[j], user_payload_[j]);
					user_payload_[j] = nullptr;
					header_[j] = nullptr;
					packet_[j] = nullptr;
				}
			} else if (header_[i]->id < expected_id) {
				// packet id smaller than expected, packet is out dated, release it
				iox_sub_release_chunk(subscriber_[i], user_payload_[i]);
				user_payload_[i] = nullptr;
				header_[i] = nullptr;
				packet_[i] = nullptr;
				continue;
			} else {
				// packet id is equal to expected id
				++group_info_[group_index_[i]].valid_packets;
			}
			packet_[i] = (const DaqPacket*)user_payload_[i];
			first_events_[i].used = true;
			decode_offset_[i] = 0;
		}
	}

	// not get enough packet
	if (group_info_[0].valid_packets != group_info_[0].size) {
		return nullptr;
	}

if (!has_taken_) {
	for (int m = 0; m < 4; ++m) {
		for (int j = 0; j < 4; ++j) {
			std::cout
				<< "Module " << m
				<< "  timestamp low " << packet_[m]->data[4*j+1]
				<< "  timestamp high " << (packet_[m]->data[4*j+2] & 0xffff)
				<< ", cfd " << ((packet_[m]->data[4*j+2] >> 16) & 0xffff) << "\n";
		}
	}
}

	has_taken_ = true;


	// timestamps of first events
	int64_t ref_timestamp;
	// initialize
	event_.clear();

	bool finish = false;
	while (!finish) {
		finish = true;
		// get first events
		for (size_t i = 0; i < module_num_; ++i) {
			if (
				first_events_[i].used
				&& decode_offset_[i]+sizeof(DataHeader)/4 < header_[i]->length
			) {
				Decode(
					packet_[i]->data,
					decode_offset_[i],
					sampling_rate_[i],
					first_events_[i]
				);
			}
		}

		// check data
		bool has_data = false;
		for (size_t i = 0; i < module_num_; ++i) {
			if (
				decode_offset_[i]+sizeof(DataHeader)/4 < header_[i]->length
				|| !first_events_[i].used
			) {
				has_data = true;
			}
		}
		if (!has_data) {
			has_taken_ = false;
			// clean
			for (size_t i = 0; i < module_num_; ++i) {
				iox_sub_release_chunk(subscriber_[i], user_payload_[i]);
				user_payload_[i] = nullptr;
				header_[i] = nullptr;
				packet_[i] = nullptr;
			}
			group_info_[0].valid_packets = 0;
			++group_info_[0].expect_id;
			return nullptr;
		}

		if (event_.empty()) {
			// new event, find the minimum timestamp
			// minimum timestamp
			int64_t min_ts = 0x7fff'ffff'ffff'ffff;
			// module with minimum timestamp
			size_t min_ts_index = 16;
			for (size_t i = 0; i < module_num_; ++i) {
				if (first_events_[i].used) continue;
				if (first_events_[i].timestamp < min_ts) {
					min_ts = first_events_[i].timestamp;
					min_ts_index = i;
				}
			}
			if (min_ts_index == 16) {
				std::cerr << "Error: Could not find minimum timestamp.\n";
				break;
			}
			finish = false;
			first_events_[min_ts_index].used = true;
			ref_timestamp = first_events_[min_ts_index].timestamp;
			// fill to fundamental event
			event_.push_back(first_events_[min_ts_index]);

		} else {
			for (size_t i = 0; i < module_num_; ++i) {
				if (first_events_[i].used) continue;
				if (
					first_events_[i].timestamp - ref_timestamp > -window
					&& first_events_[i].timestamp - ref_timestamp < window
				) {
					finish = false;
					first_events_[i].used = true;
					event_.push_back(first_events_[i]);
				}
			}
		}
	}

	if (event_.empty()) return nullptr;
	return &event_;
}


bool OnlineDataReceiver::Alive() const {
	return !iox::posix::hasTerminationRequested();
}



void OnlineDataReceiver::Decode(
	const unsigned int *data,
	size_t &offset,
	const int rate,
	DecodeEvent &event
) {
	const DataHeader *header = (const DataHeader*)(data + offset);
	// get event length and increase offset
	int event_length = (header->data[0] >> 17) & 0x3fff;
	offset += event_length;
	// fill information
	event.module = (unsigned short)(((header->data[0] >> 4) & 0xf) - 2);
	event.channel = header->data[0] & 0xf;
	event.energy = header->data[3] & 0xffff;
	// cfd
	event.cfd = 0.0;
	if (rate == 100) {
		bool cfd_force = (header->data[2] >> 31) != 0;
		if (!cfd_force) {
			event.cfd = double((header->data[2] >> 16) & 0x7fff);
			event.cfd = event.cfd / 32768.0 * 10.0;
		}
	} else if (rate == 250) {
		bool cfd_force = (header->data[2] >> 31) != 0;
		if (!cfd_force) {
			unsigned int cfds = (header->data[2] >> 30) & 0x1;
			event.cfd = double((header->data[2] >> 16) & 0x3fff);
			event.cfd = (event.cfd / 16384.0 - cfds) * 4.0;
		}
	} else { 					//500
		unsigned int cfds = (header->data[2] >> 29) & 0x7;
		bool cfd_force = cfds == 7;
		if (!cfd_force) {
			event.cfd = double((header->data[2] >> 16) & 0x1fff);
			event.cfd = (event.cfd / 8192.0 + cfds - 1) * 2.0;
		}
	}
	// timestamp
	event.timestamp = int64_t(header->data[2] & 0xffff) << 32;
	event.timestamp |= header->data[1];
	if (rate == 250) event.timestamp *= 8;
	else event.timestamp *= 10;
	// time
	event.time = double(event.timestamp) + event.cfd;
	// used
	event.used = false;
	return;
}