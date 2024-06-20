#ifndef __EVENT_H__
#define __EVENT_H__

struct NotMapEvent {
	bool used;
	unsigned short slot;
	unsigned short channel;
	unsigned int energy;
	unsigned int cfdft;
	unsigned int cfd;
	double time;
};


struct DssdMapEvent {
	unsigned short side;
	unsigned short strip;
	unsigned int energy;
};


struct OnlineTestFundamentalEvent {
	int num;
	unsigned short slot[16];
	unsigned short channel[16];
	unsigned int energy[16];
	double time[16];
};

#endif