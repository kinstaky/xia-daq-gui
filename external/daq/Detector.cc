#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <iostream>
#include <cstdio>
#include <iomanip>
#include <filesystem>

#if defined (__cplusplus)
extern "C" {
#endif
#include <iceoryx_binding_c/chunk.h>
#if defined (__cplusplus)
}
#endif
#include <iceoryx_posh/runtime/posh_runtime.hpp>

#include "external/pixie/app/pixie16app_defs.h"
#include "external/pixie/app/pixie16app_export.h"

#include "Detector.hh"
#include "Global.hh"
#include "wuReadData.hh"

using namespace std;
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......


Detector::Detector(int mode)
: shmfd(-1)
, fonline(0) {

	OfflineMode = (unsigned short)mode;

	for(int i = 0; i < PRESET_MAX_MODULES; i++) {
		fsave[i] = NULL;
		buffer_top_[i] = 0;
		FILESIZE[i] = 0;
		user_payload_[i] = nullptr;
		header_[i] = nullptr;
		packet_[i] = nullptr;
		packet_id_[i] = 0;
	}

	// publisher options
	iox_pub_options_init(&publisher_options_);
	publisher_options_.historyCapacity = 5U;
	publisher_options_.subscriberTooSlowPolicy =
		ConsumerTooSlowPolicy_DISCARD_OLDEST_DATA;
	publisher_options_.nodeName = "daq-gui-node";

	PXISlotMap = new unsigned short[PRESET_MAX_MODULES];
	moduleslot = new std::vector<unsigned short>;
	modulesamplingrate = new std::vector<unsigned short>;
	modulebits = new std::vector<unsigned short>;

	if(shmfd < 0) OpenSharedMemory();
	shmid1 = 0;
	shmid2 = 0;


#ifdef DECODERONLINE
	InitDecoderOnline();
#endif

}

Detector::~Detector()
{
	if(moduleslot) delete moduleslot;
	if(modulesamplingrate) delete modulesamplingrate;
	if(modulebits) delete modulebits;


	std::cout<<"detector is deleted!"<<std::endl;
	ExitSystem();
}

bool Detector::ReadConfigFile(char *config)
{
	crateid = wuReadData::ReadValue<unsigned int>("CrateID", config);

	moduleslot->clear();
	modulesamplingrate->clear();
	modulebits->clear();
	wuReadData::ReadVector("ModuleSlot", config, moduleslot);
	wuReadData::ReadVector("ModuleSampingRate", config, modulesamplingrate);
	wuReadData::ReadVector("ModuleBits", config, modulebits);

	wuReadData::ReadVector("ModuleGroupIndex", config, &group_index_, true);
	wuReadData::ReadVector("ModuleAlignment", config, &module_align_, true);
	if (group_index_.size() != modulesamplingrate->size()) {
		std::cerr << "[Error] ModuleGroupIndex size != ModuleSamplingRate size.\n";
		return false;
	}
	if (module_align_.size() != modulesamplingrate->size()) {
		std::cerr << "[Error] ModuleAlignment size != ModuleSamplingRate size.\n";
		return false;
	}

#ifdef FIRMWARE100M12BIT
	File100M12bit_sys = wuReadData::ReadValue<std::string>("100M12bit_sys", config);
	File100M12bit_fip = wuReadData::ReadValue<std::string>("100M12bit_fip", config);
	File100M12bit_dspldr = wuReadData::ReadValue<std::string>("100M12bit_dspldr", config);
	File100M12bit_dsplst = wuReadData::ReadValue<std::string>("100M12bit_dsplst", config);
	File100M12bit_dspvar = wuReadData::ReadValue<std::string>("100M12bit_dspvar", config);
#endif

#ifdef FIRMWARE100M14BIT
	File100M14bit_sys = wuReadData::ReadValue<std::string>("100M14bit_sys", config);
	File100M14bit_fip = wuReadData::ReadValue<std::string>("100M14bit_fip", config);
	File100M14bit_dspldr = wuReadData::ReadValue<std::string>("100M14bit_dspldr", config);
	File100M14bit_dsplst = wuReadData::ReadValue<std::string>("100M14bit_dsplst", config);
	File100M14bit_dspvar = wuReadData::ReadValue<std::string>("100M14bit_dspvar", config);
#endif


#ifdef FIRMWARE250M12BIT
	File250M12bit_sys = wuReadData::ReadValue<std::string>("250M12bit_sys", config);
	File250M12bit_fip = wuReadData::ReadValue<std::string>("250M12bit_fip", config);
	File250M12bit_dspldr = wuReadData::ReadValue<std::string>("250M12bit_dspldr", config);
	File250M12bit_dsplst = wuReadData::ReadValue<std::string>("250M12bit_dsplst", config);
	File250M12bit_dspvar = wuReadData::ReadValue<std::string>("250M12bit_dspvar", config);
#endif

#ifdef FIRMWARE250M14BIT
	File250M14bit_sys = wuReadData::ReadValue<std::string>("250M14bit_sys", config);
	File250M14bit_fip = wuReadData::ReadValue<std::string>("250M14bit_fip", config);
	File250M14bit_dspldr = wuReadData::ReadValue<std::string>("250M14bit_dspldr", config);
	File250M14bit_dsplst = wuReadData::ReadValue<std::string>("250M14bit_dsplst", config);
	File250M14bit_dspvar = wuReadData::ReadValue<std::string>("250M14bit_dspvar", config);
#endif

#ifdef FIRMWARE250M16BIT
	File250M16bit_sys = wuReadData::ReadValue<std::string>("250M16bit_sys", config);
	File250M16bit_fip = wuReadData::ReadValue<std::string>("250M16bit_fip", config);
	File250M16bit_dspldr = wuReadData::ReadValue<std::string>("250M16bit_dspldr", config);
	File250M16bit_dsplst = wuReadData::ReadValue<std::string>("250M16bit_dsplst", config);
	File250M16bit_dspvar = wuReadData::ReadValue<std::string>("250M16bit_dspvar", config);
#endif

#ifdef FIRMWARE500M12BIT
	File500M12bit_sys = wuReadData::ReadValue<std::string>("500M12bit_sys", config);
	File500M12bit_fip = wuReadData::ReadValue<std::string>("500M12bit_fip", config);
	File500M12bit_dspldr = wuReadData::ReadValue<std::string>("500M12bit_dspldr", config);
	File500M12bit_dsplst = wuReadData::ReadValue<std::string>("500M12bit_dsplst", config);
	File500M12bit_dspvar = wuReadData::ReadValue<std::string>("500M12bit_dspvar", config);
#endif

#ifdef FIRMWARE500M14BIT
	File500M14bit_sys = wuReadData::ReadValue<std::string>("500M14bit_sys", config);
	File500M14bit_fip = wuReadData::ReadValue<std::string>("500M14bit_fip", config);
	File500M14bit_dspldr = wuReadData::ReadValue<std::string>("500M14bit_dspldr", config);
	File500M14bit_dsplst = wuReadData::ReadValue<std::string>("500M14bit_dsplst", config);
	File500M14bit_dspvar = wuReadData::ReadValue<std::string>("500M14bit_dspvar", config);
#endif


	FileSettingPars = wuReadData::ReadValue<std::string>("SettingPars", config);

	// std::cout<<File100M14bit_sys<<std::endl;
	// std::cout<<File100M14bit_fip<<std::endl;
	// std::cout<<File100M14bit_dspldr<<std::endl;
	// std::cout<<File100M14bit_dsplst<<std::endl;
	// std::cout<<File100M14bit_dspvar<<std::endl;
	// std::cout<<File250M14bit_sys<<std::endl;
	// std::cout<<File250M14bit_fip<<std::endl;
	// std::cout<<File250M14bit_dspldr<<std::endl;
	// std::cout<<File250M14bit_dsplst<<std::endl;
	// std::cout<<File250M14bit_dspvar<<std::endl;
	// std::cout<<FileSettingPars<<std::endl;

	return true;
}

