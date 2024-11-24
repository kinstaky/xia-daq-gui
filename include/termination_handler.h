#ifndef __TERMINATION_HANDLER_H__
#define __TERMINATION_HANDLER_H__

#include <csignal>

#include <RQ_OBJECT.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TApplication.h>


class TerminationHandler {
	RQ_OBJECT("TerminationHandler")
public:
	// TerminationHandler() = default;
	// TerminationHandler(TH1F* h1, TH2F *h2);

	void Terminate() {
		std::raise(SIGINT);
		gApplication->Terminate();
	}
	// void Refresh(Int_t event,Int_t x,Int_t y,TObject* select);

private:
	// TH1F *rh1;
	// TH2F *rh2;
};


#endif	// __TERMINATION_HANDLER_H__