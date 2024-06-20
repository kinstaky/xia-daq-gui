#include "include/termination-handler.h"

#include <csignal>
#include <iostream>

#include <TApplication.h>


TerminationHandler::TerminationHandler() {
}

void TerminationHandler::Terminate() {
    std::raise(SIGINT);
    gApplication->Terminate();
}