bool Detector::BootSystem()
{
	if (!ReadConfigFile()) return false;

	if(OfflineMode == 0)
		{
			NumModules = moduleslot->size();
			std::cout<<"---------- Init System Mode: Online ----------"<<std::endl;
			std::cout<<"Module Number: "<< NumModules<<std::endl;
			for (unsigned int i = 0; i < moduleslot->size(); ++i)
	{
		std::cout<<"Module "<<i<<" in slot "<<moduleslot->at(i)<<std::endl;
	}

		}

		

	else
		{
			cout<<"---------- Init System Mode: Offline ----------"<<endl;
			if(modulesamplingrate->size() != modulebits->size())
	{
		std::cout<<"The number of ModuleSampingRate is not equal to ModuleBits"<<std::endl;
		return false;
	}
			NumModules = modulesamplingrate->size();
			std::cout<<"Module Number: "<< NumModules <<std::endl;
			for (unsigned int i = 0; i < modulesamplingrate->size(); ++i)
	{
		std::cout<<"module "<<i<<"  sampling rate "<<modulesamplingrate->at(i)<<"  bits "<<modulebits->at(i)<<std::endl;
	}
		}

	for(unsigned short k = 0; k < NumModules; k++)
		{
			PXISlotMap[k] = moduleslot->at(k);
		}


	int retval = 0;
	retval = Pixie16InitSystem(NumModules, PXISlotMap, OfflineMode);
	if (retval != 0)
		{
			ErrorInfo("Detector.cc", "BootSystem()", "Pixie16InitSystem", retval);
			std::cout << "PCI Pixie init has failed: " << retval << std::endl;
			return false;
		}


	if(OfflineMode != 0)
		{
			OfflineMode = 1;
			for(unsigned short k = 0; k < NumModules; k++)
	{
		if(modulesamplingrate->at(k) == 100 && modulebits->at(k) == 12)
			OfflineMode = 1;
		else if(modulesamplingrate->at(k) == 100 && modulebits->at(k) == 14)
			OfflineMode = 2;
		else if(modulesamplingrate->at(k) == 250 && modulebits->at(k) == 12)
			OfflineMode = 3;
		else if(modulesamplingrate->at(k) == 250 && modulebits->at(k) == 14)
			OfflineMode = 4;
		else if(modulesamplingrate->at(k) == 500 && modulebits->at(k) == 12)
			OfflineMode = 5;
		else if(modulesamplingrate->at(k) == 500 && modulebits->at(k) == 14)
			OfflineMode = 6;
		else if(modulesamplingrate->at(k) == 250 && modulebits->at(k) == 16)
			OfflineMode = 7;

		Pixie16SetOfflineVariant(k,OfflineMode);
		// HongyiWuPixie16SetOfflineVariant(k,OfflineMode,modulebits->at(k),modulesamplingrate->at(k));
		// ModuleInformation[k].Module_OfflineVariant = OfflineMode;
	}
			OfflineMode = 1;
		}


	for(unsigned short k = 0; k < NumModules; k++)
		{
			retval = Pixie16ReadModuleInfo(k, &ModuleInformation[k].Module_Rev, &ModuleInformation[k].Module_SerNum, &ModuleInformation[k].Module_ADCBits, &ModuleInformation[k].Module_ADCMSPS);

			if(retval != 0)
	{
		ErrorInfo("Detector.cc", "BootSystem()", "Pixie16ReadModuleInfo", retval);
		std::cout<<"Pixie16ReadModuleInfo: failed to read module information in module " <<k<<", retval="<<retval<<std::endl;
	}
			else
	{
		// if(OfflineMode != 0)
		//   {
		//     ModuleInformation[k].Module_ADCBits = modulebits->at(k);
		//     ModuleInformation[k].Module_ADCMSPS = modulesamplingrate->at(k);
		//   }
		std::cout<<"Rev:"<<ModuleInformation[k].Module_Rev<<"  SerNum:"<<ModuleInformation[k].Module_SerNum<<"  ADCBits:"<<ModuleInformation[k].Module_ADCBits<<"  ADCMSPS:"<<ModuleInformation[k].Module_ADCMSPS<<std::endl;
	}
		}


	for(unsigned short k = 0; k < NumModules; k++)
		{
			// 旧版固件0x7F 启动有问题，11032016新版没问题
			strcpy(DSPParFile, FileSettingPars.c_str());

			if(ModuleInformation[k].Module_ADCMSPS == 100 && ModuleInformation[k].Module_ADCBits == 12)
	{
#ifdef FIRMWARE100M12BIT
			strcpy(ComFPGAConfigFile, File100M12bit_sys.c_str());
			strcpy(SPFPGAConfigFile, File100M12bit_fip.c_str());
			strcpy(TrigFPGAConfigFile, File100M12bit_dsplst.c_str());
			strcpy(DSPCodeFile, File100M12bit_dspldr.c_str());
			strcpy(DSPVarFile, File100M12bit_dspvar.c_str());
#else
		std::cout<<"No 100M-12bit firmware was found!!!"<<std::endl;
		return false;
#endif
	}
			else if(ModuleInformation[k].Module_ADCMSPS == 100 && ModuleInformation[k].Module_ADCBits == 14)
		{
#ifdef FIRMWARE100M14BIT
			strcpy(ComFPGAConfigFile, File100M14bit_sys.c_str());
			strcpy(SPFPGAConfigFile, File100M14bit_fip.c_str());
			strcpy(TrigFPGAConfigFile, File100M14bit_dsplst.c_str());
			strcpy(DSPCodeFile, File100M14bit_dspldr.c_str());
			strcpy(DSPVarFile, File100M14bit_dspvar.c_str());
#else
		std::cout<<"No 100M-14bit firmware was found!!!"<<std::endl;
		return false;
#endif
		}
			else if(ModuleInformation[k].Module_ADCMSPS == 250 && ModuleInformation[k].Module_ADCBits == 12)
	{
#ifdef FIRMWARE250M12BIT
			strcpy(ComFPGAConfigFile, File250M12bit_sys.c_str());
			strcpy(SPFPGAConfigFile, File250M12bit_fip.c_str());
			strcpy(TrigFPGAConfigFile, File250M12bit_dsplst.c_str());
			strcpy(DSPCodeFile, File250M12bit_dspldr.c_str());
			strcpy(DSPVarFile, File250M12bit_dspvar.c_str());
#else
		std::cout<<"No 250M-14bit firmware was found!!!"<<std::endl;
		return false;
#endif
	}
			else if(ModuleInformation[k].Module_ADCMSPS == 250 && ModuleInformation[k].Module_ADCBits == 14)
		{
#ifdef FIRMWARE250M14BIT
			strcpy(ComFPGAConfigFile, File250M14bit_sys.c_str());
			strcpy(SPFPGAConfigFile, File250M14bit_fip.c_str());
			strcpy(TrigFPGAConfigFile, File250M14bit_dsplst.c_str());
			strcpy(DSPCodeFile, File250M14bit_dspldr.c_str());
			strcpy(DSPVarFile, File250M14bit_dspvar.c_str());
#else
		std::cout<<"No 250M-14bit firmware was found!!!"<<std::endl;
		return false;
#endif
		}
			else if(ModuleInformation[k].Module_ADCMSPS == 250 && ModuleInformation[k].Module_ADCBits == 16)
		{
#ifdef FIRMWARE250M16BIT
			strcpy(ComFPGAConfigFile, File250M16bit_sys.c_str());
			strcpy(SPFPGAConfigFile, File250M16bit_fip.c_str());
			strcpy(TrigFPGAConfigFile, File250M16bit_dsplst.c_str());
			strcpy(DSPCodeFile, File250M16bit_dspldr.c_str());
			strcpy(DSPVarFile, File250M16bit_dspvar.c_str());
#else
		std::cout<<"No 250M-16bit firmware was found!!!"<<std::endl;
		return false;
#endif
		}
			else if(ModuleInformation[k].Module_ADCMSPS == 500 && ModuleInformation[k].Module_ADCBits == 12)
	{
#ifdef FIRMWARE500M12BIT
			strcpy(ComFPGAConfigFile, File500M12bit_sys.c_str());
			strcpy(SPFPGAConfigFile, File500M12bit_fip.c_str());
			strcpy(TrigFPGAConfigFile, File500M12bit_dsplst.c_str());
			strcpy(DSPCodeFile, File500M12bit_dspldr.c_str());
			strcpy(DSPVarFile, File500M12bit_dspvar.c_str());
#else
		std::cout<<"No 500M-12bit firmware was found!!!"<<std::endl;
		return false;
#endif
	}
			else if(ModuleInformation[k].Module_ADCMSPS == 500 && ModuleInformation[k].Module_ADCBits == 14)
		{
#ifdef FIRMWARE500M14BIT
			strcpy(ComFPGAConfigFile, File500M14bit_sys.c_str());
			strcpy(SPFPGAConfigFile, File500M14bit_fip.c_str());
			strcpy(TrigFPGAConfigFile, File500M14bit_dsplst.c_str());
			strcpy(DSPCodeFile, File500M14bit_dspldr.c_str());
			strcpy(DSPVarFile, File500M14bit_dspvar.c_str());
#else
		std::cout<<"No 500M-14bit firmware was found!!!"<<std::endl;
		return false;
#endif
		}
			else
		{
			std::cout<<"There is no Samping Rate =>"<<ModuleInformation[k].Module_ADCMSPS<<" and Bits =>"<<ModuleInformation[k].Module_ADCBits<<" firmware. "<<std::endl;
			std::cout<<"Please contact Hongyi Wu(wuhongyi@qq.com)"<<std::endl;
			return false;
		}
			std::cout<<"----------------------------------------"<<std::endl;
			std::cout<<ComFPGAConfigFile<<std::endl;
			std::cout<<SPFPGAConfigFile<<std::endl;
			std::cout<<TrigFPGAConfigFile<<std::endl;
			std::cout<<DSPCodeFile<<std::endl;
			std::cout<<DSPParFile<<std::endl;
			std::cout<<DSPVarFile<<std::endl;
			retval = Pixie16BootModule(ComFPGAConfigFile,	// name of communications FPGA configuration file
					 SPFPGAConfigFile,	// name of signal processing FPGA configuration file
					 TrigFPGAConfigFile,	// name of trigger FPGA configuration file
					 DSPCodeFile,	// name of executable code file for digital signal processor (DSP)
					 DSPParFile,	// name of DSP parameter file
					 DSPVarFile,	// name of DSP variable names file
					 k,	// pixie module number
					 0x7F);	// boot pattern bit mask

			if (retval != 0)
		{
			ErrorInfo("Detector.cc", "BootSystem()", "Pixie16BootModule", retval);
			std::cout << "Booti failed!  Module NUMBER "<< k <<"  : ";
		switch(retval)
			{
			case -1:
				std::cout<<"Invalid Pixie-16 module number."<<std::endl;
				break;
			case -2:
				std::cout<<"Size of ComFPGAConfigFile is invalid."<<std::endl;
				break;
			case -3:
				std::cout<<"Failed to boot Communication FPGA, Check log file."<<std::endl;
				break;
			case -4:
				std::cout<<"Failed to allocate memory to store data in ComFPGAConfigFile."<<std::endl;
				break;
			case -5:
				std::cout<<"Failed to open ComFPGAConfigFile"<<std::endl;
				break;
			case -10:
				std::cout<<"Size of SPFPGAConfigFile is invalid"<<std::endl;
				break;
			case -11:
				std::cout<<"Failed to boot signal processing FPGA, Check log file."<<std::endl;
				break;
			case -12:
				std::cout<<"Failed to allocate memory to store data in SPFPGAConfigFile."<<std::endl;
				break;
			case -13:
				std::cout<<"Failed to open SPFPGAConfigFile"<<std::endl;
				break;
			case -14:
				std::cout<<"Failed to boot DSP, Check log file."<<std::endl;
				break;
			case -15:
				std::cout<<"Failed to allocate memory to store DSP executable code."<<std::endl;
				break;
			case -16:
				std::cout<<"Failed to open DSPCodeFile."<<std::endl;
				break;
			case -17:
				std::cout<<"Size of DSPParFile is invalid."<<std::endl;
				break;
			case -18:
				std::cout<<"Failed to open DSPParFile."<<std::endl;
				break;
			case -19:
				std::cout<<"Can’t initialize DSP variable indices."<<std::endl;
				break;
			case -20:
				std::cout<<"Can’t copy DSP variable indices, Check log file."<<std::endl;
				break;
			case -21:
				std::cout<<"Failed to program Fippi in a module, Check log file."<<std::endl;
				break;
			case -22:
				std::cout<<"Failed to set DACs in a module, Check log file."<<std::endl;
				break;
			case -23:
				std::cout<<"Failed to start RESET_ADC run in a module, Check log file."<<std::endl;
				break;
			case -24:
				std::cout<<"RESET_ADC run timed out in a module, Check log file."<<std::endl;
				break;

			default:
				std::cout<<""<<std::endl;
				break;
			}
			return false;
		}

			// close debug mode   TrigConfig3 [0]
			unsigned int ModParData;
			retval = Pixie16ReadSglModPar((char*)"TrigConfig3", &ModParData, k);
			if(retval < 0) ErrorInfo("Detector.cc", "BootSystem()", "Pixie16ReadSglModPar/TrigConfig3", retval);
			if(APP32_TstBit(0, ModParData))
	{
		ModParData = APP32_ClrBit(0, ModParData);
		retval = Pixie16WriteSglModPar((char*)"TrigConfig3", ModParData, k);
		if(retval < 0) ErrorInfo("Detector.cc", "BootSystem()", "Pixie16WriteSglModPar/TrigConfig3", retval);
	}
		}

	for(unsigned short k = 0; k < NumModules; k++)
		{
			retval = Pixie16WriteSglModPar((char*)"CrateID", crateid, k);
			if (retval < 0)
	{
		ErrorInfo("Detector.cc", "BootSystem()", "Pixie16WriteSglModPar/CrateID", retval);
		std::cout<<"Pixie16WriteSglModPar/CrateID in module "<<k<<" failed, retval = "<<retval<<std::endl;
		return false;
	}
		}


	// Adjust DC-Offsets
	for(unsigned short k = 0; k < NumModules; k++)
		{
			retval = Pixie16AdjustOffsets(k);
			if (retval < 0)
	{
		ErrorInfo("Detector.cc", "BootSystem()", "Pixie16AdjustOffsets", retval);
		std::cout<<"Pixie16AdjustOffsets in module "<<k<<" failed, retval = "<<retval<<std::endl;
		return false;
	}

			unsigned int blcut;
			for (unsigned short kk = 0; kk < 16; ++kk)
	{
		retval = Pixie16BLcutFinder(k,kk,&blcut);
		if(retval < 0)
			{
				ErrorInfo("Detector.cc", "BootSystem()", "Pixie16BLcutFinder", retval);
				return false;
			}
	}
		}

	std::cout<<"----------------------------------------"<<std::endl;
	if(OfflineMode == 0)
		{
			for(unsigned short k = 0; k < NumModules; k++)
	{
		unsigned int CSR;
		retval = Pixie16ReadCSR(k, &CSR);
		if(retval != 0)
			{
				ErrorInfo("Detector.cc", "BootSystem()", "Pixie16ReadCSR", retval);
				std::cout<<"Pixie16ReadCSR: Invalid Pixie module number in module "<<k<<", retval="<<retval<<std::endl;
			}
		else
			{
				std::cout<<"host Control & Status Register =>  mod:"<<k<<"  value:0x"<<hex<<CSR<<dec<<std::endl;
			}
	}
		}

	 return true;

}

