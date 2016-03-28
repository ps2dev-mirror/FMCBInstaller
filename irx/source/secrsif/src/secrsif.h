struct SecrSifDownloadHeaderParams{
	s32 port, slot;
	u8 buffer[0x400];		//0x008
	SecrBitTable_t BitTable;	//0x408
	s32 size;			//0x808
	s32 result;			//0x80C
};

struct SecrSifDownloadBlockParams{
	u8 buffer[0x400];
	s32 size;		//0x400
	s32 result;		//0x404
};

struct SecrSifDownloadGetKbitParams{
	s32 port, slot;
	u8 kbit[16];	//0x08
	s32 result;	//0x18
};

struct SecrSifDownloadGetKcParams{
	s32 port, slot;
	u8 kc[16];	//0x08
	s32 result;	//0x18
};

struct SecrSifDownloadGetIcvps2Params{
	u8 icvps2[8];	//0x00
	s32 result;	//0x08
};

struct SecrSifDiskBootHeaderParams{
	u8 buffer[0x400];
	SecrBitTable_t BitTable;	//0x400
	s32 size;		//0x800
	s32 result;		//0x804
};

struct SecrSifDiskBootBlockParams{
	u8 source[0x400];
	u8 destination[0x400];	//0x400
	s32 size;		//0x800
	s32 result;		//0x804
};
