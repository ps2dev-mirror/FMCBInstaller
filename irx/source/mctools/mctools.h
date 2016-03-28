/* RPC function numbers */
#define MCTOOLS_CreateCrossLinkedEnts	0x00
#define MCTOOLS_DeleteCrossLinkedEnts	0x01
#define MCTOOLS_GetMCInfo		0x02
#define MCTOOLS_ReadCluster		0x03
#define MCTOOLS_WriteBlock		0x04
#define MCTOOLS_Init			0x80
#define MCTOOLS_InitPageCache		0x81
#define MCTOOLS_FlushPageCache		0x82
#define MCTOOLS_FlushMCMANClusterCache	0x83

#define MCTOOLS_RPC_NUM		0x00001906

#define MAX_RPC_FILE_ENTRIES	8

/* 340 bytes long */
struct SCE_MC_Superblock{
	unsigned char magic[28];	/* "Sony PS2 Memory Card Format" */
	unsigned char version[12];	/* E.g. "1.X.0.0". Version 1.2 indicates full support for the bad_block_list */
	unsigned short int page_len;	/* Length of each page in bytes. */
	unsigned short int pages_per_cluster;
	unsigned short int pages_per_block;/* Must be 16 (?) */
	unsigned short int unknown;	/* Unknown. Value: 0xFF00 */
	unsigned int clusters;		/* Total size of the card in clusters. */
	unsigned int alloc_start;	/* The cluster number of the first allocatable cluster (The cluster immediately after the FAT) */
	unsigned int alloc_end;		/* The cluster number of the cluster after the highest-allocatable cluster. Not used. */
	unsigned int rootdir_cluster;	/* The first cluster representing the root directory relative to alloc_start. Must be zero. */
	unsigned int backup_block1;	/* Erase block used as a backup area during programming. */
	unsigned int backup_block2;	/* A block with it's content set to 1 during programming. */
	unsigned char unknown2[8];	/* Unused/unknown region. */
	unsigned int ifc_list[32];	/* List of indirect FAT clusters (On a standard 8M card there's only one indirect FAT cluster). */
	unsigned int bad_block_list[32];/* List of erase blocks that have errors and shouldn't be used. */
	unsigned char card_type;	/* Memory card type (2 = Playstation 2 Memory Card). */
	unsigned char card_flags;	/* Physical characteristics of the memory card. */
};

/* 512 bytes long (Occupies one sector). 96 bytes long here. */
struct SCE_MC_dirent{
	unsigned short int mode;	/* Attributes of this directory entry. */
	unsigned short int unknown;
	unsigned int length;		/* Length in bytes if a file, or entries if a directory. */
	unsigned char created[8];	/* Creation time. */
	unsigned int cluster;		/* First cluster of the file, or 0xFFFFFFFF for an empty file. In "." entries this the first cluster of this directory's parent directory instead. Relative to alloc_offset. */
	unsigned int dir_entry;		/* Only in "." entries. Entry of this directory in its parent's directory. */
	unsigned char modified[8];	/* Modification time. */
	unsigned int attr;		/* User defined attribute */
	unsigned char unknown2[28];
	unsigned char name[32];		/* Zero terminated name for this directory entry. */
};

struct FileAlias{
	unsigned short int mode;
	char name[66];	//Note: Do not just update this! This affects the size of each record within the uninstallation file, and changing this may break older installers that expect the original size. Instead, update this and the uninstallation file's version number together to lock out the older installers.
};

/* RPC data structures. */
struct MCTools_MCMANFlushCommandData{
	int port, slot;
};

struct MCTools_CrossLinkedEntRPCData{
	unsigned char port;
	unsigned char slot;
	char path[256];
	struct FileAlias Aliases[MAX_RPC_FILE_ENTRIES];
};

struct MCTools_McSpecData{
	unsigned short int PageSize, BlockSize;
	unsigned int CardSize;
};

struct MCTools_GetMCInfoRPCReqData{
	unsigned char port;
	unsigned char slot;
	struct MCTools_McSpecData *SpecData;	//On the EE
};

struct MCTools_GetMCInfoRPCResData{
	int result;
	struct MCTools_McSpecData SpecData;
};

struct MCTools_ReadWriteRPCData{
	int port, slot;
	unsigned short int PageSize, BlockSize;	//Or cluster size, when reading.
	unsigned int offset;	/* In blocks, clusters or pages. Depending on the RPC function used. */
	void *EEbuffer;
	void *IOPbuffer;
};

struct MCTools_CacheRPCData{
	unsigned short port;
	unsigned short slot;
	struct MCTools_McSpecData SpecData;
};

struct MCTools_AlignmentData{
	void *buffer1Address;
	void *buffer2Address;

	unsigned char buffer1[64];
	unsigned char buffer2[64];

	unsigned int buffer1_len;
	unsigned int buffer2_len;
};