int Detector::Syncronise()
{
	int retval = 0;
	retval = Pixie16WriteSglModPar((char*)"IN_SYNCH", 0, 0);
	if (retval < 0)
		{
			ErrorInfo("Detector.cc", "Syncronise()", "Pixie16WriteSglModPar/IN_SYNCH", retval);
			fprintf(stderr, "Failed to write IN_SYNCH");
		}
	return retval;
}


/// @brief save online needed information to file
/// @param[in] run run number
/// @param[in] crate crate number
/// @param[in] sampling_rate module sampling rate
/// @param[in] group_index group index of module
void SaveOnlineInformation(
	const int run,
	const int crate,
	const std::vector<unsigned short> &sampling_rate,
	const std::vector<unsigned int> &group_index
) {
	// get directory path
	std::string path = std::string(getenv("HOME")) + "/.xia-daq-gui-online";
	// create directory
	std::filesystem::create_directories(path);
	// open file
	std::ofstream fout(path+"/online_information.txt");
	fout << run << "\n"
		<< crate << "\n"
		<< sampling_rate.size() << "\n";
    for (size_t i = 0; i < sampling_rate.size(); ++i) {
		fout << sampling_rate[i] << " \n"[i==sampling_rate.size()-1];
	}
	for (size_t i = 0; i < group_index.size(); ++i) {
		fout << group_index[i] << " \n"[i==group_index.size()-1];
	}
	// close file
	fout.close();
}

