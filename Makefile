EE_BIN = FMCBInstaller.elf

#IOP modules
EE_IOP_OBJS = UDNL_irx.o IOMANX_irx.o FILEXIO_irx.o SIO2MAN_irx.o PADMAN_irx.o MCMAN_irx.o MCSERV_irx.o SECRSIF_irx.o MCTOOLS_irx.o USBD_irx.o USBHDFSD_irx.o POWEROFF_irx.o DEV9_irx.o ATAD_irx.o HDD_irx.o PFS_irx.o #UDPTTY_irx.o
IRX_DIR = irx/compiled

EE_RES_OBJS = background.o pad_layout.o
EE_OBJS = main.o ioprstctrl.o modutils.o UI.o menu.o libsecr.o pad.o system.o graphics.o ReqSpaceCalc.o FreeTypeSupport.o $(EE_RES_OBJS) $(EE_IOP_OBJS) mctools_rpc.o

EE_INCS := -I$(PS2SDK)/ports/include -I$(PS2SDK)/ee/include -I$(PS2SDK)/common/include -I./irx/source/secrsif/src -I./irx/source/mctools
EE_LDFLAGS := -L$(PS2SDK)/ports/lib -L$(PS2SDK)/ee/lib -Tlinkfile -s
EE_LIBS = -lgs -lpng -lz -lm -lfreetype -lcdvd -lmc -lpadx -lhdd -lfileXio -lpatches -lc -lkernel
EE_GPVAL = -G8192
EE_CFLAGS += -Os -mgpopt $(EE_GPVAL)

%.o : %.c
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

%.o : %.S
	$(EE_CC) $(EE_CFLAGS) $(EE_INCS) -c $< -o $@

%.o : %.s
	$(EE_AS) $(EE_ASFLAGS) $< -o $@

$(EE_BIN) : $(EE_OBJS) $(PS2SDK)/ee/startup/crt0.o
	$(EE_CC) $(EE_CFLAGS) -mno-crt0 $(EE_LDFLAGS) -o $(EE_BIN) $(EE_OBJS) $(PS2SDK)/ee/startup/crt0.o $(EE_LIBS)

all:
	$(MAKE) $(EE_BIN)

clean:
	rm -f $(EE_BIN) $(EE_BIN_REL) $(EE_OBJS)

background.o:
	bin2o $(EE_GPVAL) resources/background.png background.o background

pad_layout.o:
	bin2o $(EE_GPVAL) resources/pad_layout.png pad_layout.o pad_layout

POWEROFF_irx.o:
	bin2o $(EE_GPVAL) $(PS2SDK)/iop/irx/poweroff.irx POWEROFF_irx.o POWEROFF_irx

DEV9_irx.o:
	bin2o $(EE_GPVAL) $(PS2SDK)/iop/irx/ps2dev9.irx DEV9_irx.o DEV9_irx

UDPTTY_irx.o:
	bin2o $(EE_GPVAL) $(PS2SDK)/iop/irx/udptty.irx UDPTTY_irx.o UDPTTY_irx

IOMANX_irx.o:
	bin2o $(EE_GPVAL) $(PS2SDK)/iop/irx/iomanX.irx IOMANX_irx.o IOMANX_irx

FILEXIO_irx.o:
	bin2o $(EE_GPVAL) $(PS2SDK)/iop/irx/fileXio.irx FILEXIO_irx.o FILEXIO_irx

SIO2MAN_irx.o:
	bin2o $(EE_GPVAL) $(PS2SDK)/iop/irx/freesio2.irx SIO2MAN_irx.o SIO2MAN_irx

PADMAN_irx.o:
	bin2o $(EE_GPVAL) $(PS2SDK)/iop/irx/freepad.irx PADMAN_irx.o PADMAN_irx

MCMAN_irx.o:
	bin2o $(EE_GPVAL) $(IRX_DIR)/mcman.irx MCMAN_irx.o MCMAN_irx

MCSERV_irx.o:
	bin2o $(EE_GPVAL) $(PS2SDK)/iop/irx/mcserv.irx MCSERV_irx.o MCSERV_irx

SECRSIF_irx.o:
	bin2o $(EE_GPVAL) $(IRX_DIR)/secrsif.irx SECRSIF_irx.o SECRSIF_irx

MCTOOLS_irx.o:
	bin2o $(EE_GPVAL) $(IRX_DIR)/mctools.irx MCTOOLS_irx.o MCTOOLS_irx

USBD_irx.o:
	bin2o $(EE_GPVAL) $(PS2SDK)/iop/irx/usbd.irx USBD_irx.o USBD_irx

USBHDFSD_irx.o:
	bin2o $(EE_GPVAL) $(PS2SDK)/iop/irx/usbhdfsd.irx USBHDFSD_irx.o USBHDFSD_irx

UDNL_irx.o:
	bin2o $(EE_GPVAL) $(IRX_DIR)/udnl.irx UDNL_irx.o UDNL_irx

ATAD_irx.o:
	bin2o $(EE_GPVAL) $(PS2SDK)/iop/irx/ps2atad.irx ATAD_irx.o ATAD_irx

HDD_irx.o:
	bin2o $(EE_GPVAL) $(PS2SDK)/iop/irx/ps2hdd.irx HDD_irx.o HDD_irx

PFS_irx.o:
	bin2o $(EE_GPVAL) $(PS2SDK)/iop/irx/ps2fs.irx PFS_irx.o PFS_irx

include $(PS2SDK)/samples/Makefile.pref
