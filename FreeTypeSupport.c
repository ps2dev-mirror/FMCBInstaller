#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <kernel.h>
#include <wchar.h>

#include <libgs.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "graphics.h"
#include "FreeTypeSupport.h"

struct GSGlyphSlot{
	wint_t character;
	unsigned int age;		// 0 = unused.
	unsigned short VramPageX, VramPageY;
	short int OffsetY, OffsetX;	//Offset X and Y values for drawing the character (Refer to the FreeType documentation).
};

struct FreeTypeAtlasVramPage{
	u32 vram;
	void *buffer;
	struct GSGlyphSlot GlyphSlots[NUM_ATLAS_ROWS][NUM_ATLAS_COLUMNS];
};

typedef struct FreeTypeFont{
	FT_Face FTFace;
	GS_IMAGE Texture;
	GS_IMAGE Clut;
	void *ClutMem;
	unsigned int IsLoaded;
	struct FreeTypeAtlasVramPage VramPages[NUM_VRAM_PAGES];
} GS_FreeTypeFont_t;

static GS_FreeTypeFont_t GS_FTFont;

int ResetFreeType(struct UIDrawGlobal *gsGlobal){
	unsigned short int i, x, y;
	struct GSGlyphSlot *GSGlyphSlot;
	short int width_aligned;
	int result;

	if(GS_FTFont.IsLoaded){
		result=0;

		GS_FTFont.Clut.x = 0;
		GS_FTFont.Clut.y = 0;
		GS_FTFont.Clut.width = 16;
		GS_FTFont.Clut.height = 16;
		GS_FTFont.Clut.vram_width = 1;	//width (16) / 64 = 1

		GS_FTFont.Clut.vram_addr = GsVramAllocTextureBuffer(GS_FTFont.Clut.width, GS_FTFont.Clut.height, GS_FTFont.Clut.psm);
		UploadClut(gsGlobal, &GS_FTFont.Clut, GS_FTFont.ClutMem);

		GS_FTFont.Texture.x = 0;
		GS_FTFont.Texture.y = 0;
		width_aligned = (GS_FTFont.Texture.width+127)&~127;
		GS_FTFont.Texture.vram_width = width_aligned / 64;
		for(i=0; i<NUM_VRAM_PAGES; i++){
			GS_FTFont.VramPages[i].vram = GsVramAllocTextureBuffer(width_aligned, (GS_FTFont.Texture.height+127)&~127, GS_FTFont.Texture.psm);
			for(y=0; y<NUM_ATLAS_ROWS; y++){
				for(x=0; x<NUM_ATLAS_COLUMNS; x++){
					GSGlyphSlot=&GS_FTFont.VramPages[i].GlyphSlots[y][x];
					GSGlyphSlot->age=0;
					GSGlyphSlot->character=0;
				}
			}
		}
	}
	else result=-1;

	return result;
}

static int InitFreeTypeSupportCommon(struct UIDrawGlobal *gsGlobal, GS_FreeTypeFont_t *GS_FTFont){
	int result;
	unsigned int TextureSizeEE;
	unsigned short int i, y, x;

	if((result=FT_Set_Pixel_Sizes(GS_FTFont->FTFace, FONT_WIDTH, FONT_HEIGHT))==0){
		GS_FTFont->Texture.width=NUM_ATLAS_COLUMNS*FONT_WIDTH;
		GS_FTFont->Texture.height=NUM_ATLAS_ROWS*FONT_HEIGHT;
		GS_FTFont->Texture.psm = GS_TEX_8;
		GS_FTFont->Clut.psm = GS_CLUT_32;

		// generate the clut table
		u32 *clut = memalign(128, 256 * 4);
		GS_FTFont->ClutMem=clut;
		for (i = 0; i < 256; ++i) clut[i] = (i > 0x80 ? 0x80 : i) << 24 | i << 16 | i << 8 | i;
		SyncDCache(clut, (void*)((unsigned int)clut+256*4));

		TextureSizeEE = ((GS_FTFont->Texture.width+127)&~127) * ((GS_FTFont->Texture.height+127)&~127);
		for(i=0,result=0; i<NUM_VRAM_PAGES; i++){
			if((GS_FTFont->VramPages[i].buffer=memalign(128, TextureSizeEE))==NULL){
				result=-ENOMEM;
				break;
			}

			for(y=0; y<NUM_ATLAS_ROWS; y++){
				for(x=0; x<NUM_ATLAS_COLUMNS; x++){
					GS_FTFont->VramPages[i].GlyphSlots[y][x].VramPageX=x*FONT_WIDTH;
					GS_FTFont->VramPages[i].GlyphSlots[y][x].VramPageY=y*FONT_HEIGHT;
				}
			}
		}

		if(result==0){
			GS_FTFont->IsLoaded=1;
			ResetFreeType(gsGlobal);
		}
	}

	return result;
}

