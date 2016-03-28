#include <kernel.h>
#include <libpad.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <osd_config.h>
#include <limits.h>
#include <wchar.h>

#include <libgs.h>

#include "main.h"
#include "system.h"
#include "pad.h"
#include "graphics.h"
#include "FreeTypeSupport.h"
#include "UI.h"

extern int errno __attribute__((section("data")));

struct UIDrawGlobal UIDrawGlobal;
GS_IMAGE BackgroundTexture;
GS_IMAGE PadLayoutTexture;

#ifdef UI_FONT_MEM
static void *gFontBuffer = NULL;
static int gFontBufferSize;
#endif
static unsigned short int SelectButton, CancelButton;

#define NUM_SUPPORTED_LANGUAGES	8

static int language=LANGUAGE_ENGLISH;

#include "lang.c"

static char *ExtLangStringTable[SYS_UI_MSG_COUNT];
static char *ExtLangLblStringTable[SYS_UI_LBL_COUNT];

static void UnloadLanguage(void);

static int FormatLanguageStrings(const char *in, int len, char *out)
{
	wchar_t wchar1, wchar2;
	int ActualLength, CharLen1, CharLen2;

	ActualLength = 0;
	CharLen1 = mbtowc(&wchar1, in, len);
	while(CharLen1 > 0 && wchar1 != '\0')
	{
		if(((CharLen2 = mbtowc(&wchar2, in + CharLen1, len - CharLen1)) > 0) && (wchar1 == '\\' && wchar2 != '\0'))
		{
			switch(wchar2)
			{
				case 'n':
					*out = '\n';
					out++;
					ActualLength++;
					break;
				case 't':
					*out = '\t';
					out++;
					ActualLength++;
					break;
			}

			in += (CharLen1 + CharLen2);
			len -= (CharLen1 + CharLen2);
			CharLen2 = mbtowc(&wchar2, in, len);
		} else {
			memcpy(out, in, CharLen1);
			out += CharLen1;
			ActualLength += CharLen1;
			in += CharLen1;
			len -= CharLen1;
		}

		CharLen1 = CharLen2;
		wchar1 = wchar2;
	}
	*out = '\0';

	return(ActualLength + 1);
}

static int ParseLanguageFile(char **array, FILE *file, unsigned int ExpectedNumLines)
{
	int result, LinesLoaded, len;
	unsigned char BOMTemp[3];
	char line[256];

	if(fread(BOMTemp, 1, 3, file)!=3 || (BOMTemp[0]!=0xEF || BOMTemp[1]!=0xBB || BOMTemp[2]!=0xBF))
	{	//Check for the BOM byte sequence. Skip it, if it exists.
		rewind(file);
	}

	result=0;
	for(LinesLoaded = 0; fgets(line, sizeof(line), file) != NULL; LinesLoaded++)
	{
		len = strlen(line);

		if(len >= 1 && line[len - 1] == '\n')	//Remove the newline character, if it exists.
		{
			line[len - 1] = '\0';
			len--;
		}

		if((array[LinesLoaded] = malloc(len + 1)) != NULL)
		{
			array[LinesLoaded] = realloc(array[LinesLoaded], FormatLanguageStrings(line, len + 1, array[LinesLoaded]));
		} else {
			result=-ENOMEM;
			break;
		}
	}

	if(result == 0)
	{
		if(LinesLoaded != ExpectedNumLines)
		{
			DEBUG_PRINTF("ParseLanguageFile: Mismatched number of lines (%u/%d)\n", LinesLoaded, ExpectedNumLines);
			result = -1;
		}
	}

	return result;
}

static int ParseFontListFile(char **array, FILE *file, unsigned int ExpectedNumLines)
{
	int result, LinesLoaded, len;
	char line[256];

	result=0;
	for(LinesLoaded = 0; fgets(line, sizeof(line), file) != NULL; LinesLoaded++)
	{
		len = strlen(line);

		if(len >= 1 && line[len - 1] == '\n')	//Remove the newline character, if it exists.
		{
			line[len - 1] = '\0';
			len--;
		}

		if(LinesLoaded < ExpectedNumLines)
		{
			if((array[LinesLoaded] = malloc(len + 1)) != NULL)
			{
				strcpy(array[LinesLoaded], line);
			} else {
				result = -ENOMEM;
				break;
			}
		}
	}

	if(result == 0)
	{
		if(LinesLoaded!=ExpectedNumLines)
		{
			DEBUG_PRINTF("ParseFontListFile: Mismatched number of lines (%u/%d)\n", LinesLoaded, ExpectedNumLines);
			result = -1;
		}
	}

	return result;
}

static const char DefaultFreeTypeFontFilename[]="NotoSans-Bold.ttf";

static char *GetDefaultFontFilePath(void)
{
	char *result;

	if((result=malloc(sizeof(DefaultFreeTypeFontFilename)+6+2))!=NULL)
		sprintf(result, "lang/%s", DefaultFreeTypeFontFilename);

	return result;
}

