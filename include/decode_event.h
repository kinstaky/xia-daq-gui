#ifndef __DECODE_EVENT_H__
#define __DECODE_EVENT_H__

struct DecodeEvent {
	int module;
	int channel;
	int energy;
	double time;
	int64_t timestamp;
	double cfd;
	bool used;
};

#endif // __DECODE_EVENT_