int Detector::StartRun(int continue_run)
{
	std::cout<<"RUN START"<<std::endl;

#ifdef DECODERONLINE
	memcpy(shmptr_dec,&runnumber,sizeof(int));
	for(unsigned short i = 0;i < NumModules;i++)
		{
			memcpy(shmptr_dec+4+4*i,&buffid[i],sizeof(int));
		}
#endif


	int retval = 0;
	// All modules start acuqire and Stop acquire simultaneously
	retval = Pixie16WriteSglModPar((char*)"SYNCH_WAIT", 1, 0);
	if (retval < 0)
		{
			ErrorInfo("Detector.cc", "StartRun(...)", "Pixie16WriteSglModPar/SYNCH_WAIT", retval);
			fprintf(stderr, "Failed to write SYNCH_WAIT\n");
			return retval;
		}

	if (continue_run == 0)
		{
			// Reset clock counters to 0
			retval = Pixie16WriteSglModPar((char*)"IN_SYNCH", 0, 0);
			if (retval < 0)
	{
		ErrorInfo("Detector.cc", "StartRun(...)", "Pixie16WriteSglModPar/IN_SYNCH", retval);
		fprintf(stderr, "In Sync problem\n");
		return retval;
	}
		}

	PrevRateTime = get_time();// used in statistics
	StartTime = get_time();

	if (continue_run == 0)
		retval = Pixie16StartListModeRun(NumModules,0x100, NEW_RUN);
	else
		retval = Pixie16StartListModeRun(NumModules, 0x100,RESUME_RUN);
	if (retval < 0)
		{
			ErrorInfo("Detector.cc", "StartRun(...)", "Pixie16StartListModeRun", retval);
			fprintf(stderr, "Failed to start ListMode run in module");
			return retval;
		}

	char run_name[16];
	sprintf(run_name, "run%d", runnumber);
	for (unsigned int i = 0; i < NumModules; ++i) {
		// module name
		char module_name[16];
		sprintf(module_name, "c%um%u", crateid, i);
		// initialize publishers
		publishers_[i] = iox_pub_init(
			publisher_storage_+i,
			"DaqPacket", run_name, module_name,
			&publisher_options_
		);
	}

	// save online needed information
	SaveOnlineInformation(
		runnumber,
		crateid,
		*modulesamplingrate,
		group_index_
	);

	return 0;
}