static char *GetFontFilePath(unsigned int language)
{
	char *FontFileArray[NUM_SUPPORTED_LANGUAGES], *result, *pFreeTypeFontFilename;
	FILE *file;
	int i;

	result=NULL;
	memset(FontFileArray, 0, sizeof(FontFileArray));

	if((file = fopen("lang/fonts.txt", "r")) != NULL)
	{
		if(ParseFontListFile(FontFileArray, file, NUM_SUPPORTED_LANGUAGES)==0)
		{
			pFreeTypeFontFilename=FontFileArray[language];

			if((result = malloc(strlen(pFreeTypeFontFilename)+6)) != NULL)
				sprintf(result, "lang/%s", pFreeTypeFontFilename);
		} else
			result=GetDefaultFontFilePath();

		for(i=0; i<NUM_SUPPORTED_LANGUAGES; i++)
			if(FontFileArray[i]!=NULL) free(FontFileArray[i]);

		fclose(file);
	} else
		result=GetDefaultFontFilePath();

	return result;
}

static int LoadLanguageStrings(unsigned int language)
{
	int result;
	FILE *file;
	char path[20];
	static const char *LanguageShortForms[NUM_SUPPORTED_LANGUAGES]={
		"JA",
		"EN",
		"FR",
		"SP",
		"GE",
		"IT",
		"DU",
		"PO"
	};

	memset(ExtLangStringTable, 0, sizeof(ExtLangStringTable));
	memset(ExtLangLblStringTable, 0, sizeof(ExtLangLblStringTable));

	sprintf(path, "lang/strings_%s.txt", LanguageShortForms[language]);
	if((file = fopen(path, "r")) != NULL)
	{
		result=ParseLanguageFile(ExtLangStringTable, file, SYS_UI_MSG_COUNT);

		fclose(file);
		if(result==0)
		{
			sprintf(path, "lang/labels_%s.txt", LanguageShortForms[language]);
			if((file = fopen(path, "r")) != NULL)
			{
				result=ParseLanguageFile(ExtLangLblStringTable, file, SYS_UI_LBL_COUNT);
				fclose(file);
			}
			else result = -errno;
		}
	}
	else result = -errno;

	if(result!=0)
		UnloadLanguage();

	return result;
}

static void UnloadLanguage(void)
{
	unsigned int i;

	for(i=0; i<SYS_UI_MSG_COUNT; i++)
	{
		if(ExtLangStringTable[i]!=NULL)
		{
			free(ExtLangStringTable[i]);
			ExtLangStringTable[i]=NULL;
		}
	}

	for(i=0; i<SYS_UI_LBL_COUNT; i++)
	{
		if(ExtLangLblStringTable[i]!=NULL)
		{
			free(ExtLangLblStringTable[i]);
			ExtLangLblStringTable[i]=NULL;
		}
	}
}

const char *GetUIString(unsigned int StringID)
{
	return(ExtLangStringTable[StringID]!=NULL?ExtLangStringTable[StringID]:DefaultLanguageStringTable[StringID]);
}

const char *GetUILabel(unsigned int LabelID)
{
	return(ExtLangLblStringTable[LabelID]!=NULL?ExtLangLblStringTable[LabelID]:DefaultLanguageLabelStringTable[LabelID]);
}
static void WaitForDevice(void)
{
	nopdelay();
	nopdelay();
	nopdelay();
	nopdelay();
	nopdelay();
	nopdelay();
	nopdelay();
	nopdelay();
}

