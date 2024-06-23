#ifndef DETECTOR_HH_
#define DETECTOR_HH_

#include "Global.hh"

#include <fcntl.h>
#include <semaphore.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>
#include <cstring>
#include <string>
#ifdef RECODESHA256
#include <openssl/sha.h>
#endif

#if defined (__cplusplus)
extern "C" {
#endif
#include <iceoryx_binding_c/publisher.h>
#if defined (__cplusplus)
}
#endif

#include "external/pixie/app/pixie16app_export.h"
#include "include/daq-packet.h"

#define BUFFER_LENGTH 1310720// 5MB

#define FIRMWARE100M12BIT
#define FIRMWARE100M14BIT
#define FIRMWARE250M12BIT
#define FIRMWARE250M14BIT
#define FIRMWARE250M16BIT
#define FIRMWARE500M12BIT
#define FIRMWARE500M14BIT

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......


#define SHAREDMEMORYDATAOFFSET 1170 //BYTE
// 1st 4    bytes IDcode for event shared memory
// 2nd 2    bytes number of valid Num Modules in shared memory
// 3rd 4    bytes Crate ID/Run Number  [31:24]  [23:0]
// 4   4    bytes ID for update spectrum
// 5   128  bytes name of data file
// 6   1024 bytes path of data file
// 7   2    bytes software id
// 8   2    bytes software version
#define SHAREDMEMORYDATASTATISTICS 448
#define SHAREDMEMORYDATAENERGYLENGTH 32768
#define SHAREDMEMORYDATAMAXCHANNEL 16

// 共享内存数据定义 header + 插件采样率 + 数据统计 + 能谱
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

class Detector {
public:
	Detector(int mode);
	virtual ~Detector();

	int Syncronise();
	int ExitSystem();
	int StartRun(int continue_run);
	int OpenSaveFile(int n,const char *FileN);
	int SavetoFile(int nFile);
	int RunStatus();
	int ReadDataFromModules(int thres = 0,unsigned short endofrun = 0);
	int StopRun();
	int CloseFile();
	int SetOnlineFlag(bool flag);
	void SetRecordFlag(bool flag);
	bool GetRecordFlag() {return frecord;}
#ifdef DECODERONLINE
	void SetDecoderFlag(bool flag);
#endif
#ifdef RECODESHA256
	void GetSHA256(int n, unsigned char *sha) {memcpy(sha, SHA256result[n], 32*sizeof(unsigned char));}
#endif
	void SetRunFlag(bool flag);
	bool GetRunFlag() {return frunstatus;}
	int SaveHistogram(char *fileN,int mod);
	int SaveDSPPars(char *file);
	int OpenSharedMemory();
	int UpdateSharedMemory();
	void SetRunNumber(int r);
	void UpdateFilePathAndNameInSharedMemory(const char *path,const char *filen);
	unsigned int GetFileSize(int n); // in MB

	int AcquireADCTrace(unsigned short *trace, unsigned long size, unsigned short module, unsigned short ChanNum);

	bool BootSystem();

	void StatisticsForModule();
	void UpdateEnergySpectrumForModule();

	unsigned short GetModuleADCMSPS(int mod) {return ModuleInformation[mod].Module_ADCMSPS;}
	unsigned short GetModuleADCBits(int mod) {return ModuleInformation[mod].Module_ADCBits;}

	struct Module_Info {
		// Module information
		unsigned short Module_Rev;
		unsigned int   Module_SerNum;
		unsigned short Module_ADCBits;
		unsigned short Module_ADCMSPS;
		unsigned short Module_OfflineVariant;
	};


public:
  	unsigned short NumModules;

private:
	unsigned short *PXISlotMap;
	unsigned short OfflineMode;

	unsigned int crateid;
	std::vector<unsigned short> *moduleslot;
	std::vector<unsigned short> *modulesamplingrate;
	std::vector<unsigned short> *modulebits;

	std::string File100M12bit_sys;
	std::string File100M12bit_fip;
	std::string File100M12bit_dspldr;
	std::string File100M12bit_dsplst;
	std::string File100M12bit_dspvar;

