#ifndef __TERMINATION_HANDLER_H__
#define __TERMINATION_HANDLER_H__

#include <RQ_OBJECT.h>

class TerminationHandler {
	RQ_OBJECT("TerminationHandler")
public:
	TerminationHandler();

	void Terminate();
};


#endif	// __TERMINATION_HANDLER_H__