static void InitGraphics(void)
{
	unsigned int FrameBufferVRAMAddress;
	short int dx, dy;

	memset(&UIDrawGlobal, 0, sizeof(UIDrawGlobal));

	UIDrawGlobal.interlaced	= GS_INTERLACED;
	UIDrawGlobal.ffmd	= GS_FFMD_FIELD;

	if(GetConsoleVMode() == 0)
	{	//NTSC
		UIDrawGlobal.vmode	= GS_MODE_NTSC;
		UIDrawGlobal.width	= 640;
		UIDrawGlobal.height	= 448;
		dx = 0;
		dy = 0;
	} else {
		//PAL
		UIDrawGlobal.vmode	= GS_MODE_PAL;
		UIDrawGlobal.width	= 640;
		UIDrawGlobal.height	= 512;
		dx = 4;
		dy = 0;
	}

	GsResetGraph(GS_INIT_RESET, UIDrawGlobal.interlaced, UIDrawGlobal.vmode, UIDrawGlobal.ffmd);

	if(UIDrawGlobal.ffmd == GS_FFMD_FRAME)
		UIDrawGlobal.height /= 2;
	UIDrawGlobal.psm	= GS_PIXMODE_32;

	UIDrawGlobal.giftable.packet_count	= GIF_PACKET_MAX;
	UIDrawGlobal.giftable.packets		= UIDrawGlobal.packets;

	FrameBufferVRAMAddress=GsVramAllocFrameBuffer(UIDrawGlobal.width, UIDrawGlobal.height, UIDrawGlobal.psm);
	GsSetDefaultDrawEnv(&UIDrawGlobal.draw_env, UIDrawGlobal.psm, UIDrawGlobal.width, UIDrawGlobal.height);
	GsSetDefaultDrawEnvAddress(&UIDrawGlobal.draw_env, FrameBufferVRAMAddress);

	GsSetDefaultDisplayEnv(&UIDrawGlobal.disp_env, UIDrawGlobal.psm, UIDrawGlobal.width, UIDrawGlobal.height, dx, dy);
	GsSetDefaultDisplayEnvAddress(&UIDrawGlobal.disp_env, FrameBufferVRAMAddress);

	//execute draw/display environment(s)  (context 1)
	GsPutDrawEnv1(&UIDrawGlobal.draw_env);
	GsPutDisplayEnv1(&UIDrawGlobal.disp_env);

	//set common primitive-drawing settings (Refer to documentation on PRMODE and PRMODECONT registers).
	GsOverridePrimAttributes(GS_DISABLE, 0, 0, 0, 0, 0, 0, 0, 0);

	//Set transparency settings for context 1 (Refer to documentation on TEST and TEXA registers).
	//Alpha test = disabled, always pass, alpha reference = 128, fail method = no update
	GsEnableAlphaTransparency1(GS_DISABLE, GS_ALPHA_ALWAYS, 0x80, GS_ALPHA_NO_UPDATE);
	//Global alpha blending is enabled
	GsEnableAlphaBlending1(GS_ENABLE);

	//Set transparency settings for context 2 (Refer to documentation on TEST and TEXA registers).
	//Alpha test = disabled, always pass, alpha reference = 128, fail method = no update
	GsEnableAlphaTransparency2(GS_DISABLE, GS_ALPHA_ALWAYS, 0x80, GS_ALPHA_NO_UPDATE);
	//Global alpha blending is enabled
	GsEnableAlphaBlending2(GS_ENABLE);
}

#ifdef UI_FONT_MEM
#define UI_LOAD_FONT_FUNC(DrawGlobal, path) LoadFontIntoBuffer(DrawGlobal, path)

static int LoadFontIntoBuffer(struct UIDrawGlobal *gsGlobal, const char *path)
{
	FILE *file;
	int result, size;
	void *buffer;

	if((file = fopen(path, "rb")) != NULL)
	{
		fseek(file, 0, SEEK_END);
		size = ftell(file);
		rewind(file);

		if((buffer = malloc(size)) != NULL)
		{
			if(fread(buffer, 1, size, file) == size)
			{
				if((result = InitFreeTypeWithBuffer(gsGlobal, buffer, size)) != 0)
					free(buffer);
				else {
					gFontBuffer = buffer;
					gFontBufferSize = size;
				}
			} else {
				result = -EIO;
				free(buffer);
			}
		} else
			result = -ENOMEM;
		fclose(file);
	} else
		result = -errno;

	return result;
}
#else
#define UI_LOAD_FONT_FUNC(DrawGlobal, path) InitFreeType(DrawGlobal, path)
#endif

int InitializeUI(void)
{
	char *pFreeTypeFontFilePath;
	int result;

	result=0;
	if((language=configGetLanguage())>=NUM_SUPPORTED_LANGUAGES) language=LANGUAGE_ENGLISH;

	DEBUG_PRINTF("InitializeUI: language is: %u\n", language);

	if(GetConsoleRegion() == CONSOLE_REGION_JAPAN)
	{
		SelectButton = PAD_CIRCLE;
		CancelButton = PAD_CROSS;
	} else {
		SelectButton = PAD_CROSS;
		CancelButton = PAD_CIRCLE;
	}

	InitGraphics();

	DEBUG_PRINTF("InitGraphics()\n");

	while((result=LoadLanguageStrings(language))==-ENODEV)
	{
		DEBUG_PRINTF("LoadLanguageStrings(%u): %d\n", language, result);
		WaitForDevice();
	}
	DEBUG_PRINTF("LoadLanguageStrings(%u) result: %d\n", language, result);

	if((pFreeTypeFontFilePath=GetFontFilePath(language))!=NULL)
	{
		DEBUG_PRINTF("GetFontFilePath(%d): %s\n", language, pFreeTypeFontFilePath);
	}
	else{
		DEBUG_PRINTF("Can't get font file path from GetFontFilePath(%d).\n", language);
		return -1;
	}

	if((result=UI_LOAD_FONT_FUNC(&UIDrawGlobal, pFreeTypeFontFilePath))!=0)
	{
		DEBUG_PRINTF("InitFreeType(%s) result: %d. Using default font.\n", pFreeTypeFontFilePath, result);
		free(pFreeTypeFontFilePath);
		pFreeTypeFontFilePath=GetDefaultFontFilePath();

		if((result=UI_LOAD_FONT_FUNC(&UIDrawGlobal, pFreeTypeFontFilePath))!=0)
		{
			DEBUG_PRINTF("InitFreeType(%s) result: %d\n", pFreeTypeFontFilePath, result);
		}
	}
	DEBUG_PRINTF("InitFreeType(%s) result: %d\n", pFreeTypeFontFilePath, result);
	free(pFreeTypeFontFilePath);

	if(result==0)
	{
		LoadBackground(&UIDrawGlobal, &BackgroundTexture);
		LoadPadGraphics(&UIDrawGlobal, &PadLayoutTexture);
	}

	GsClearDrawEnv1(&UIDrawGlobal.draw_env);

	return result;
}

