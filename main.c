#include <iopcontrol.h>
#include <iopheap.h>
#include <kernel.h>
#include <libcdvd.h>
#include <libhdd.h>
#include <libmc.h>
#include <fileXio_rpc.h>
#include <hdd-ioctl.h>
#include <loadfile.h>
#include <malloc.h>
#include <sbv_patches.h>
#include <sifcmd.h>
#include <sifrpc.h>
#include <stdio.h>
#include <string.h>

#include <libgs.h>

#include "main.h"
#include "pad.h"
#include "graphics.h"
#include "font.h"

#include "libsecr.h"
#include "mctools_rpc.h"
#include "system.h"
#include "ReqSpaceCalc.h"
#include "UI.h"
#include "menu.h"

#include "ioprstctrl.h"

extern unsigned char IOMANX_irx_start[];
extern unsigned int IOMANX_irx_size;

extern unsigned char FILEXIO_irx_start[];
extern unsigned int FILEXIO_irx_size;

extern unsigned char SIO2MAN_irx_start[];
extern unsigned int SIO2MAN_irx_size;

extern unsigned char PADMAN_irx_start[];
extern unsigned int PADMAN_irx_size;

extern unsigned char MCMAN_irx_start[];
extern unsigned int MCMAN_irx_size;

extern unsigned char MCSERV_irx_start[];
extern unsigned int MCSERV_irx_size;

extern unsigned char SECRSIF_irx_start[];
extern unsigned int SECRSIF_irx_size;

extern unsigned char MCTOOLS_irx_start[];
extern unsigned int MCTOOLS_irx_size;

extern unsigned char USBD_irx_start[];
extern unsigned int USBD_irx_size;

extern unsigned char USBHDFSD_irx_start[];
extern unsigned int USBHDFSD_irx_size;

extern unsigned char POWEROFF_irx_start[];
extern unsigned int POWEROFF_irx_size;

extern unsigned char DEV9_irx_start[];
extern unsigned int DEV9_irx_size;

extern unsigned char ATAD_irx_start[];
extern unsigned int ATAD_irx_size;

extern unsigned char HDD_irx_start[];
extern unsigned int HDD_irx_size;

extern unsigned char PFS_irx_start[];
extern unsigned int PFS_irx_size;

#ifdef DEBUG_TTY_FEEDBACK
extern unsigned char UDPTTY_irx_start[];
extern unsigned int UDPTTY_irx_size;
#endif

int IsHDDUnitConnected=0;

extern unsigned char UDNL_irx_start[];
extern unsigned int UDNL_irx_size;

int VBlankStartSema;

static int VBlankStartHandler(int cause){
	ee_sema_t sema;
	iReferSemaStatus(VBlankStartSema, &sema);
	if(sema.count<sema.max_count) iSignalSema(VBlankStartSema);
	return 0;
}

#define SYSTEM_INIT_THREAD_STACK_SIZE	0x800

struct SystemInitParams{
	int InitCompleteSema;
};

static void SystemInitThread(struct SystemInitParams *SystemInitParams){
	static const char PS2FS_args[]="-n\0""24\0""-o\0""8";	// Try not to adjust this unless you know what you are doing. The tricky part i keeping the NULL character in the middle of that argument list separated from the number 8.

	if(SifExecModuleBuffer(ATAD_irx_start, ATAD_irx_size, 0, NULL, NULL)>=0){
		SifExecModuleBuffer(HDD_irx_start, HDD_irx_size, 0, NULL, NULL);
		SifExecModuleBuffer(PFS_irx_start, PFS_irx_size, sizeof(PS2FS_args), PS2FS_args, NULL);
	}

	SifExecModuleBuffer(SECRSIF_irx_start, SECRSIF_irx_size, 0, NULL, NULL);
	SifExecModuleBuffer(MCSERV_irx_start, MCSERV_irx_size, 0, NULL, NULL);
	SifExecModuleBuffer(PADMAN_irx_start, PADMAN_irx_size, 0, NULL, NULL);
	SifExecModuleBuffer(MCTOOLS_irx_start, MCTOOLS_irx_size, 0, NULL, NULL);

	SignalSema(SystemInitParams->InitCompleteSema);
	ExitDeleteThread();
}

