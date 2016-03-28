#include <kernel.h>
#include <libpad.h>
#include <stdio.h>
#include <string.h>
#include <tamtypes.h>

#include "pad.h"

static unsigned char padArea[2][256] ALIGNED(64);
static u32 old_pad[2]={0, 0};

void PadInitPads(void)
{
	padInit(0);
	padPortOpen(0, 0, padArea[0]);
	padPortOpen(1, 0, padArea[1]);

	memset(old_pad, 0, sizeof(old_pad));
}

void PadDeinitPads(void)
{
	padPortClose(0, 0);
	padPortClose(1, 0);
	padEnd();
}

int ReadPadStatus(int port, int slot){
	struct padButtonStatus buttons;
	u32 new_pad, paddata;

	new_pad=0;
	if (padRead(port, slot, &buttons) != 0) {
		paddata = 0xffff ^ buttons.btns;

		new_pad = paddata & ~old_pad[port];
		old_pad[port] = paddata;
	}

	return new_pad;
}

int ReadCombinedPadStatus(void){
	return(ReadPadStatus(0, 0)|ReadPadStatus(1, 0));
}
