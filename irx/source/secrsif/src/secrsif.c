#include <loadcore.h>
#include <stdio.h>
#include <sysclib.h>
#include <thbase.h>
#include <irx.h>
#include <secrman.h>
#include <sifman.h>
#include <sifcmd.h>

#include "secrsif.h"

#define MODNAME "secrsif"
IRX_ID(MODNAME, 0x01, 0x02);

extern struct irx_export_table _exp_secrsif;

static SifRpcDataQueue_t SifQueueData01;	//0x00000990
static SifRpcDataQueue_t SifQueueData02;	//0x000009a8
static SifRpcDataQueue_t SifQueueData03;	//0x000009c0
static SifRpcDataQueue_t SifQueueData04;	//0x000009d8
static SifRpcDataQueue_t SifQueueData05;	//0x000009f0
static SifRpcDataQueue_t SifQueueData06;	//0x00000a08
static SifRpcDataQueue_t SifQueueData07;	//0x00000a20

static SifRpcServerData_t SifServerData01;	//0x00000a38
static SifRpcServerData_t SifServerData02;	//0x00000a80
static SifRpcServerData_t SifServerData03;	//0x00000ac8
static SifRpcServerData_t SifServerData04;	//0x00000b10
static SifRpcServerData_t SifServerData05;	//0x00000b58
static SifRpcServerData_t SifServerData06;	//0x00000ba0
static SifRpcServerData_t SifServerData07;	//0x00000be8

static int SifRpcThread01;	//0x00000c2c
static int SifRpcThread02;	//0x00000c30
static int SifRpcThread03;	//0x00000c34
static int SifRpcThread04;	//0x00000c38
static int SifRpcThread05;	//0x00000c3c
static int SifRpcThread06;	//0x00000c40
static int SifRpcThread07;	//0x00000c44

static unsigned char SifServerBuffer01[0x1000];	//0x00000c48

//0x00000000
static void *SifRpcA01_handler(int function, void *buffer, int nbytes){
	((struct SecrSifDownloadHeaderParams*)buffer)->result=SecrDownloadHeader(((struct SecrSifDownloadHeaderParams*)buffer)->port, ((struct SecrSifDownloadHeaderParams*)buffer)->slot, ((struct SecrSifDownloadHeaderParams*)buffer)->buffer, &((struct SecrSifDownloadHeaderParams*)buffer)->BitTable, &((struct SecrSifDownloadHeaderParams*)buffer)->size);
	return buffer;
}

//0x000000e0
static void *SifRpcA02_handler(int function, void *buffer, int nbytes){
	((struct SecrSifDownloadBlockParams*)buffer)->result=SecrDownloadBlock(((struct SecrSifDownloadBlockParams*)buffer)->buffer, ((struct SecrSifDownloadBlockParams*)buffer)->size);
	return buffer;
}

//0x000001b0
static void *SifRpcA03_handler(int function, void *buffer, int nbytes){
	((struct SecrSifDownloadGetKbitParams*)buffer)->result=SecrDownloadGetKbit(((struct SecrSifDownloadGetKbitParams*)buffer)->port, ((struct SecrSifDownloadGetKbitParams*)buffer)->slot, ((struct SecrSifDownloadGetKbitParams*)buffer)->kbit);
	return buffer;
}

//0x00000284
static void *SifRpcA04_handler(int function, void *buffer, int nbytes){
	((struct SecrSifDownloadGetKcParams*)buffer)->result=SecrDownloadGetKc(((struct SecrSifDownloadGetKcParams*)buffer)->port, ((struct SecrSifDownloadGetKcParams*)buffer)->slot, ((struct SecrSifDownloadGetKcParams*)buffer)->kc);
	return buffer;
}

//0x00000358
static void *SifRpcA05_handler(int function, void *buffer, int nbytes){
	((struct SecrSifDownloadGetIcvps2Params*)buffer)->result=SecrDownloadGetICVPS2(((struct SecrSifDownloadGetIcvps2Params*)buffer)->icvps2);
	return buffer;
}

//0x00000424
static void *SifRpcA06_handler(int function, void *buffer, int nbytes){
	((struct SecrSifDiskBootHeaderParams*)buffer)->result=SecrDiskBootHeader(((struct SecrSifDiskBootHeaderParams*)buffer)->buffer, &((struct SecrSifDiskBootHeaderParams*)buffer)->BitTable, &((struct SecrSifDiskBootHeaderParams*)buffer)->size);
	return buffer;
}

//0x000004f8
static void *SifRpcA07_handler(int function, void *buffer, int nbytes){
	((struct SecrSifDiskBootBlockParams*)buffer)->result=SecrDiskBootBlock(((struct SecrSifDiskBootBlockParams*)buffer)->source, ((struct SecrSifDiskBootBlockParams*)buffer)->destination, ((struct SecrSifDiskBootBlockParams*)buffer)->size);
	printf("sif_disk_boot_block %d %d\n", ((struct SecrSifDiskBootBlockParams*)buffer)->source, ((struct SecrSifDiskBootBlockParams*)buffer)->result);

	return buffer;
}

