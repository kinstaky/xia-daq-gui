#include "include/termination_handler.h"

#include <csignal>
#include <iostream>

#include <TApplication.h>


// TerminationHandler::TerminationHandler(TH1F *h1,TH2F *h2) {
//     rh1 = h1;
//     rh2 = h2;
// }

void TerminationHandler::Terminate() {
    std::raise(SIGINT);
    gApplication->Terminate();
}

// void TerminationHandler::Refresh(Int_t event,Int_t x,Int_t y,TObject* select){
// 	if(event==24 && x==0 && y==4148){//f5
// 		std::cout << "Refresh the histogram.\n";
// 		rh1->Reset();
// 		rh2->Reset();
// 	}
//     // if(event==24 && x==0 && y==4149){//f6
//     //     std::cout<<"Refresh the histogram"<<std::endl;
//     //     rh1->Fill(1);
//     //     rh2->Fill(1,1);
//     //  }
// //    std::cout<<event<<"\t"<<x<<"\t"<<y<<"\t"<<"\t"<<select<<std::endl;
// }