void DeinitializeUI(void)
{
	UnloadLanguage();
	DeinitFreeType();

#ifdef UI_FONT_MEM
	if(gFontBuffer != NULL)
	{
		free(gFontBuffer);
		gFontBuffer = NULL;
		gFontBufferSize = 0;
	}
#endif
}

enum MBOX_SCREEN_ID{
	MBOX_SCREEN_ID_TITLE	= 1,
	MBOX_SCREEN_ID_MESSAGE,
	MBOX_SCREEN_ID_BTN1,
	MBOX_SCREEN_ID_BTN2,
	MBOX_SCREEN_ID_BTN3,
	MBOX_SCREEN_ID_BTN4,
};

static struct UIMenuItem MessageBoxItems[]={
	{MITEM_LABEL, MBOX_SCREEN_ID_TITLE},
	{MITEM_SEPERATOR},
	{MITEM_BREAK},

	{MITEM_STRING, MBOX_SCREEN_ID_MESSAGE, MITEM_FLAG_READONLY}, {MITEM_BREAK}, {MITEM_BREAK},

	{MITEM_BREAK},
	{MITEM_BREAK},
	{MITEM_BREAK},
	{MITEM_BREAK},
	{MITEM_BREAK},
	{MITEM_BREAK},
	{MITEM_BREAK},

	{MITEM_BUTTON, MBOX_SCREEN_ID_BTN1, MITEM_FLAG_POS_MID, 0, 16}, {MITEM_BREAK}, {MITEM_BREAK},
	{MITEM_BUTTON, MBOX_SCREEN_ID_BTN3, MITEM_FLAG_POS_MID, 0, 16}, {MITEM_BREAK}, {MITEM_BREAK},
	{MITEM_BUTTON, MBOX_SCREEN_ID_BTN2, MITEM_FLAG_POS_MID, 0, 16}, {MITEM_BREAK}, {MITEM_BREAK},
	{MITEM_BUTTON, MBOX_SCREEN_ID_BTN4, MITEM_FLAG_POS_MID, 0, 16},

	{MITEM_TERMINATOR}
};

static struct UIMenu MessageBoxMenu = {NULL, NULL, MessageBoxItems};

int ShowMessageBox(int Option1Label, int Option2Label, int Option3Label, int Option4Label, const char *message, int title)
{
	UISetLabel(&MessageBoxMenu, MBOX_SCREEN_ID_TITLE, title);
	UISetString(&MessageBoxMenu, MBOX_SCREEN_ID_MESSAGE, message);

	UISetLabel(&MessageBoxMenu, MBOX_SCREEN_ID_BTN1, Option1Label);
	UISetVisible(&MessageBoxMenu, MBOX_SCREEN_ID_BTN1, Option1Label != -1);
	UISetLabel(&MessageBoxMenu, MBOX_SCREEN_ID_BTN2, Option2Label);
	UISetVisible(&MessageBoxMenu, MBOX_SCREEN_ID_BTN2, Option2Label != -1);
	UISetLabel(&MessageBoxMenu, MBOX_SCREEN_ID_BTN3, Option3Label);
	UISetVisible(&MessageBoxMenu, MBOX_SCREEN_ID_BTN3, Option3Label != -1);
	UISetLabel(&MessageBoxMenu, MBOX_SCREEN_ID_BTN4, Option4Label);
	UISetVisible(&MessageBoxMenu, MBOX_SCREEN_ID_BTN4, Option4Label != -1);

	if(Option1Label != -1 || Option2Label != -1 || Option3Label != -1 || Option4Label != -1)
	{
		switch(UIExecMenu(&MessageBoxMenu, NULL))
		{
			case MBOX_SCREEN_ID_BTN1:
				return 1;
			case MBOX_SCREEN_ID_BTN2:
				return 2;
			case MBOX_SCREEN_ID_BTN3:
				return 3;
			case MBOX_SCREEN_ID_BTN4:
				return 4;
			default:
				return 0;
		}
	} else {
		UIDrawMenu(&MessageBoxMenu, UI_OFFSET_X, UI_OFFSET_Y, -1);
		SyncFlipFB(&UIDrawGlobal);
	}
}

void DisplayWarningMessage(unsigned int message)
{
	ShowMessageBox(SYS_UI_LBL_OK, -1, -1, -1, GetUIString(message), SYS_UI_LBL_WARNING);
}

void DisplayErrorMessage(unsigned int message)
{
	ShowMessageBox(SYS_UI_LBL_OK, -1, -1, -1, GetUIString(message), SYS_UI_LBL_ERROR);
}

