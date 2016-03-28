#define NUM_ATLAS_COLUMNS	16
#define NUM_ATLAS_ROWS		16
#define FONT_WIDTH		16
#define FONT_HEIGHT		16
#define NUM_VRAM_PAGES		1		
#define TAB_STOPS		4

int InitFreeType(struct UIDrawGlobal *gsGlobal, const char *FontFile);
int InitFreeTypeWithBuffer(struct UIDrawGlobal *gsGlobal, void *buffer, unsigned int size);
void DeinitFreeType(void);
int ResetFreeType(struct UIDrawGlobal *gsGlobal);	//Performs a partial re-initialization of the FreeType library and re-allocates VRAM. Used when VRAM has been cleared.
void FreeTypePrintfWithFeedback(struct UIDrawGlobal *gsGlobal, float x, float y, int z, float scale, GS_RGBAQ colour, const char *string, float *xRel, float *yRel);
void FreeTypePrintf(struct UIDrawGlobal *gsGlobal, float x, float y, int z, float scale, GS_RGBAQ colour, const char *string);
