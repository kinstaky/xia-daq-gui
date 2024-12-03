#ifndef __SIGNAL_HANDLER_H__
#define __SIGNAL_HANDLER_H__

#include <iostream>
#include <csignal>

#include <RQ_OBJECT.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TApplication.h>


class SignalHandler {
	RQ_OBJECT("SIGNALHandler")
public:

	/// @brief constructor
	SignalHandler() {
		should_refresh_ = false;
	}


	/// @brief stop the program
	void Terminate() {
		std::raise(SIGINT);
		gApplication->Terminate();
	}



    /// @brief update refresh status
	void Refresh(
		int event,
		int x,
		int y,
		TObject*
	) {
		if (event == 24 && x == 0 && y == 4148) {
			should_refresh_ = true;
		} else if (event == 24 && x == 114 && y == 114) {
			should_refresh_ = true;
		}
	}

	/// @brief check whether it's needed to refresh
	/// @returns true if it's needed to refresh, false otherwise
	///
	bool ShouldRefresh() {
		if (should_refresh_) {
			should_refresh_ = false;
			return true;
		}
		return false;
	}

private:
	bool should_refresh_;
};


#endif	// __SIGNAL_HANDLER_H__