void DisplayInfoMessage(unsigned int message)
{
	ShowMessageBox(SYS_UI_LBL_OK, -1, -1, -1, GetUIString(message), SYS_UI_LBL_INFO);
}

int DisplayPromptMessage(unsigned int message, unsigned char Button1Label, unsigned char Button2Label)
{
	return ShowMessageBox(Button1Label, Button2Label, -1, -1, GetUIString(message), SYS_UI_LBL_CONFIRM);
}

void DisplayFlashStatusUpdate(unsigned int message)
{
	ShowMessageBox(-1, -1, -1, -1, GetUIString(message), SYS_UI_LBL_WAIT);
}

struct UIMenuItem *UIGetItem(struct UIMenu *menu, unsigned char id)
{
	struct UIMenuItem *result;
	unsigned int i;

	result = NULL;
	for(i=0; menu->items[i].type != MITEM_TERMINATOR; i++)
	{
		if(menu->items[i].id == id)
		{
			result = &menu->items[i];
			break;
		}
	}

	return result;
}

void UISetVisible(struct UIMenu *menu, unsigned char id, int visible)
{
	struct UIMenuItem *item;

	if((item = UIGetItem(menu, id)) != NULL)
	{
		if(visible)
			item->flags &= ~MITEM_FLAG_HIDDEN;
		else
			item->flags |= MITEM_FLAG_HIDDEN;
	}
}

void UISetReadonly(struct UIMenu *menu, unsigned char id, int readonly)
{
	struct UIMenuItem *item;

	if((item = UIGetItem(menu, id)) != NULL)
	{
		if(!readonly)
			item->flags &= ~MITEM_FLAG_READONLY;
		else
			item->flags |= MITEM_FLAG_READONLY;
	}
}

void UISetEnabled(struct UIMenu *menu, unsigned char id, int enabled)
{
	struct UIMenuItem *item;

	if((item = UIGetItem(menu, id)) != NULL)
	{
		if(enabled)
			item->flags &= ~MITEM_FLAG_DISABLED;
		else
			item->flags |= MITEM_FLAG_DISABLED;
	}
}

void UISetValue(struct UIMenu *menu, unsigned char id, int value)
{
	struct UIMenuItem *item;

	if((item = UIGetItem(menu, id)) != NULL)
	{
		item->value.value = value;
	}
}

void UISetLabel(struct UIMenu *menu, unsigned char id, int label)
{
	struct UIMenuItem *item;

	if((item = UIGetItem(menu, id)) != NULL)
	{
		item->value.value = label;
	}
}

void UISetString(struct UIMenu *menu, unsigned char id, const char *string)
{
	struct UIMenuItem *item;

	if((item = UIGetItem(menu, id)) != NULL)
	{
		item->string.buffer = string;
	}
}

void UISetType(struct UIMenu *menu, unsigned char id, unsigned char type)
{
	struct UIMenuItem *item;

	if((item = UIGetItem(menu, id)) != NULL)
	{
		item->type = type;
	}
}

void UISetFormat(struct UIMenu *menu, unsigned char id, unsigned char format, unsigned char width)
{
	struct UIMenuItem *item;

	if((item = UIGetItem(menu, id)) != NULL)
	{
		item->format = format;
		item->width = width;
	}
}

