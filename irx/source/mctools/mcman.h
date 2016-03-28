/*
  Copyright 2009-2010, jimmikaelkael
  Licenced under Academic Free License version 3.0
  Review Open PS2 Loader README & LICENSE files for further details.
*/

#ifndef __MCMAN_H__
#define __MCMAN_H__

//#define USING_MCMAN

// modInfo struct returned by xmcman exports 42
struct modInfo_t { 
        const char *name;
        u16 version;
};

typedef struct _sceMcStDateTime {
        u8  Resv2;
        u8  Sec;
        u8  Min;
        u8  Hour;
        u8  Day;
        u8  Month;
        u16 Year;
} sceMcStDateTime;

/* MCMAN public structure */
typedef struct _sceMcTblGetDir { //size = 64
        sceMcStDateTime _Create; // 0
        sceMcStDateTime _Modify; // 8
        u32 FileSizeByte;                // 16  
        u16 AttrFile;                    // 20
        u16 Reserve1;                    // 22
        u32 Reserve2;                    // 24
        u32 PdaAplNo;                    // 28
        u8  EntryName[32];               // 32
} sceMcTblGetDir;

typedef struct _MCCacheEntry {
	int  cluster;   // 0
	u8  *cl_data;   // 4
	u16  mc_slot;   // 8
	u8   wr_flag;   // 10
	u8   mc_port;   // 11
	u8   rd_flag;   // 12
	u8   unused[3]; // 13
} McCacheEntry;

// Card Flags
#define CF_USE_ECC			0x01
#define CF_BAD_BLOCK			0x08
#define CF_ERASE_ZEROES			0x10

#define MCMAN_MAXSLOT			4
#define MCMAN_CLUSTERSIZE		1024
#define MCMAN_CLUSTERFATENTRIES         256

typedef struct {				// size = 512
	u16 mode;					// 0
	u16 unused;					// 2	
	u32 length;					// 4
	sceMcStDateTime created;	// 8
	u32 cluster;				// 16
	u32 dir_entry;				// 20
	sceMcStDateTime modified;	// 24
	u32 attr;					// 32
	u32 unused2[7];				// 36
	u8  name[32];				// 64
	u8  unused3[416];			// 96
} McFsEntry;

/* MCMAN EXPORTS */
#ifdef USING_MCMAN
#define mcman_IMPORTS_start DECLARE_IMPORT_TABLE(mcman, 1, 1)
#else
#define mcman_IMPORTS_start DECLARE_IMPORT_TABLE(mcman, 2, 3)
#endif
#define mcman_IMPORTS_end END_IMPORT_TABLE