void AllocatePayload(
	const iox_pub_t &publisher,
	void **user_payload,
	PacketHeader **header,
	DaqPacket **packet
) {
	const uint32_t ALIGNMENT = 8;
	// allocate shared memeory
	enum iox_AllocationResult res = iox_pub_loan_aligned_chunk_with_user_header(
		publisher, user_payload,
		sizeof(DaqPacket), ALIGNMENT,
		sizeof(PacketHeader), ALIGNMENT
	);
	if (res != AllocationResult_SUCCESS) {
		// allocate memory failed
		*user_payload = nullptr;
		*header = nullptr;
		*packet = nullptr;
		iox::LogWarn() << "Failed to allocate user payload! "
			<< "Error code " << int(res);
	}
	// success, get header pointer
	*header = (PacketHeader*)iox_chunk_header_to_user_header(
		iox_chunk_header_from_user_payload(*user_payload)
	);
	// initialize length
	(*header)->length = 0;
	// get packet pointer
	*packet = (DaqPacket*)(*user_payload);
}


void CopyBuffer(
	unsigned int *buffer,
	PacketHeader *header,
	DaqPacket *packet,
	size_t copy_words,
	size_t &packet_unread_words,
	size_t &packet_read_position
) {
	memcpy(
		(void*)(packet->data + header->length),
		(void*)(buffer + packet_read_position),
		copy_words*sizeof(unsigned int)
	);
	header->length += copy_words;
	packet_unread_words -= copy_words;
	packet_read_position += copy_words;
}


int Detector::ReadDataFromModules(int thres,unsigned short  endofrun) {
	// when events' number exceeds thres, data will be read out from FIFO
	if(endofrun == 0) {
		if(thres <= EXTFIFO_READ_THRESH) thres = EXTFIFO_READ_THRESH; // 1024 words
		if(thres > 3*EXTFIFO_READ_THRESH) thres = 3*EXTFIFO_READ_THRESH;
	} else {
		thres = 2;
	}

	if(fonline) StatisticsForModule();

	// initialize group write flag
	for (int i = 0; i < PRESET_MAX_MODULES; ++i) {
		group_read_[i] = false;
	}

	// words in FIFO
	unsigned int nwords[PRESET_MAX_MODULES];

	for (unsigned short i = 0; i < NumModules; ++i) {
		// check result
		int check_result = Pixie16CheckExternalFIFOStatus(nwords+i, i);
		if (check_result < 0) {
			ErrorInfo("Detector.cc", "ReadDataFromModules(...)", "Pixie16CheckExternalFIFOStatus", check_result);
			std::cout << "Invalid modnum!\n";
			return 0;
		}
		// allocate payload
		if (!user_payload_[i]) {
			AllocatePayload(
				publishers_[i], user_payload_+i, header_+i, packet_+i
			);
		}

		// read group if reach the packet threshold
		size_t read_words = user_payload_[i] ? header_[i]->length : 0;
		if (
			nwords[i] + read_words + packet_unread_words_[i]
				> PACKET_SIZE - EXTFIFO_READ_THRESH
		) {
			group_read_[group_index_[i]] = true;
		}
	}

	for (unsigned short i = 0; i < NumModules; ++i) {
		if (group_read_[group_index_[i]] || nwords[i] > (unsigned int)thres) {
			// buffer is full
			if (nwords[i] + buffer_top_[i] > BUFFER_LENGTH) {
				if (user_payload_[i]) {
					// get copy words
					size_t copy_words = packet_unread_words_[i];
					// get packet tail
					packet_tail_[i] = packet_unread_words_[i] % module_align_[i];
					// align in 4 words
					copy_words -= packet_tail_[i];
					// copy to shared memory
					memcpy(
						(void*)(packet_[i]->data + header_[i]->length),
						(void*)(buffer_[i] + packet_read_position_[i]),
						copy_words*sizeof(unsigned int)
					);
					header_[i]->length += copy_words;
// if (header_[i]->length > PACKET_SIZE) std::cout << "Packet " << packet_id_[group_index_[i]] << " over size, " << i << ", " << header_[i]->length << "\n";
// if (packet_read_position_[i]+copy_words > buffer_top_[i]) std::cout << "Copy over buffer top, " << i << ", "
// 	<< "read position " << packet_read_position_[i] << ", copy words " << copy_words
// 	<< ", buffer top " << buffer_top_[i] << ", packet id " << packet_id_[group_index_[i]] << "\n";
					packet_tail_[i] = (module_align_[i] - packet_tail_[i]) % module_align_[i];
					packet_read_position_[i] = packet_tail_[i];
					packet_unread_words_[i] = 0;
				}
				SavetoFile(i);
			}
			if (nwords[i] > 0) {
				int read_result = Pixie16ReadDataFromExternalFIFO(
					buffer_[i] + buffer_top_[i],
					nwords[i],
					i
				);
				if (read_result < 0) {
					ErrorInfo("Detector.cc", "ReadDataFromModules(...)", "Pixie16ReadDataFromExternalFIFO", read_result);
					std::cout<<"CheckExternalFIFOWords: " << i << ", " << nwords[i] << std::endl;
				}
				buffer_top_[i] += nwords[i];
				packet_unread_words_[i] += nwords[i];
			}
		}
		if (group_read_[group_index_[i]]) {
			// copy to payload and publish
			if (user_payload_[i]) {
				if (packet_tail_[i]) {
					packet_unread_words_[i] -= packet_tail_[i];
					packet_tail_[i] = 0;
				}
				size_t copy_words =
					packet_unread_words_[i] + header_[i]->length <= PACKET_SIZE
					? packet_unread_words_[i]
					: PACKET_SIZE - header_[i]->length;
				copy_words -= copy_words % module_align_[i];
				memcpy(
					(void*)(packet_[i]->data+header_[i]->length),
					(void*)(buffer_[i]+packet_read_position_[i]),
					copy_words*sizeof(unsigned int)
				);
				// update variable
				header_[i]->length += copy_words;
// if (header_[i]->length > PACKET_SIZE) std::cout << "Packet " << packet_id_[group_index_[i]] << " over size " << i << ", " << header_[i]->length << "\n";
// if (packet_read_position_[i]+copy_words > buffer_top_[i]) std::cout << "Copy over buffer top, " << i << ", "
// 	<< "read position " << packet_read_position_[i] << ", copy words " << copy_words
// 	<< ", buffer top " << buffer_top_[i] << ", packet id " << packet_id_[group_index_[i]] << "\n";
				packet_read_position_[i] += copy_words;
				packet_unread_words_[i] -= copy_words;
				// publish
				header_[i]->id = packet_id_[group_index_[i]];
				iox_pub_publish_chunk(publishers_[i], user_payload_[i]);
// std::cout << "Publish " << i << ", " << packet_id_[group_index_[i]] << "\n";
				user_payload_[i] = nullptr;
				header_[i] = nullptr;
				packet_[i] = nullptr;
			}
		}
	}
	// increase packet ID
	for (unsigned short i = 0; i < NumModules; ++i) {
		if (group_read_[i]) ++packet_id_[i];
	}


	// // group policy with second level cache sync
	// // check readable words
	// for (unsigned short i = 0; i < NumModules; ++i) {
	// 	// unsigned int nwords;
	// 	int check_result = Pixie16CheckExternalFIFOStatus(nwords+i, i);
	// 	if (check_result < 0) {
	// 		ErrorInfo("Detector.cc", "ReadDataFromModules(...)", "Pixie16CheckExternalFIFOStatus", check_result);
	// 		std::cout << "Invalid modnum!\n";
	// 		return 0;
	// 	}
	// 	if (nwords[i] < (unsigned int)thres) continue;

	// 	if (!user_payload_[i]) {
	// 		AllocatePayload(
	// 			publishers_[i], user_payload_+i, header_+i, packet_+i
	// 		);
	// 		if (!user_payload_[i]) continue;
	// 	}

	// 	if (header_[i]->length + nwords[i] >= PACKET_SIZE) {
	// 		group_read_[group_index_[i]] = true;
	// 	}
	// }

	// // write to file and publish
	// for (unsigned short i = 0; i < NumModules; ++i) {
	// 	if (group_read_[group_index_[i]] && user_payload_[i]) {
	// 		SavetoFile(i);

	// 		// config header
	// 		header_[i]->id = packet_id_[i];
	// 		++packet_id_[i];
	// 		iox_pub_publish_chunk(publishers_[i], user_payload_[i]);

	// 		// allocate new shared memory
	// 		AllocatePayload(
	// 			publishers_[i], user_payload_+i, header_+i, packet_+i
	// 		);
	// 	}

	// 	if (!user_payload_[i]) continue;

	// 	if (nwords[i] < (unsigned int)thres) continue;
	// 	int read_result = Pixie16ReadDataFromExternalFIFO(
	// 		packet_[i]->data + header_[i]->length,
	// 		nwords[i],
	// 		i
	// 	);
	// 	if(read_result < 0){
	// 		ErrorInfo("Detector.cc", "ReadDataFromModules(...)", "Pixie16ReadDataFromExternalFIFO", read_result);
	// 		std::cout<<"CheckExternalFIFOWords: " << i << ", " << nwords[i] << std::endl;
	// 	}
	// 	header_[i]->length += nwords[i];
	// }

	// // no group policy
	// for(unsigned short i = 0;i < NumModules;i++) {
	// 	unsigned int nwords;
	// 	retval = Pixie16CheckExternalFIFOStatus(&nwords,i);
	// 	if(retval<0) {
	// 		ErrorInfo("Detector.cc", "ReadDataFromModules(...)", "Pixie16CheckExternalFIFOStatus", retval);
	// 		std::cout<<"Invalid modnum!"<<std::endl;
	// 		return 0;
	// 	}

	// 	if (nwords < (unsigned int)thres) continue;

	// 	if (!user_payload_[i]) {
	// 		AllocatePayload(
	// 			publishers_[i], user_payload_+i, header_+i, packet_+i
	// 		);
	// 		if (!user_payload_[i]) continue;
	// 	}

	// 	// save to file and publish
	// 	if (header_[i]->length + nwords >= BUFFER_LENGTH) {
	// 		SavetoFile(i);

	// 		// write packet ID
	// 		header_[i]->id = packet_id_[i];
	// 		iox_pub_publish_chunk(publishers_[i], user_payload_[i]);
	// 		++packet_id_[i];

	// 		// allocate new shared memory
	// 		AllocatePayload(
	// 			publishers_[i], user_payload_+i, header_+i, packet_+i
	// 		);
	// 		if (!user_payload_[i]) continue;
	// 	}
	// 	// write to packet
	// 	retval = Pixie16ReadDataFromExternalFIFO(
	// 		packet_[i]->data+header_[i]->length,
	// 		nwords,
	// 		i
	// 	);
	// 	if(retval < 0){
	// 		ErrorInfo("Detector.cc", "ReadDataFromModules(...)", "Pixie16ReadDataFromExternalFIFO", retval);
	// 		std::cout<<"CheckExternalFIFOWords: "<<nwords<<std::endl;
	// 	}
	// 	header_[i]->length += nwords;
	// 	//    cout<<"nwords: "<<nwords<<endl;
	// }

	return 1;
}

