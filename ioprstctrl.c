#include <kernel.h>
#include <stdio.h>
#include <iopcontrol.h>
#include <iopheap.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <sifdma.h>
#include <sifrpc.h>
#include <string.h>

#include "ioprstctrl.h"
#include "modutils.h"

/*--------------------------------------------------------------------------------------------------------------*/
/* A function that creates and send IOP reset SIF DMA packets                                                   */
/* containing commands to reset the IOP with a modified IOPRP image.                                            */
/*--------------------------------------------------------------------------------------------------------------*/
void SifIopResetBufferUDNL(const void *updater, unsigned int size)
{
	while(!SifIopReset(NULL, 0)){};
	while(!SifIopSync()){};

	SifInitRpc(0);
	SifInitIopHeap();
	SifLoadFileInit();

	sbv_patch_enable_lmb();

	SifSetReg(SIF_REG_SMFLAG, 0x40000);

	/* Load the IOPRP image asynchronously */
	SifExecModuleBufferAsync((void*)updater, size, 0, NULL, NULL);

	SifSetReg(SIF_REG_SMFLAG, 0x10000);
	SifSetReg(SIF_REG_SMFLAG, 0x20000);
	SifSetReg(0x80000002, 0);
	SifSetReg(0x80000000, 0);

	/* Keep the RPC services on the EE side in sync with the IOP side after IOP reset */
	/* The EE would have to wait for the IOP to complete the reset anyway... so the code is placed here for efficiency */
	SifExitRpc();
	SifLoadFileExit();
	SifExitIopHeap();
}