/* 05 */ int  McDetectCard(int port, int slot);
#define	I_McDetectCard DECLARE_IMPORT(5, McDetectCard)
/* 06 */ int  McOpen(int port, int slot, char *filename, int flags);
#define	I_McOpen DECLARE_IMPORT(6, McOpen)
/* 07 */ int  McClose(int fd);
#define	I_McClose DECLARE_IMPORT(7, McClose)
/* 08 */ int  McRead(int fd, void *buf, int length);
#define	I_McRead DECLARE_IMPORT(8, McRead)
/* 09 */ int  McWrite(int fd, void *buf, int length);
#define	I_McWrite DECLARE_IMPORT(9, McWrite)
/* 10 */ int  McSeek(int fd, int offset, int origin);
#define	I_McSeek DECLARE_IMPORT(10, McSeek)
/* 11 */ int  McFormat(int port, int slot);
#define	I_McFormat DECLARE_IMPORT(11, McFormat)
/* 12 */ int  McGetDir(int port, int slot, char *dirname, int flags, int maxent, sceMcTblGetDir *info);
#define	I_McGetDir DECLARE_IMPORT(12, McGetDir)
/* 13 */ int  McDelete(int port, int slot, char *filename, int flags);
#define	I_McDelete DECLARE_IMPORT(13, McDelete)
/* 14 */ int  McFlush(int fd);
#define	I_McFlush DECLARE_IMPORT(14, McFlush)
/* 15 */ int  McChDir(int port, int slot, char *newdir, char *currentdir);
#define	I_McChDir DECLARE_IMPORT(15, McChDir)
/* 16 */ int  McSetFileInfo(int port, int slot, char *filename, sceMcTblGetDir *info, int flags);
#define	I_McSetFileInfo DECLARE_IMPORT(16, McSetFileInfo)
#ifdef USING_MCMAN
/* 17 */ int McEraseBlock(int port, int block, void **pagebuf, void *eccbuf); // Yes, rom0:MCMAN doesn't have a 5th argument (The slot field)!
#define	I_McEraseBlock DECLARE_IMPORT(17, McEraseBlock)
#else
/* 17 */ int  McEraseBlock(int port, int slot, int block, void **pagebuf, void *eccbuf);
#define	I_McEraseBlock DECLARE_IMPORT(17, McEraseBlock)
#endif
/* 18 */ int  McReadPage(int port, int slot, int page, void *buf);
#define	I_McReadPage DECLARE_IMPORT(18, McReadPage)
/* 19 */ int  McWritePage(int port, int slot, int page, void *pagebuf, void *eccbuf);
#define	I_McWritePage DECLARE_IMPORT(19, McWritePage)
/* 20 */ void McDataChecksum(void *buf, void *ecc);
#define	I_McDataChecksum DECLARE_IMPORT(20, McDataChecksum);
/* 29 */ int  McReadPS1PDACard(int port, int slot, int page, void *buf);
#define	I_McReadPS1PDACard DECLARE_IMPORT(29,  McReadPS1PDACard)
/* 30 */ int  McWritePS1PDACard(int port, int slot, int page, void *buf);
#define	I_McWritePS1PDACard DECLARE_IMPORT(30, McWritePS1PDACard)
/* 36 */ int  McUnformat(int port, int slot);
#define	I_McUnformat DECLARE_IMPORT(36, McUnformat)
/* 37 */ int  McRetOnly(int fd);
#define	I_McRetOnly DECLARE_IMPORT(37, McRetOnly)
/* 38 */ int  McGetFreeClusters(int port, int slot);
#define	I_McGetFreeClusters DECLARE_IMPORT(38, McGetFreeClusters)
/* 39 */ int  McGetMcType(int port, int slot);
#define	I_McGetMcType DECLARE_IMPORT(39, McGetMcType)
/* 40 */ void McSetPS1CardFlag(int flag);
#define	I_McSetPS1CardFlag DECLARE_IMPORT(40, McSetPS1CardFlag)

#ifndef USING_MCMAN
/* Available in XMCMAN only */
/* 21 */ int  McDetectCard2(int port, int slot);
#define	I_McDetectCard2 DECLARE_IMPORT(21, McDetectCard2)
/* 22 */ int  McGetFormat(int port, int slot);
#define	I_McGetFormat DECLARE_IMPORT(22, McGetFormat)
/* 23 */ int  McGetEntSpace(int port, int slot, char *dirname);
#define	I_McGetEntSpace DECLARE_IMPORT(23, McGetEntSpace)
/* 24 */ int  McReplaceBadBlock(void);
#define	I_McReplaceBadBlock DECLARE_IMPORT(24, McReplaceBadBlock)
/* 25 */ int  McCloseAll(void);
#define	I_McCloseAll DECLARE_IMPORT(25, McCloseAll)
/* 42 */ struct modInfo_t *McGetModuleInfo(void);
#define	I_McGetModuleInfo DECLARE_IMPORT(42, McGetModuleInfo)
/* 43 */ int  McGetCardSpec(int port, int slot, u16 *pagesize, u16 *blocksize, int *cardsize, u8 *flags);
#define	I_McGetCardSpec DECLARE_IMPORT(43,  McGetCardSpec)
/* 44 */ int  McGetFATentry(int port, int slot, int fat_index, int *fat_entry);
#define	I_McGetFATentry DECLARE_IMPORT(44, McGetFATentry)
/* 45 */ int  McCheckBlock(int port, int slot, int block);
#define	I_McCheckBlock DECLARE_IMPORT(45,McCheckBlock)
/* 46 */ int  McSetFATentry(int port, int slot, int fat_index, int fat_entry);
#define	I_McSetFATentry DECLARE_IMPORT(46, McSetFATentry)
/* 47 */ int  McReadDirEntry(int port, int slot, int cluster, int fsindex, McFsEntry **pfse);
#define	I_McReadDirEntry DECLARE_IMPORT(47, McReadDirEntry)
/* 48 */ void Mc1stCacheEntSetWrFlagOff(void);
#define	I_Mc1stCacheEntSetWrFlagOff DECLARE_IMPORT(48, Mc1stCacheEntSetWrFlagOff)
/* 50 */int McReadCluster(int port, int slot, int cluster, McCacheEntry **pmce);
#define	I_McReadCluster DECLARE_IMPORT(50, McReadCluster)
/* 51 */int McFlushClusterCache(int port, int slot);
#define	I_McFlushClusterCache DECLARE_IMPORT(51, McFlushClusterCache)
/* 52 */int McSetDirEntryState(int port, int slot, int cluster, int fsindex, int flags);
#define	I_McSetDirEntryState DECLARE_IMPORT(52, McSetDirEntryState)

