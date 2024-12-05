#ifndef __SIGNAL_HANDLER_H__
#define __SIGNAL_HANDLER_H__

#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>

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
		should_save_ = false;
	}


	/// @brief stop the program
	void Terminate() {
		std::raise(SIGINT);
	}



    /// @brief update refresh status
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


	/// @brief check whether it's needed to save
	/// @returns true if it's needed to save, false otherwise
	///
	bool ShouldSave() {
		if (should_save_) {
			should_save_ = false;
			return true;
		}
		return false;
	}


private:
	bool should_refresh_;
	bool should_save_;
};


#endif	// __SIGNAL_HANDLER_H__