long Detector::get_time()
{
	long time_ms;
	struct timeval t1;
	struct timezone tz;
	gettimeofday(&t1, &tz);
	time_ms = (t1.tv_sec) * 1000 + t1.tv_usec / 1000;
	return time_ms;
}

void Detector::StatisticsForModule()
{

	CurrentTime = get_time();
	ElapsedTime = CurrentTime - PrevRateTime; /* milliseconds */
	if (ElapsedTime > 3000)
		{
			UpdateSharedMemory();
			PrevRateTime = CurrentTime;
		}
}

int Detector::RunStatus()
{
	int sum=0;

	for(int i = 0;i < NumModules;i++)
		{
			int ret = 0;
			ret = Pixie16CheckRunStatus(i);
			std::cout<<"Run status: "<<i <<" sta:"<<ret<<std::endl;
			sum = sum + ret;
		}
	return sum;
}


int Detector::AcquireADCTrace(unsigned short *trace, unsigned long size, unsigned short module, unsigned short ChanNum)
{
	int result;
	if(module < NumModules)
		{
				result = Pixie16AcquireADCTrace(module);
	if (result < 0)
		{
			ErrorInfo("Detector.cc", "AcquireADCTrace(...)", "Pixie16AcquireADCTrace", result);
			return result;
		}

				result = Pixie16ReadSglChanADCTrace(trace,	// trace data
							size,	// trace length
							module,	// module number
							ChanNum);
	if (result < 0) ErrorInfo("Detector.cc", "AcquireADCTrace(...)", "Pixie16ReadSglChanADCTrace", result);
		}
	else
		{
			std::cout<<"wrong module number: "<<module<<std::endl;
			return -1000;
		}

	FILE* datafil;
	datafil=fopen("trace.dat","w");

	for(unsigned int i = 0;i < size;i++)
		{
			fprintf(datafil,"%d %d\n",i,trace[i]);
		}
	fclose(datafil);

	return result;
}

