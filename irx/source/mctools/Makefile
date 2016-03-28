IOP_BIN  = mctools.irx
IOP_OBJS = mctools.o MCIO.o cache.o imports.o

IOP_CFLAGS += -Wall -fno-builtin -mgpopt -G8192 -O2 -mno-check-zero-division
IOP_LDFLAGS += -s
IOP_INCS += -I$(PS2SDK)/iop/include -I$(PS2SDK)/common/include

all: $(IOP_BIN)

clean:
	rm -f $(IOP_BIN) $(IOP_OBJS)

include $(PS2SDK)/Defs.make
include Rules.make
