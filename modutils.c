#include <stdio.h>
#include <loadfile.h>
#include <iopheap.h>
#include <kernel.h>
#include <sifdma.h>
#include <sifrpc.h>
#include <string.h>

#include "modutils.h"

#define LF_PATH_MAX	252
#define LF_ARG_MAX	252

enum _lf_functions {
        LF_F_MOD_LOAD = 0,
        LF_F_ELF_LOAD,

        LF_F_SET_ADDR,
        LF_F_GET_ADDR,

        LF_F_MG_MOD_LOAD,
        LF_F_MG_ELF_LOAD,

        LF_F_MOD_BUF_LOAD,

        LF_F_MOD_STOP,
        LF_F_MOD_UNLOAD,

        LF_F_SEARCH_MOD_BY_NAME,
        LF_F_SEARCH_MOD_BY_ADDRESS,
};

extern SifRpcClientData_t _lf_cd; /* Declared in PS2SDK library */

struct _lf_module_buffer_load_arg {
	union {
		void	*ptr;
		int	result;
	} p;
	union {
		int	arg_len;
		int	modres;
	} q;
	char	unused[LF_PATH_MAX];
	char	args[LF_ARG_MAX];
} ALIGNED(16);

int _SifLoadModuleBufferAsync(void *ptr, int arg_len, const char *args, int *modres)
{
	struct _lf_module_buffer_load_arg arg;

	memset(&arg, 0, sizeof arg);

	arg.p.ptr = ptr;
	if (args && arg_len) {
		arg.q.arg_len = arg_len > LF_ARG_MAX ? LF_ARG_MAX : arg_len;
		memcpy(arg.args, args, arg.q.arg_len);
	} else {
		arg.q.arg_len = 0;
	}

	if (SifCallRpc(&_lf_cd, LF_F_MOD_BUF_LOAD, SIF_RPC_M_NOWAIT, &arg, sizeof arg, &arg, 8,
				NULL, NULL) < 0)
		return -E_SIF_RPC_CALL;

	if (modres)
		*modres = arg.q.modres;

	return arg.p.result;
}

int SifExecModuleBufferAsync(void *ptr, u32 size, u32 arg_len, const char *args, int *mod_res)
{
	SifDmaTransfer_t dmat;
	void *iop_addr;
	int res;
	unsigned int qid;

	/* Round the size up to the nearest 16 bytes. */
	size = (size + 15) & -16;

	if (!(iop_addr = SifAllocIopHeap(size)))
		return -E_IOP_NO_MEMORY;

	dmat.src = ptr;
	dmat.dest = iop_addr;
	dmat.size = size;
	dmat.attr = 0;
//	SifWriteBackDCache(ptr, size);
	qid = SifSetDma(&dmat, 1);

	if (!qid)
	    return -1; // should have a better error here...

	while(SifDmaStat(qid) >= 0);

	res = _SifLoadModuleBufferAsync(iop_addr, arg_len, args, mod_res);
//	SifFreeIopHeap(iop_addr);

	return res;
}