void UIDrawMenu(struct UIMenu *menu, short int StartX, short int StartY, short int selection)
{
	const char *pLabel;
	char FormatString[8], *pFormatString, ValueString[32];
	struct UIMenuItem *item, *SelectedItem;
	short int x, y, width, height;
	float xRel, yRel;
	GS_RGBAQ colour;
	int LabelLen;

	DrawBackground(&UIDrawGlobal, &BackgroundTexture);

	x = StartX;
	y = StartY;
	SelectedItem = selection >= 0 ? &menu->items[selection] : NULL;
	for(item = menu->items; item->type != MITEM_TERMINATOR; item++)
	{
		if(item->flags&MITEM_FLAG_HIDDEN) continue;

		if(item->flags&MITEM_FLAG_POS_ABS)
		{
			x = item->x;
			y = item->y;
		} else {
			x += item->x;
			y += item->y;
		}

		switch(item->type)
		{
			case MITEM_SEPERATOR:
				x = StartX;
				y += FONT_HEIGHT;
				DrawLine(&UIDrawGlobal, x, y+FONT_HEIGHT/2, UIDrawGlobal.width - UI_OFFSET_X, y+FONT_HEIGHT/2, 1, GS_WHITE);
				//Fall through.
			case MITEM_BREAK:
				x = StartX;
				y += FONT_HEIGHT;
				break;
			case MITEM_TAB:
				x += (TAB_STOPS*FONT_WIDTH) - (unsigned int)x % (unsigned int)(TAB_STOPS*FONT_WIDTH);
				break;
			case MITEM_SPACE:
				x += FONT_WIDTH;
				break;
			case MITEM_STRING:
				if(item->string.buffer != NULL)
				{
					colour = (item->flags & MITEM_FLAG_DISABLED) ? GS_GREY_FONT : ((item->flags & MITEM_FLAG_READONLY) ? GS_WHITE_FONT : (item == SelectedItem ? GS_YELLOW_FONT : GS_BLUE_FONT));
					FreeTypePrintfWithFeedback(&UIDrawGlobal, x, y, 1, 1.0f, colour, item->string.buffer, &xRel, &yRel);
					x += xRel;
					y += yRel;
				}
				break;
			case MITEM_BUTTON:
				if((pLabel = GetUILabel(item->value.value)) != NULL)
				{
					LabelLen = mbslen(pLabel);

					width = item->width * FONT_WIDTH;
					height = FONT_HEIGHT + FONT_HEIGHT / 2;
					if(item == SelectedItem)
					{
						width *= 1.1f;
						height *= 1.1f;
					}

					if(item->flags & MITEM_FLAG_POS_MID)
						x = StartX + (UIDrawGlobal.width - width) / 2;

					DrawSprite(&UIDrawGlobal, x, y - FONT_HEIGHT / 2, x + width, y + height, 2, item == SelectedItem ? GS_LGREY : GS_GREY);

					colour = (item->flags & MITEM_FLAG_DISABLED) ? GS_GREY_FONT : (item == SelectedItem ? GS_YELLOW_FONT : GS_WHITE_FONT);

					FreeTypePrintfWithFeedback(&UIDrawGlobal, x + (width - LabelLen * FONT_WIDTH) / 2, y, 1, 1.0f, colour, pLabel, &xRel, &yRel);
					x += xRel;
					y += yRel + FONT_HEIGHT;
				}
				break;
			case MITEM_LABEL:
				if((pLabel = GetUILabel(item->value.value)) != NULL)
				{
					if(item->flags & MITEM_FLAG_POS_MID)
						x = (UIDrawGlobal.width - mbslen(pLabel) * FONT_WIDTH) / 2;
					FreeTypePrintfWithFeedback(&UIDrawGlobal, x, y, 1, 1.0f, GS_WHITE_FONT, pLabel, &xRel, &yRel);
					x += xRel;
					y += yRel;
				}
				break;
			case MITEM_COLON:
				FreeTypePrintf(&UIDrawGlobal, x, y, 1, 1.0f, GS_WHITE_FONT, ":");
				x += FONT_WIDTH;
				break;
			case MITEM_DASH:
				FreeTypePrintf(&UIDrawGlobal, x, y, 1, 1.0f, GS_WHITE_FONT, "-");
				x += FONT_WIDTH;
				break;
			case MITEM_DOT:
				FreeTypePrintf(&UIDrawGlobal, x, y, 1, 1.0f, GS_WHITE_FONT, ".");
				x += FONT_WIDTH;
				break;
			case MITEM_SLASH:
				FreeTypePrintf(&UIDrawGlobal, x, y, 1, 1.0f, GS_WHITE_FONT, "/");
				x += FONT_WIDTH;
				break;
			case MITEM_VALUE:
				pFormatString = FormatString;

				if(item->flags&MITEM_FLAG_UNIT_PREFIX)
				{
					switch(item->format)
					{
						case MITEM_FORMAT_HEX:
							pFormatString[0] = '0';
							pFormatString[1] = 'x';
							pFormatString+=2;
							break;
					}
				}

				*pFormatString = '%';
				pFormatString++;

				if(item->width > 0)
				{
					pFormatString += sprintf(pFormatString, "0%u", item->width);
				}

				switch(item->format)
				{
					case MITEM_FORMAT_DEC:
						*pFormatString = 'd';
						pFormatString++;
						break;
					case MITEM_FORMAT_UDEC:
						*pFormatString = 'u';
						pFormatString++;
						break;
					case MITEM_FORMAT_HEX:
						*pFormatString = 'x';
						pFormatString++;
						break;
					case MITEM_FORMAT_POINTER:
						*pFormatString = 'p';
						pFormatString++;
						break;
					case MITEM_FORMAT_FLOAT:
						*pFormatString = 'f';
						pFormatString++;
						break;
				}

				*pFormatString = '\0';
				sprintf(ValueString, FormatString, item->value.value);
				colour = (item->flags & MITEM_FLAG_DISABLED) ? GS_GREY_FONT : ((item->flags & MITEM_FLAG_READONLY) ? GS_WHITE_FONT : (item == SelectedItem ? GS_YELLOW_FONT : GS_BLUE_FONT));
				FreeTypePrintfWithFeedback(&UIDrawGlobal, x, y, 1, 1.0f, colour, ValueString, &xRel, NULL);
				x += xRel;
				break;
			case MITEM_PROGRESS:
				DrawProgressBar(&UIDrawGlobal, item->value.value / 100.0f, x + 20, y, 4, UIDrawGlobal.width - (x + 20) - 20, GS_BLUE);
				y += FONT_HEIGHT;
				break;
#ifdef UI_EN_MITEM_TOGGLE
			case MITEM_TOGGLE:
				colour = (item->flags & MITEM_FLAG_DISABLED) ? GS_GREY_FONT : ((item->flags & MITEM_FLAG_READONLY) ? GS_WHITE_FONT : (item == SelectedItem ? GS_YELLOW_FONT : GS_BLUE_FONT));
				FreeTypePrintfWithFeedback(&UIDrawGlobal, x, y, 1, 1.0f, colour, UIGetLabel(item->value.value == 0 ? : SYS_UI_LBL_DISABLED : SYS_UI_LBL_ENABLED), &xRel, &yRel);
				x += xRel;
				y += yRel;
#endif
		}
	}
}

