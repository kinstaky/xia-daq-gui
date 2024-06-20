#include <cstdio>
#include <cstdlib>
#include <iostream>

#include <iceoryx_posh/runtime/posh_runtime.hpp>

#include "external/daq/MainFrame.hh"

using namespace std;
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

int main(int argc, char **argv) {
	iox::runtime::PoshRuntime::initRuntime("gddaq-gui-online");

	TApplication theApp("App", &argc, argv);

	// check compiler
	if(sizeof(char) != 1) std::cout<<"sizeof(char) != 1 The current compiler is not suitable for running the program！"<<std::endl; 
	if(sizeof(short) != 2) std::cout<<"sizeof(short) != 2 The current compiler is not suitable for running the program！"<<std::endl; 
	if(sizeof(int) != 4) std::cout<<"sizeof(int) != 4 The current compiler is not suitable for running the program！"<<std::endl;

	MainFrame mainWindow(gClient->GetRoot());

	theApp.Run();

	return 0;
}