int InitFreeType(struct UIDrawGlobal *gsGlobal, const char *FontFile){
	FT_Library FTLibrary;
	int result;

	memset(&GS_FTFont, 0, sizeof(GS_FreeTypeFont_t));

	if((result=FT_Init_FreeType(&FTLibrary))==0){
		if((result=FT_New_Face(FTLibrary, FontFile, 0, &GS_FTFont.FTFace))==0){
			result=InitFreeTypeSupportCommon(gsGlobal, &GS_FTFont);
		}

		if(result!=0) DeinitFreeType();
	}

	return result;
}

int InitFreeTypeWithBuffer(struct UIDrawGlobal *gsGlobal, void *buffer, unsigned int size){
	FT_Library FTLibrary;
	int result;

	memset(&GS_FTFont, 0, sizeof(GS_FreeTypeFont_t));

	if((result=FT_Init_FreeType(&FTLibrary))==0){
		if((result=FT_New_Memory_Face(FTLibrary, buffer, size, 0, &GS_FTFont.FTFace))==0){
			result=InitFreeTypeSupportCommon(gsGlobal, &GS_FTFont);
		}

		if(result!=0) DeinitFreeType();
	}

	return result;
}

void DeinitFreeType(void)
{
	unsigned int i;

	FT_Done_Face(GS_FTFont.FTFace);

	for(i=0; i<NUM_VRAM_PAGES; i++){
		if(GS_FTFont.VramPages[i].buffer!=NULL) free(GS_FTFont.VramPages[i].buffer);
	}

	if(GS_FTFont.ClutMem!=NULL) free(GS_FTFont.ClutMem);

	memset(&GS_FTFont, 0, sizeof(GS_FreeTypeFont_t));
}