static void UITransitionSlideRightOut(struct UIMenu *menu, int SelectedOption)
{
	int i;

	for(i=0; i<=15; i++)
	{
		UIDrawMenu(menu, UI_OFFSET_X + i * 48, UI_OFFSET_Y, SelectedOption);
		SyncFlipFB(&UIDrawGlobal);
	}
}

static void UITransitionSlideLeftOut(struct UIMenu *menu, int SelectedOption)
{
	int i;

	for(i=0; i<=15; i++)
	{
		UIDrawMenu(menu, UI_OFFSET_X + -i * 48, UI_OFFSET_Y, SelectedOption);
		SyncFlipFB(&UIDrawGlobal);
	}
}

static void UITransitionSlideRightIn(struct UIMenu *menu, int SelectedOption)
{
	int i;

	for(i=15; i>0; i--)
	{
		UIDrawMenu(menu, UI_OFFSET_X + i * 48, UI_OFFSET_Y, SelectedOption);
		SyncFlipFB(&UIDrawGlobal);
	}
}

static void UITransitionSlideLeftIn(struct UIMenu *menu, int SelectedOption)
{
	int i;

	for(i=15; i>0; i--)
	{
		UIDrawMenu(menu, UI_OFFSET_X + -i * 48, UI_OFFSET_Y, SelectedOption);
		SyncFlipFB(&UIDrawGlobal);
	}
}

static void UITransitionFadeIn(struct UIMenu *menu, int SelectedOption)
{
	int i;
	GS_RGBAQ rgbaq;

	rgbaq.r = 0;
	rgbaq.g = 0;
	rgbaq.b = 0;
	rgbaq.q = 0;

	for(i=0; i<=15; i++)
	{
		rgbaq.a = 0x80-(i*8);
		UIDrawMenu(menu, UI_OFFSET_X, UI_OFFSET_Y, SelectedOption);
		SyncFlipFB(&UIDrawGlobal);
	}
}

static void UITransitionFadeOut(struct UIMenu *menu, int SelectedOption)
{
	int i;
	GS_RGBAQ rgbaq;

	rgbaq.r = 0;
	rgbaq.g = 0;
	rgbaq.b = 0;
	rgbaq.q = 0;

	for(i=15; i>0; i--)
	{
		rgbaq.a = 0x80-(i*8);
		UIDrawMenu(menu, UI_OFFSET_X, UI_OFFSET_Y, SelectedOption);
		SyncFlipFB(&UIDrawGlobal);
	}
}

void UITransition(struct UIMenu *menu, int type, int SelectedOption)
{
	switch(type)
	{
		case UIMT_LEFT_OUT:
			UITransitionSlideLeftOut(menu, SelectedOption);
			break;
		case UIMT_RIGHT_OUT:
			UITransitionSlideRightOut(menu, SelectedOption);
			break;
		case UIMT_LEFT_IN:
			UITransitionSlideLeftIn(menu, SelectedOption);
			break;
		case UIMT_RIGHT_IN:
			UITransitionSlideRightIn(menu, SelectedOption);
			break;
		case UIMT_FADE_IN:
			UITransitionFadeIn(menu, SelectedOption);
			break;
		case UIMT_FADE_OUT:
			UITransitionFadeOut(menu, SelectedOption);
			break;
	}
}

static short int UIGetNextSelectableItem(struct UIMenu *menu, short int index)
{
	short int result;
	int i;

	index = (index < 0) ? 0 : index + 1;
	result = -1;
	for(i = index; menu->items[i].type != MITEM_TERMINATOR; i++)
	{
		if(menu->items[i].type > MITEM_LABEL
			&& !(menu->items[i].flags & MITEM_FLAG_DISABLED)
			&& !(menu->items[i].flags & MITEM_FLAG_HIDDEN)
			&& !(menu->items[i].flags & MITEM_FLAG_READONLY))
		{
			result = i;
			break;
		}
	}

	return result;
}

static short int UIGetPrevSelectableItem(struct UIMenu *menu, short int index)
{
	short int result;
	int i;

	index = (index < 0) ? 0 : index - 1;
	result = -1;
	for(i = index; i >= 0; i--)
	{
		if(menu->items[i].type > MITEM_LABEL
			&& !(menu->items[i].flags & MITEM_FLAG_DISABLED)
			&& !(menu->items[i].flags & MITEM_FLAG_HIDDEN)
			&& !(menu->items[i].flags & MITEM_FLAG_READONLY))
		{
			result = i;
			break;
		}
	}

	return result;
}