	std::string File100M14bit_sys;//ComFPGAConfigFile
	std::string File100M14bit_fip;//SPFPGAConfigFile
	std::string File100M14bit_dsplst;//TrigFPGAConfigFile
	std::string File100M14bit_dspldr;//DSPCodeFile
	std::string File100M14bit_dspvar;//DSPVarFile

	std::string File250M12bit_sys;
	std::string File250M12bit_fip;
	std::string File250M12bit_dspldr;
	std::string File250M12bit_dsplst;
	std::string File250M12bit_dspvar;

	std::string File250M14bit_sys;
	std::string File250M14bit_fip;
	std::string File250M14bit_dspldr;
	std::string File250M14bit_dsplst;
	std::string File250M14bit_dspvar;

	std::string File250M16bit_sys;
	std::string File250M16bit_fip;
	std::string File250M16bit_dspldr;
	std::string File250M16bit_dsplst;
	std::string File250M16bit_dspvar;

	std::string File500M12bit_sys;
	std::string File500M12bit_fip;
	std::string File500M12bit_dspldr;
	std::string File500M12bit_dsplst;
	std::string File500M12bit_dspvar;

	std::string File500M14bit_sys;
	std::string File500M14bit_fip;
	std::string File500M14bit_dspldr;
	std::string File500M14bit_dsplst;
	std::string File500M14bit_dspvar;

	std::string FileSettingPars;//DSPParFile

	char ComFPGAConfigFile[1024];
	char SPFPGAConfigFile[1024];
	char TrigFPGAConfigFile[1024];
	char DSPCodeFile[1024];
	char DSPParFile[1024];
	char DSPVarFile[1024];

	bool ReadConfigFile(char *config = (char*)"./parset/cfgPixie16.txt");

	struct Module_Info ModuleInformation[PRESET_MAX_MODULES];

	// variables for shared memory
	sem_t *shmsem; // shared memory semaphore
	int    shmfd;  // shared memory id
	unsigned char *shmptr;// pointer to shm

	unsigned int shmid1,shmid2;

#ifdef DECODERONLINE
	// int    shmfd_dec;  // shared memory id
	unsigned char *shmptr_dec;// pointer to shm
	// runnumber 4 byte
	// bufflegth 4 byte * 13
	// temp      4 byte
	// buff * 14

	void InitDecoderOnline();
	bool   fdecoder;
#endif

#ifdef RECODESHA256
	SHA256_CTX sha256_ctx[PRESET_MAX_MODULES];
	unsigned char SHA256result[PRESET_MAX_MODULES][32];
#endif

	bool   fonline;
	bool   frecord;
	int runnumber;
	bool frunstatus;
	unsigned int crateidrunnumber;
	//  bool BootSystem (unsigned short NumModules, unsigned short *PXISlotMap);

	uint64_t CurrentTime;
	uint64_t PrevRateTime;
	uint64_t ElapsedTime;
	uint64_t StartTime;
	uint64_t StopTime;
	long get_time();


	FILE *fsave[PRESET_MAX_MODULES];
	unsigned int buffer_[PRESET_MAX_MODULES][BUFFER_LENGTH];
	size_t buffer_top_[PRESET_MAX_MODULES];
	unsigned long FILESIZE[PRESET_MAX_MODULES];
	// shared memory with iceoryx part
	// publisher
	iox_pub_options_t publisher_options_;
	iox_pub_t publishers_[PRESET_MAX_MODULES];
	iox_pub_storage_t publisher_storage_[PRESET_MAX_MODULES];
	// packet
	void *user_payload_[PRESET_MAX_MODULES];
	PacketHeader *header_[PRESET_MAX_MODULES];
	DaqPacket *packet_[PRESET_MAX_MODULES];
	// packet size and pointer
	uint64_t packet_id_[PRESET_MAX_MODULES];
	size_t packet_unread_words_[PRESET_MAX_MODULES];
	size_t packet_read_position_[PRESET_MAX_MODULES];
	size_t packet_tail_[PRESET_MAX_MODULES];
	// group policy
	unsigned int group_index_[PRESET_MAX_MODULES];
	bool group_read_[PRESET_MAX_MODULES];
};

#endif /*DETECTOR_HH_*/