static inline int UploadFreeTypeChar(struct UIDrawGlobal *gsGlobal, GS_FreeTypeFont_t *GS_FTFont, FT_GlyphSlot GlyphSlot, struct GSGlyphSlot **GlyphSlotOut){
	struct GSGlyphSlot *GSGlyphSlot;
	unsigned char *FTCharRow;
	unsigned short int i, x, y, yOffset, LeastUsedSlotX, LeastUsedSlotY, LeastUsedSlotVRAMSlot;
	unsigned int LeastUsedSlotAge;
	int VRAMSlot;

	*GlyphSlotOut=NULL;

	//Choose one glyph slot.
	for(LeastUsedSlotAge=0,LeastUsedSlotX=LeastUsedSlotY=0,i=0,LeastUsedSlotVRAMSlot=VRAMSlot=0,GSGlyphSlot=NULL; i<NUM_VRAM_PAGES; i++){
		for(y=0; y<NUM_ATLAS_ROWS; y++){
			for(x=0; x<NUM_ATLAS_COLUMNS; x++){
				GSGlyphSlot=&GS_FTFont->VramPages[i].GlyphSlots[y][x];

				if(GSGlyphSlot->age==0){	//Use unused slots.
					VRAMSlot=i;
					goto UpdateAges;
				}
				else{
					if(LeastUsedSlotAge<GSGlyphSlot->age){
						LeastUsedSlotX=x;
						LeastUsedSlotY=y;
						LeastUsedSlotVRAMSlot=i;
					}
				}
			}
		}
	}

	if(i==NUM_VRAM_PAGES){
		VRAMSlot=LeastUsedSlotVRAMSlot;
		GSGlyphSlot=&GS_FTFont->VramPages[LeastUsedSlotVRAMSlot].GlyphSlots[LeastUsedSlotY][LeastUsedSlotX];
	}

UpdateAges:
	//Update ages.
	for(i=0; i<NUM_VRAM_PAGES; i++){
		for(y=0; y<NUM_ATLAS_ROWS; y++){
			for(x=0; x<NUM_ATLAS_COLUMNS; x++){
				if(&GS_FTFont->VramPages[i].GlyphSlots[y][x]!=GSGlyphSlot){
					if(GS_FTFont->VramPages[i].GlyphSlots[y][x].age>0) GS_FTFont->VramPages[i].GlyphSlots[y][x].age++;
					else goto VRAM_Page_Selected;	//Slots will be used in order. When an empty slot is encountered, the following slots will be all unused.
				}
			}
		}
	}

VRAM_Page_Selected:
	*GlyphSlotOut=GSGlyphSlot;

	//Initiate a texture flush before reusing the VRAM page, if the slot was just used earlier.
	GsTextureFlush();

	if(GSGlyphSlot->age>0) ExecSyncClear(gsGlobal);	//If the slot contains a previously used character (not transferred to VRAM yet), ensure that the character was already transferred to VRAM before reusing its host-side buffer.

	GSGlyphSlot->OffsetX=GlyphSlot->bitmap_left;
	GSGlyphSlot->OffsetY=GlyphSlot->bitmap_top;

	for(yOffset=0; yOffset<GlyphSlot->bitmap.rows; yOffset++){
		FTCharRow=(void*)UNCACHED_SEG(((unsigned int)GS_FTFont->VramPages[VRAMSlot].buffer+GSGlyphSlot->VramPageX+(GSGlyphSlot->VramPageY+yOffset)*FONT_WIDTH*NUM_ATLAS_COLUMNS));
		memcpy(FTCharRow, &((unsigned char*)GlyphSlot->bitmap.buffer)[yOffset*GlyphSlot->bitmap.width], GlyphSlot->bitmap.width);
		memset((void*)UNCACHED_SEG((unsigned int)FTCharRow+GlyphSlot->bitmap.width), 0, FONT_WIDTH-GlyphSlot->bitmap.width);
	}
	for(; yOffset<FONT_HEIGHT; yOffset++){
		FTCharRow=(void*)UNCACHED_SEG(((unsigned int)GS_FTFont->VramPages[VRAMSlot].buffer+GSGlyphSlot->VramPageX+(GSGlyphSlot->VramPageY+yOffset)*FONT_WIDTH*NUM_ATLAS_COLUMNS));
		memset(FTCharRow, 0, FONT_WIDTH);
	}

	GS_FTFont->Texture.vram_addr = GS_FTFont->VramPages[VRAMSlot].vram;
	GsLoadImage(GS_FTFont->VramPages[VRAMSlot].buffer, &GS_FTFont->Texture);
	GsTextureFlush();

	return VRAMSlot;
}