int UIExecMenu(struct UIMenu *menu, int (*callback)(struct UIMenu *menu, unsigned short int frame, int selection, int padstatus))
{
	int result;
	u32 PadStatus;
	struct UIMenuItem *item;
	short int selection, NextSel;
	unsigned short int frame;

	PadStatus = 0;
	frame = 0;

	//Find first selectable option.
	item = ((selection = UIGetNextSelectableItem(menu, -1)) >= 0) ? &menu->items[selection] : NULL;

	if(callback != NULL)
	{
		if((result = callback(menu, frame, selection, -1)) != 0)
			return result;
	}

	while(1)
	{
		PadStatus=ReadCombinedPadStatus();

		if(PadStatus&PAD_UP)
		{
			//Try to find the previous selectable option.
			if((NextSel = UIGetPrevSelectableItem(menu, selection)) >= 0)
			{
				selection = NextSel;
				item = &menu->items[selection];
			}
		} else if(PadStatus&PAD_DOWN) {
			//Try to find the next selectable option.
			if((NextSel = UIGetNextSelectableItem(menu, selection)) >= 0)
			{
				selection = NextSel;
				item = &menu->items[selection];
			}
		} else if(PadStatus&PAD_LEFT) {
			if(item != NULL && !(item->flags & MITEM_FLAG_READONLY))
			{
				switch(item->type)
				{
					case MITEM_VALUE:
						if(item->value.value - 1 > item->value.min)
							item->value.value--;
				}
			}
		} else if(PadStatus&PAD_RIGHT) {
			if(item != NULL && !(item->flags & MITEM_FLAG_READONLY))
			{
				switch(item->type)
				{
					case MITEM_VALUE:
						if(item->value.value + 1 < item->value.max)
							item->value.value++;
				}
			}
		}

		if(PadStatus&SelectButton)
		{
			if(item != NULL && !(item->flags & MITEM_FLAG_DISABLED))
			{
				switch(item->type)
				{
					case MITEM_BUTTON:
						return item->id;
				}
			}
		} else if(PadStatus&CancelButton) {
			//User aborted.
			return 1;
		}

		if(PadStatus&PAD_R1) {
			if(menu->next != NULL)
			{
				UITransition(menu, UIMT_RIGHT_OUT, selection);

				menu = menu->next;
				//Find first selectable option.
				item = ((selection = UIGetNextSelectableItem(menu, -1)) >= 0) ? &menu->items[selection] : NULL;

				UITransition(menu, UIMT_LEFT_IN, selection);

				if(callback != NULL)
				{
					if((result = callback(menu, frame, selection, -1)) != 0)
						return result;
				}
			}
		} else if(PadStatus&PAD_L1) {
			if(menu->prev != NULL)
			{
				UITransition(menu, UIMT_LEFT_OUT, selection);

				menu = menu->prev;
				//Find first selectable option.
				item = ((selection = UIGetNextSelectableItem(menu, -1)) >= 0) ? &menu->items[selection] : NULL;

				UITransition(menu, UIMT_RIGHT_IN, selection);

				if(callback != NULL)
				{
					if((result = callback(menu, frame, selection, -1)) != 0)
						return result;
				}
			}
		}

		UIDrawMenu(menu, UI_OFFSET_X, UI_OFFSET_Y, selection);

		//Draw legends
		if(menu->next != NULL)
		{
			if(frame % 180 < 30)
				DrawButtonLegend(&UIDrawGlobal, &PadLayoutTexture, BUTTON_TYPE_R1, UIDrawGlobal.width - 40 - frame % 30 / 3, 400, 4);
			else if(frame % 180 < 60)
				DrawButtonLegend(&UIDrawGlobal, &PadLayoutTexture, BUTTON_TYPE_R1, UIDrawGlobal.width - 40 - 10 + (frame % 30 / 3), 400, 4);
			else
				DrawButtonLegend(&UIDrawGlobal, &PadLayoutTexture, BUTTON_TYPE_R1, UIDrawGlobal.width - 40, 400, 4);
		}
		if(menu->prev != NULL)
		{
			if(frame % 180 < 30)
				DrawButtonLegend(&UIDrawGlobal, &PadLayoutTexture, BUTTON_TYPE_L1, 20 + frame % 30 / 3, 400, 4);
			else if(frame % 180 < 60)
				DrawButtonLegend(&UIDrawGlobal, &PadLayoutTexture, BUTTON_TYPE_L1, 20 + 10 - frame % 30 / 3, 400, 4);
			else
				DrawButtonLegend(&UIDrawGlobal, &PadLayoutTexture, BUTTON_TYPE_L1, 20, 400, 4);
		}

		if(callback != NULL)
		{
			if((result = callback(menu, frame, selection, PadStatus)) != 0)
				return result;
		}

		SyncFlipFB(&UIDrawGlobal);
		frame++;
	}
}