/* 62 */ int McInvalidateClusterCache(int port, int slot);	//Not part of MCMAN, but required for crosslinking hacks. Invalidates the cluster cache. Named "mcman_clearcache" within Jimmikaelkael's MCMAN module.
#define	I_McInvalidateClusterCache DECLARE_IMPORT(62, McInvalidateClusterCache)
#endif

// in addition to errno
#define EFORMAT                                           140

// MCMAN basic error codes
#define sceMcResSucceed               0
#define sceMcResChangedCard          -1
#define sceMcResNoFormat             -2
#define sceMcResFullDevice           -3
#define sceMcResNoEntry              -4
#define sceMcResDeniedPermit         -5
#define sceMcResNotEmpty             -6
#define sceMcResUpLimitHandle        -7
#define sceMcResFailReplace          -8
#define sceMcResFailResetAuth        -11
#define sceMcResFailDetect               -12
#define sceMcResFailDetect2              -13
#define sceMcResDeniedPS1Permit          -51
#define sceMcResFailAuth                 -90


// Memory Card device types
#define sceMcTypeNoCard               0
#define sceMcTypePS1                  1
#define sceMcTypePS2                  2
#define sceMcTypePDA                  3

/* High-Level File I/O */
#define SCE_CST_MODE                  0x01
#define SCE_CST_ATTR                  0x02
#define SCE_CST_SIZE                  0x04
#define SCE_CST_CT                    0x08
#define SCE_CST_AT                    0x10
#define SCE_CST_MT                    0x20
#define SCE_CST_PRVT                  0x40

#define SCE_STM_R                     0x01
#define SCE_STM_W                     0x02
#define SCE_STM_X                     0x04
#define SCE_STM_C                     0x08
#define SCE_STM_F                     0x10
#define SCE_STM_D                     0x20

/* file attributes */
#define sceMcFileAttrReadable         SCE_STM_R
#define sceMcFileAttrWriteable        SCE_STM_W
#define sceMcFileAttrExecutable       SCE_STM_X
#define sceMcFileAttrDupProhibit      SCE_STM_C
#define sceMcFileAttrFile             SCE_STM_F
#define sceMcFileAttrSubdir           SCE_STM_D
#define sceMcFileCreateDir            0x0040
#define sceMcFileAttrClosed           0x0080
#define sceMcFileCreateFile           0x0200
#define sceMcFile0400                 0x0400
#define sceMcFileAttrPDAExec          0x0800
#define sceMcFileAttrPS1              0x1000
#define sceMcFileAttrHidden           0x2000
#define sceMcFileAttrExists           0x8000

#endif