//0x00000044
static void func_00000044(void *parameters){
	if(!sceSifCheckInit()){
		printf("yet sif hasn't been init\n");
		sceSifInit();
	}

	sceSifInitRpc(0);
	sceSifSetRpcQueue(&SifQueueData01, GetThreadId());
	sceSifRegisterRpc(&SifServerData01, 0x80000A01, &SifRpcA01_handler, SifServerBuffer01, NULL, NULL, &SifQueueData01);
	sceSifRpcLoop(&SifQueueData01);
}

//0x00000114
static void func_00000114(void *parameters){
	if(!sceSifCheckInit()){
		printf("yet sif hasn't been init\n");
		sceSifInit();
	}

	sceSifInitRpc(0);
	sceSifSetRpcQueue(&SifQueueData02, GetThreadId());
	sceSifRegisterRpc(&SifServerData02, 0x80000A02, &SifRpcA02_handler, SifServerBuffer01, NULL, NULL, &SifQueueData02);
	sceSifRpcLoop(&SifQueueData02);
}

//0x000001e8
static void func_000001e8(void *parameters){
	if(!sceSifCheckInit()){
		printf("yet sif hasn't been init\n");
		sceSifInit();
	}

	sceSifInitRpc(0);
	sceSifSetRpcQueue(&SifQueueData03, GetThreadId());
	sceSifRegisterRpc(&SifServerData03, 0x80000A03, &SifRpcA03_handler, SifServerBuffer01, NULL, NULL, &SifQueueData03);
	sceSifRpcLoop(&SifQueueData03);
}

//0x000002bc
static void func_000002bc(void *parameters){
	if(!sceSifCheckInit()){
		printf("yet sif hasn't been init\n");
		sceSifInit();
	}

	sceSifInitRpc(0);
	sceSifSetRpcQueue(&SifQueueData04, GetThreadId());
	sceSifRegisterRpc(&SifServerData04, 0x80000A04, &SifRpcA04_handler, SifServerBuffer01, NULL, NULL, &SifQueueData04);
	sceSifRpcLoop(&SifQueueData04);
}

//0x00000388
static void func_00000388(void *parameters){
	if(!sceSifCheckInit()){
		printf("yet sif hasn't been init\n");
		sceSifInit();
	}

	sceSifInitRpc(0);
	sceSifSetRpcQueue(&SifQueueData05, GetThreadId());
	sceSifRegisterRpc(&SifServerData05, 0x80000A05, &SifRpcA05_handler, SifServerBuffer01, NULL, NULL, &SifQueueData05);
	sceSifRpcLoop(&SifQueueData05);
}

//0x0000045c
static void func_0000045c(void *parameters){
	if(!sceSifCheckInit()){
		printf("yet sif hasn't been init\n");
		sceSifInit();
	}

	sceSifInitRpc(0);
	sceSifSetRpcQueue(&SifQueueData06, GetThreadId());
	sceSifRegisterRpc(&SifServerData06, 0x80000A06, &SifRpcA06_handler, SifServerBuffer01, NULL, NULL, &SifQueueData06);
	sceSifRpcLoop(&SifQueueData06);
}

//0x00000544
static void func_00000544(void *parameters){
	if(!sceSifCheckInit()){
		printf("yet sif hasn't been init\n");
		sceSifInit();
	}

	sceSifInitRpc(0);
	sceSifSetRpcQueue(&SifQueueData07, GetThreadId());
	sceSifRegisterRpc(&SifServerData07, 0x80000A07, &SifRpcA07_handler, SifServerBuffer01, NULL, NULL, &SifQueueData07);
	sceSifRpcLoop(&SifQueueData07);
}

//0x000005e0
int _start(int argc, char *argv[]){
	int result;
	iop_thread_t thread;

	if(RegisterLibraryEntries(&_exp_secrsif)==0){
		thread.attr=TH_C;
		thread.thread=&func_00000044;
		thread.priority=0x28;
		thread.stacksize=0x800;
		if((SifRpcThread01=CreateThread(&thread))!=0){
			StartThread(SifRpcThread01, NULL);

			thread.thread=&func_00000114;
			if((SifRpcThread02=CreateThread(&thread))!=0){
				StartThread(SifRpcThread02, NULL);

				thread.thread=&func_000001e8;
				if((SifRpcThread03=CreateThread(&thread))!=0){
					StartThread(SifRpcThread03, NULL);

					thread.thread=&func_000002bc;
					if((SifRpcThread04=CreateThread(&thread))!=0){
						StartThread(SifRpcThread04, NULL);

						thread.thread=&func_00000388;
						if((SifRpcThread05=CreateThread(&thread))!=0){
							StartThread(SifRpcThread05, NULL);

							thread.thread=&func_0000045c;
							if((SifRpcThread06=CreateThread(&thread))!=0){
								StartThread(SifRpcThread06, NULL);

								thread.thread=&func_00000544;
								if((SifRpcThread07=CreateThread(&thread))!=0){
									StartThread(SifRpcThread07, NULL);

									result=MODULE_RESIDENT_END;
								}
								else result=MODULE_NO_RESIDENT_END;
							}
							else result=MODULE_NO_RESIDENT_END;
						}
						else result=MODULE_NO_RESIDENT_END;
					}
					else result=MODULE_NO_RESIDENT_END;
				}
				else result=MODULE_NO_RESIDENT_END;
			}
			else result=MODULE_NO_RESIDENT_END;
		}
		else result=MODULE_NO_RESIDENT_END;
	}
	else result=MODULE_NO_RESIDENT_END;

	return result;
}