int Detector::OpenSaveFile(int n,const char *FileN)
{
	if(frecord)
		{
			fsave[n]=fopen(FileN,"wb");
			if(fsave[n] == NULL)
	{
		std::cout<<"Cannot open store file!"<<std::endl;
		return 0;
	}

	for(int i = 0; i < PRESET_MAX_MODULES; i++) {
		FILESIZE[i] = 0;
		buffer_top_[i] = 0;
		user_payload_[i] = nullptr;
		header_[i] = nullptr;
		packet_[i] = nullptr;
		packet_id_[i] = 0;
		packet_unread_words_[i] = 0;
		packet_read_position_[i] = 0;
		packet_tail_[i] = 0;
	}
#ifdef RECODESHA256
			SHA256_Init(&sha256_ctx[n]);
#endif
		}
	return 1;
}

void Detector::SetRecordFlag(bool flag)
{
	frecord = flag;
}

void Detector::SetRunFlag(bool flag)
{
	frunstatus = flag;
}

int Detector::SavetoFile(int nFile)
{
	// std::cout<<"saving file ..."<<std::endl;

#ifdef DECODERONLINE
	if(fdecoder)
		{
			for( ; ; )
	{
		int point;
		memcpy(&point,shmptr_dec+4+4*nFile,sizeof(int));
		if(point == 0)
			{
				memcpy(shmptr_dec+60+BUFFLENGTH*4*nFile,&buff[nFile],sizeof(int)*buffid[nFile]);
				memcpy(shmptr_dec+4+4*nFile,&buffid[nFile],sizeof(int));
				break;
			}
		std::cout<<"Wait Mod: "<<nFile<<"  Buff: "<<buffid[nFile]<<std::endl;
	}
		}
#endif

	if(frecord) {
		if(fsave[nFile] == NULL) {
			std::cout<<"ERROR! No opened file found for store!"<<std::endl;
			std::cout<<"CAUTION! No data will be saved!"<<std::endl;
			return 1;
		}

		size_t n = fwrite(
			buffer_[nFile], 4, buffer_top_[nFile], fsave[nFile] 
		);

		if (n != buffer_top_[nFile]) {
			std::cout << "Not all data has been stored!" << std::endl;
		}
		
		FILESIZE[nFile] += buffer_top_[nFile];
		buffer_top_[nFile] = 0; 

		// group policy with second level cache sync
		// size_t n = fwrite(
		// 	packet_[nFile]->data, 4, header_[nFile]->length, fsave[nFile]
		// );

		// if(n != header_[nFile]->length) {
		// 	std::cout<<"Not All Data has been stored!"<<std::endl;
		// }

		// FILESIZE[nFile] += header_[nFile]->length;

#ifdef RECODESHA256
			SHA256_Update(&sha256_ctx[nFile], (char *)buff[nFile], 4*buffid[nFile]);
#endif
	}

	// std::cout<<"FILE: "<<nFile<<" SIZE: "<<FILESIZE[nFile]<<std::endl;

	return 0;
}

int Detector::CloseFile() {
	for(unsigned short i = 0;i < NumModules;i++) {
		if (buffer_top_[i] > 0) SavetoFile(i);
		if(frecord) {
			fclose(fsave[i]);
#ifdef RECODESHA256
		SHA256_Final(SHA256result[i],&sha256_ctx[i]);
		 // for(int j = 0; j < 32; j++)
		 //   std::cout<<std::hex << std::setw(2) << std::setfill('0') << (int)(SHA256result[i][j]);
		 // std::cout<<std::endl;
#endif
		}
	}
	return 1;
}

unsigned int Detector::GetFileSize(int n)
{
	return (unsigned int)FILESIZE[n]/1024/1024*4;
}

int Detector::StopRun()
{
	std::cout<<"STOP RUN!"<<std::endl;
	unsigned short ModNum = 0;
	StopTime = get_time();
	int retval = Pixie16EndRun(ModNum);
	if(retval < 0) {
		ErrorInfo("Detector.cc", "StopRun(...)", "Pixie16EndRun", retval);
		std::cout<<"FAILED TO END THE RUN!!!"<<std::endl;
		return 1;
	}

	int counter = 0;
	while(RunStatus()) {
		ReadDataFromModules(0);
		counter++;
		if(counter > 10*NumModules) break;
		sleep(1); // wait 1s then try again  // add 20180504
	}
	if(counter >= 10*NumModules) {
		std::cout<<" ERROR! Some modules may not End Run correctly!"<<std::endl;
	}

	// Make sure all data has been read out
	ReadDataFromModules(0,1); // end of run
	for(unsigned short i = 0;i < NumModules;i++) {
		SavetoFile(i);
		if (user_payload_[i]) {
			header_[i]->id = packet_id_[i];
			iox_pub_publish_chunk(publishers_[i], user_payload_[i]);
			user_payload_[i] = nullptr;
			header_[i] = nullptr;
			packet_[i] = nullptr;
		}
		iox_pub_deinit(publishers_[i]);
	}
	CloseFile();

	std::cout<<"Real Run Time:"<<StopTime-StartTime<<std::endl;
	return 0;
}

#ifdef DECODERONLINE
void Detector::SetDecoderFlag(bool flag)
{
	fdecoder = flag;
}

void Detector::InitDecoderOnline()
{
	shmfd_dec = -1;
	if((shmfd_dec = shm_open("shmpixie16pkuxiadaqdecoderonline",O_CREAT|O_RDWR,0666)) < 0)
		{
			std::cout<<"Can not create shared memory [decoderonline]"<<std::endl;
		}

	if(ftruncate(shmfd_dec,(off_t)(60+BUFFLENGTH*13*4)) < 0)
		{
			std::cout<<"Can not alloc memory for shared memory [decoderonline] !"<<std::endl;
		}

	if((shmptr_dec = (unsigned char*) mmap(NULL,60+BUFFLENGTH*13*4, PROT_READ|PROT_WRITE,MAP_SHARED,shmfd_dec,0)) == MAP_FAILED)
		{
			std::cout<<"Can not mmap the shared memroy [decoderonline] to process space"<<std::endl;
		}
	return;
}
#endif


int Detector::OpenSharedMemory()
{
	 int flag = 0;
	 if((shmsem=sem_open("sempixie16pkuxiadaq",O_CREAT,0666,1)) == SEM_FAILED)
		 {
			 std::cout<<"Cannot create seamphore!"<<std::endl;
			 flag++;
		 }

	 if((shmfd=shm_open("shmpixie16pkuxiadaq",O_CREAT|O_RDWR,0666)) < 0)
		 {
			 std::cout<<"Cannot create shared memory"<<std::endl;
			 flag++;
		 }

	 if(ftruncate(shmfd,(off_t)(SHAREDMEMORYDATAOFFSET+PRESET_MAX_MODULES*2+PRESET_MAX_MODULES*4*SHAREDMEMORYDATASTATISTICS+PRESET_MAX_MODULES*4*SHAREDMEMORYDATAENERGYLENGTH*SHAREDMEMORYDATAMAXCHANNEL)) < 0)
		 {
			 std::cout<<"Cannot alloc memory for shared memory!"<<std::endl;
			 flag++;
		 }

	 if((shmptr = (unsigned char*) mmap(NULL,SHAREDMEMORYDATAOFFSET+PRESET_MAX_MODULES*2+(PRESET_MAX_MODULES*SHAREDMEMORYDATASTATISTICS*4)+PRESET_MAX_MODULES*4*SHAREDMEMORYDATAENERGYLENGTH*SHAREDMEMORYDATAMAXCHANNEL, PROT_READ|PROT_WRITE,MAP_SHARED,shmfd,0)) == MAP_FAILED)
		 {
			 std::cout<<"Cannot mmap the shared memroy to process space"<<std::endl;
			 flag++;
		 }
	 if(flag > 0) return 0;
	 std::cout<<"SHM Opend!"<<std::endl;
	 return 1;
}