static inline void DrawFreeTypeChar(struct UIDrawGlobal *gsGlobal, GS_FreeTypeFont_t *GS_FTFont, wint_t character, float x, float y, int z, float scale, GS_RGBAQ colour){
	FT_GlyphSlot GlyphSlot=GS_FTFont->FTFace->glyph;
	struct GSGlyphSlot *GSGlyphSlot;
	unsigned int i, SlotX, SlotY;
	int VRAMPage;
	float XCoordinates, YCoordinates;

	if(GS_FTFont->IsLoaded){
		//Scan through all uploaded glyph slots.
		for(i=0,VRAMPage=NUM_VRAM_PAGES,GSGlyphSlot=NULL; i<NUM_VRAM_PAGES; i++){
			for(SlotY=0; SlotY<NUM_ATLAS_ROWS; SlotY++){
				for(SlotX=0; SlotX<NUM_ATLAS_COLUMNS; SlotX++){
					if(GS_FTFont->VramPages[i].GlyphSlots[SlotY][SlotX].character==character){
						VRAMPage=i;
						GSGlyphSlot=&GS_FTFont->VramPages[VRAMPage].GlyphSlots[SlotY][SlotX];
						GSGlyphSlot->age=1;
						goto VRAM_Page_Selected;
					}
				}
			}
		}

		if(VRAMPage==NUM_VRAM_PAGES){	//Not in VRAM? Upload it.
			if(FT_Load_Char(GS_FTFont->FTFace, character, FT_LOAD_RENDER)) return;
			if((VRAMPage=UploadFreeTypeChar(gsGlobal, GS_FTFont, GlyphSlot, &GSGlyphSlot))<0) return;
			GSGlyphSlot->character=character;
			GSGlyphSlot->age=1;
	//		printf("Uploading %c, %u, %u\n", character, GSGlyphSlot->VramPageX, GSGlyphSlot->VramPageY);
		}

VRAM_Page_Selected:
		GS_FTFont->Texture.vram_addr=GS_FTFont->VramPages[VRAMPage].vram;

		YCoordinates=y-GSGlyphSlot->OffsetY*scale+FONT_HEIGHT*scale;
		XCoordinates=x+GSGlyphSlot->OffsetX*scale;
		DrawSpriteTexturedClut(gsGlobal, &GS_FTFont->Texture, &GS_FTFont->Clut, XCoordinates, YCoordinates,
						GSGlyphSlot->VramPageX, GSGlyphSlot->VramPageY,
						XCoordinates+FONT_WIDTH*scale, YCoordinates+FONT_HEIGHT*scale,
						GSGlyphSlot->VramPageX+FONT_WIDTH, GSGlyphSlot->VramPageY+FONT_HEIGHT,
						z, colour);
	}
}

void FreeTypePrintfWithFeedback(struct UIDrawGlobal *gsGlobal, float x, float y, int z, float scale, GS_RGBAQ colour, const char *string, float *xRel, float *yRel)
{
	wchar_t wchar;
	float StartX, StartY;
	int charsize, bufmax;

	StartX = x;
	StartY = y;

	for(bufmax = strlen(string) + 1; *string != '\0'; string += charsize, bufmax -= charsize)
	{
		//Up to MB_CUR_MAX
		charsize = mbtowc(&wchar, string, bufmax);

		switch(wchar)
		{
			case '\r':
				x=StartX;
				break;
			case '\n':
				y+=FONT_HEIGHT*scale;
				x=StartX;
				break;
			case '\t':
				x+=(TAB_STOPS*FONT_WIDTH*scale)-(unsigned int)x%(unsigned int)(TAB_STOPS*FONT_WIDTH*scale);
				break;
			default:
				DrawFreeTypeChar(gsGlobal, &GS_FTFont, wchar, x, y, z, scale, colour);
				x+=FONT_WIDTH*scale;
		}
	}

	if(xRel != NULL)
		*xRel = x - StartX;
	if(yRel != NULL)
		*yRel = y - StartY;
}

void FreeTypePrintf(struct UIDrawGlobal *gsGlobal, float x, float y, int z, float scale, GS_RGBAQ colour, const char *string)
{
	return FreeTypePrintfWithFeedback(gsGlobal, x, y, z, scale, colour, string, NULL, NULL);
}
