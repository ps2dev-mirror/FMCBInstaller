# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
# $Id$

IOP_OBJS_DIR = obj/
IOP_BIN_DIR = bin/
IOP_SRC_DIR = src/
IOP_INC_DIR = include/

IOP_BIN  = bin/udnl.irx
IOP_OBJS = obj/udnl.o obj/udnl_asm.o obj/imports.o obj/IOPRP.c

IOP_INCS += -I$(PS2SDKSRC)/iop/kernel/include -I$(PS2SDKSRC)/common/include -I$(IOP_SRC_DIR)/include
IOP_CFLAGS += -Wall -fno-builtin -Os -mgpopt -G8192
IOP_LDFLAGS += -s

all: $(IOP_OBJS_DIR) $(IOP_BIN_DIR) $(IOP_BIN)

clean:
	rm -f -r $(IOP_OBJS_DIR) $(IOP_BIN_DIR)

$(IOP_OBJS_DIR)IOPRP.c:
	bin2c $(IOP_SRC_DIR)IOPRP.img $(IOP_OBJS_DIR)IOPRP.c IOPRP_img

include $(PS2SDKSRC)/Defs.make
include $(PS2SDKSRC)/iop/Rules.make
include $(PS2SDKSRC)/iop/Rules.release