int main(int argc, char *argv[]){
	int SystemType;
	unsigned int FrameNum;
	ee_sema_t ThreadSema;
	struct SystemInitParams InitThreadParams;
	void *SysInitThreadStack;

/*	argv[0] = "mass:/FMCBInstaller/FMCBInstaller.elf";
	chdir("mass:/FMCBInstaller/");	*/
	if(argc<1 || GetBootDeviceID(argv[0])==BOOT_DEVICE_UNKNOWN){
		Exit(-1);
	}

	SifInitRpc(0);
	SifIopResetBufferUDNL(UDNL_irx_start, UDNL_irx_size);

	//Do something useful while the IOP resets.
	ThreadSema.init_count=0;
	ThreadSema.max_count=1;
	ThreadSema.attr=ThreadSema.option=0;
	InitThreadParams.InitCompleteSema=CreateSema(&ThreadSema);

	SysInitThreadStack=memalign(128, SYSTEM_INIT_THREAD_STACK_SIZE);

	ThreadSema.init_count=0;
	ThreadSema.max_count=1;
	ThreadSema.attr=ThreadSema.option=0;
	VBlankStartSema=CreateSema(&ThreadSema);

	AddIntcHandler(kINTC_VBLANK_START, &VBlankStartHandler, 0);
	EnableIntc(kINTC_VBLANK_START);

	while(!SifIopSync()){}

	SifInitRpc(0);
	SifInitIopHeap();
	SifLoadFileInit();
	sbv_patch_enable_lmb();

	SifExecModuleBuffer(POWEROFF_irx_start, POWEROFF_irx_size, 0, NULL, NULL);
	SifExecModuleBuffer(DEV9_irx_start, DEV9_irx_size, 0, NULL, NULL);
#ifdef DEBUG_TTY_FEEDBACK
	SifExecModuleBuffer(UDPTTY_irx_start, UDPTTY_irx_size, 0, NULL, NULL);
#endif
	SifExecModuleBuffer(IOMANX_irx_start, IOMANX_irx_size, 0, NULL, NULL);
	SifExecModuleBuffer(FILEXIO_irx_start, FILEXIO_irx_size, 0, NULL, NULL);

	SifExecModuleBuffer(USBD_irx_start, USBD_irx_size, 0, NULL, NULL);
	SifExecModuleBuffer(USBHDFSD_irx_start, USBHDFSD_irx_size, 0, NULL, NULL);

	SifExecModuleBuffer(SIO2MAN_irx_start, SIO2MAN_irx_size, 0, NULL, NULL);
	SifExecModuleBuffer(MCMAN_irx_start, MCMAN_irx_size, 0, NULL, NULL);

	SifLoadFileExit();
	fileXioInit();

	sceCdInit(SCECdINoD);

	if(InitializeUI()!=0){
		SifExitRpc();
		Exit(-1);
	}

	SysCreateThread(SystemInitThread, SysInitThreadStack, SYSTEM_INIT_THREAD_STACK_SIZE, &InitThreadParams, 0x1);

	FrameNum=0;
	/* Draw something nice here while waiting... */
	do{
		RedrawLoadingScreen(FrameNum);
		FrameNum++;
	}while(PollSema(InitThreadParams.InitCompleteSema)!=InitThreadParams.InitCompleteSema);
	DeleteSema(InitThreadParams.InitCompleteSema);
	free(SysInitThreadStack);

	DEBUG_PRINTF("Modules loaded.\n");

	mcInit(MC_TYPE_XMC);
	PadInitPads();

	SecrInit();
	InitMCTOOLS();

	StartWorkerThread();

	UpdateRegionalPaths();

	SystemType = GetPs2Type();
	if(SystemType == PS2_SYSTEM_TYPE_PS2 || SystemType == PS2_SYSTEM_TYPE_DEX){
		if(hddCheckPresent()==0 && hddCheckFormatted()==0){
			IsHDDUnitConnected=1;
		}
	}

	SecrInit();
	InitMCTOOLS();

	MainMenu();

ShutdownRPCsAndExit:
	StopWorkerThread();

	if(!IsHDDBootingEnabled())
		fileXioDevctl("hdd0:", HDDCTL_DEV9_SHUTDOWN, NULL, 0, NULL, 0);

	PadDeinitPads();

	DisableIntc(kINTC_VBLANK_START);
	RemoveIntcHandler(kINTC_VBLANK_START, 0);
	DeleteSema(VBlankStartSema);

	DeinitializeUI();

	DeinitMCTOOLS();
	SecrDeinit();

	sceCdInit(SCECdEXIT);

	fileXioExit();
	SifExitIopHeap();
	SifExitRpc();

	return 0;
}