int Detector::UpdateSharedMemory()
{
	int rc;
	rc = sem_trywait(shmsem);
	if(rc == -1 && errno != EAGAIN)
		{
			std::cout<<"sem_wait error!"<<std::endl;
			return 1;
		}
	else if(rc == -1) return 1; // this indicates the shm is under use

	shmid1++;
	memcpy(shmptr,&shmid1,sizeof(unsigned int));
	memcpy(shmptr+4,&NumModules,sizeof(unsigned short));
	crateidrunnumber = (crateid << 24) + (runnumber & 0xFFFFFF);
	memcpy(shmptr+6,&crateidrunnumber,sizeof(unsigned int));

	int retval = 0;
	unsigned int Statistics[SHAREDMEMORYDATASTATISTICS];
	for(unsigned short i = 0; i < NumModules; i++)
		{
			memcpy(shmptr+SHAREDMEMORYDATAOFFSET+2*i,&ModuleInformation[i].Module_ADCMSPS,sizeof(unsigned short));

			retval = Pixie16ReadStatisticsFromModule(Statistics, i);
			if(retval < 0)
	{
		ErrorInfo("Detector.cc", "UpdateSharedMemory()", "Pixie16ReadStatisticsFromModule", retval);
		std::cout<<"error in get statistics info"<<std::endl;
	}
			memcpy(shmptr+SHAREDMEMORYDATAOFFSET+PRESET_MAX_MODULES*2+SHAREDMEMORYDATASTATISTICS*4*i,Statistics,sizeof(unsigned int)*SHAREDMEMORYDATASTATISTICS);
		}

	if(sem_post(shmsem) == -1)
		{
			std::cout<<"sem_post error!"<<std::endl;
			return 1;
		}
	std::cout<<"SHM updated!"<<std::endl;
	return 0;
}

void Detector::UpdateEnergySpectrumForModule()
{
	int retval = 0;

	shmid2++;
	memcpy(shmptr+10,&shmid2,sizeof(unsigned int));

	unsigned int Statistics[SHAREDMEMORYDATAENERGYLENGTH*SHAREDMEMORYDATAMAXCHANNEL];
	for(unsigned short i = 0; i < NumModules; i++)
			{
	retval = Pixie16EMbufferIO(Statistics,SHAREDMEMORYDATAENERGYLENGTH*SHAREDMEMORYDATAMAXCHANNEL,0x0,1,i);
	if(retval < 0)
		{
			ErrorInfo("Detector.cc", "UpdateEnergySpectrumForModule()", "Pixie16EMbufferIO", retval);
			std::cout<<"null pointer for buffer data / number of buffer words exceeds the limit / invalid DSP external memory address / invalid I/O direction / invalid Pixie module number / I/O Failure"<<std::endl;
		}
	memcpy(shmptr+SHAREDMEMORYDATAOFFSET+PRESET_MAX_MODULES*2+PRESET_MAX_MODULES*4*SHAREDMEMORYDATASTATISTICS+i*4*SHAREDMEMORYDATAENERGYLENGTH*SHAREDMEMORYDATAMAXCHANNEL,Statistics,sizeof(unsigned int)*SHAREDMEMORYDATAENERGYLENGTH*SHAREDMEMORYDATAMAXCHANNEL);
			}

	std::cout<<"Updated Monitor Energy Spectrum. "<<std::endl;
	std::cout<<"Please don't update it too often. Frequent updates will affect DAQ efficiency."<<std::endl;
}

void Detector::UpdateFilePathAndNameInSharedMemory(const char *path,const char *filen)
{
	memcpy(shmptr+14,filen,128);
	memcpy(shmptr+142,path,1024);
	return;
}

int Detector::SetOnlineFlag(bool flag)
{
	fonline = flag;
	return 1;
}

// bool Detector::SetEvtl()
// {
//   evtlen = new unsigned int[NumModules];
//   for(unsigned short i = 0;i < NumModules;i++)
//     {
//       double mpar = -1;
//       evtlen[i] = 4; // header
//       int retval = Pixie16ReadSglChanPar((char*)"CHANNEL_CSRA",&mpar,i,0);
//       if(retval<0){
// 	ErrorInfo("Detector.cc", "SetEvtl()", "Pixie16ReadSglChanPar/CHANNEL_CSRA", retval);
// 	return 0;
//       }
//       if(APP16_TstBit(12,mpar)) evtlen[i] += 4; // esum and baseline enabled
//       if(APP16_TstBit(9,mpar)) evtlen[i] += 8; // qsum enabled
//       if(APP16_TstBit(8,mpar))
// 	{
// 	  double tracelen = -1;
// 	  retval = Pixie16ReadSglChanPar((char*)"TRACE_LENGTH",&tracelen,i,0);
// 	  if(retval < 0)
// 	    {
// 	      ErrorInfo("Detector.cc", "SetEvtl()", "Pixie16ReadSglChanPar/TRACE_LENGTH", retval);
// 	      return 0;
// 	    }
// 	  evtlen[i] += tracelen*50;
// 	}
//       std::cout<<"evtlen ch: "<<i<<" len: "<<evtlen[i]<<std::endl;
//     }
//   return 1;
// }

int Detector::ExitSystem()
{
	int retval = Pixie16ExitSystem(NumModules);
	if(retval<0)
		{
			ErrorInfo("Detector.cc", "ExitSystem()", "Pixie16ExitSystem", retval);
			return 1;
		}
	return 0;
}

int Detector::SaveHistogram(char *fileN , int mod)
{
	int retval ;
	retval = Pixie16SaveHistogramToFile(fileN,mod);
	if(retval <0 )
		{
		ErrorInfo("Detector.cc", "SaveHistogram(...)", "Pixie16SaveHistogramToFile", retval);
	}
	return 0;
}

int Detector::SaveDSPPars(char *file)
{
	int retval ;
	//Save DSP parameters to file
	retval = Pixie16SaveDSPParametersToFile(file);
	if (retval < 0)
		std::cout<<"saving DSP parameters to file failed, retval="<<retval<<std::endl;

	return 0;
}

void Detector::SetRunNumber(int r)
{
	runnumber = r;
}
