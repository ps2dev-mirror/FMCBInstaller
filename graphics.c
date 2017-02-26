#include <kernel.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libgs.h>

#include <png.h>

#include "main.h"
#include "graphics.h"
#include "pad.h"

#include "font.h"

extern int VBlankStartSema;
static GS_GIF_TAG *LastGIFPacket = NULL;

static char twh(short int val)
{
	char res;

	asm volatile("plzcw %0, %1" : "=r" (res) : "r" (val));
	res = 31 - (res + 1);
	if(val > res)
		res++;

	return res;
}

static void PNGReadMem(png_structp pngPtr, png_bytep data, png_size_t length){
        void **PngBufferPtr=png_get_io_ptr(pngPtr);

        memcpy(data, *PngBufferPtr, length);
        (unsigned int)*PngBufferPtr+=length;
}

static int LoadPNGImage(struct UIDrawGlobal *gsGlobal, GS_IMAGE *Texture, const void* buffer, unsigned int size){
	static void **PngFileBufferPtr;
	png_structp png_ptr;
	png_infop info_ptr;
	png_uint_32 width, height;
	png_bytep *row_pointers;
	void *mem;

	unsigned int sig_read = 0;
        int row, i, k=0, j, bit_depth, color_type, interlace_type;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp) NULL, NULL, NULL);

	if(!png_ptr)
	{
		//printf("PNG Read Struct Init Failed\n");
		return -1;
	}

	info_ptr = png_create_info_struct(png_ptr);

	if(!info_ptr)
	{
		//printf("PNG Info Struct Init Failed\n");
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		return -1;
	}

	if(setjmp(png_jmpbuf(png_ptr)))
	{
		//printf("Got PNG Error!\n");
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
		return -1;
	}

	PngFileBufferPtr=(void*)buffer;
	png_set_read_fn(png_ptr, &PngFileBufferPtr, &PNGReadMem);
	png_set_sig_bytes(png_ptr, sig_read);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);

	png_set_strip_16(png_ptr);

	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_expand(png_ptr);

	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand(png_ptr);

	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);

	png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);

	png_read_update_info(png_ptr, info_ptr);

	Texture->x	= 0;
	Texture->y	= 0;
	Texture->width	= width;
	Texture->height	= height;

	if(png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_RGB_ALPHA)
	{
		int row_bytes = png_get_rowbytes(png_ptr, info_ptr);
		Texture->psm = GS_TEX_32;
		mem = memalign(128, Texture->width * Texture->height * 4);

		row_pointers = calloc(height, sizeof(png_bytep));

		for (row = 0; row < height; row++) row_pointers[row] = malloc(row_bytes);

		png_read_image(png_ptr, row_pointers);

		struct pixel { unsigned char r,g,b,a; };
		struct pixel *Pixels = (struct pixel *) UNCACHED_SEG(mem);

		for (i=0;i<height;i++) {
			for (j=0;j<width;j++) {
				Pixels[k].r = row_pointers[i][4*j];
				Pixels[k].g = row_pointers[i][4*j+1];
				Pixels[k].b = row_pointers[i][4*j+2];
				Pixels[k++].a = ((unsigned int) row_pointers[i][4*j+3] * 128 / 255);
			}
		}

		for(row = 0; row < height; row++) free(row_pointers[row]);

		free(row_pointers);
	}
	else if(png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_RGB)
	{
		int row_bytes = png_get_rowbytes(png_ptr, info_ptr);
		Texture->psm = GS_TEX_24;
		mem = memalign(128, Texture->width * Texture->height * 4);

		row_pointers = calloc(height, sizeof(png_bytep));

		for(row = 0; row < height; row++) row_pointers[row] = malloc(row_bytes);

		png_read_image(png_ptr, row_pointers);

		struct pixel3 { unsigned char r,g,b; };
		struct pixel3 *Pixels = (struct pixel3 *) UNCACHED_SEG(mem);

		for (i=0;i<height;i++) {
			for (j=0;j<width;j++) {
				Pixels[k].r = row_pointers[i][4*j];
				Pixels[k].g = row_pointers[i][4*j+1];
				Pixels[k++].b = row_pointers[i][4*j+2];
			}
		}

		for(row = 0; row < height; row++) free(row_pointers[row]);

		free(row_pointers);
	}
	else
	{
		//printf("This texture depth is not supported yet!\n");
		return -1;
	}

	png_read_end(png_ptr, NULL);
	png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);

	Texture->vram_addr = GsVramAllocTextureBuffer(Texture->width, Texture->height + 64, Texture->psm);
	Texture->vram_width = ((Texture->width + 63) & ~63) / 64;

	GsLoadImage(mem, Texture);
	GsTextureFlush();
	free(mem);

	return 0;
}

void DrawBackground(struct UIDrawGlobal *gsGlobal, GS_IMAGE *background){
	short int x, y;

	x=(gsGlobal->width-background->width)/2;
	y=(gsGlobal->height-background->height)/2;

	DrawSprite(gsGlobal, 0, 0, gsGlobal->width, gsGlobal->height, 9,  (GS_RGBAQ){0x00,0x00,0x00,0x80,0x00});
	DrawSpriteTextured(gsGlobal, background,	x, y,
					0, 0,
					x+background->width, y+background->height,
					background->width, background->height,
					8, (GS_RGBAQ){0xFF,0xFF,0xFF,0x80,0x00});
}

extern unsigned char pad_layout_start[];
extern unsigned int pad_layout_size;

extern unsigned char background_start[];
extern unsigned int background_size;

int LoadBackground(struct UIDrawGlobal *gsGlobal, GS_IMAGE* Texture){
	return LoadPNGImage(gsGlobal, Texture, background_start, background_size);
}

int LoadPadGraphics(struct UIDrawGlobal *gsGlobal, GS_IMAGE* Texture){
	return LoadPNGImage(gsGlobal, Texture, pad_layout_start, pad_layout_size);
}

void DrawSetFilterMode(struct UIDrawGlobal *gsGlobal, int mode)
{
	u64 *p;

	//Use the uncached segment, to avoid needing to flush the data cache.
	p = (u64*)UNCACHED_SEG(GsGifPacketsAlloc(&gsGlobal->giftable, 2));
	LastGIFPacket = (GS_GIF_TAG*)p;

	gs_setGIF_TAG(((GS_GIF_TAG	*)&p[0]), 1,0,0,0,GS_GIF_PACKED, 1, gif_rd_ad);
	gs_setR_TEX1_1(((GS_R_TEX1	*)&p[1]), 0, 0, mode, mode, 0, 0, 0);
}

void DrawLine(struct UIDrawGlobal *gsGlobal, short int x1, short int y1, short int x2, short int y2, short int z, GS_RGBAQ rgbaq)
{
	u64 *p;

	//Use the uncached segment, to avoid needing to flush the data cache.
	p = (u64*)UNCACHED_SEG(GsGifPacketsAlloc(&gsGlobal->giftable, 3)); //Allocate 3 qwords for 1 untextured line
	LastGIFPacket = (GS_GIF_TAG*)p;

	gs_setGIF_TAG(((GS_GIF_TAG	*)&p[0]), 1,0,0,0,GS_GIF_REGLIST,4, gs_g_prim | gs_g_rgbaq << 4 | gs_g_xyz2 << 8 | gs_g_xyz2 << 12);
	//prim_type = GS_PRIM_LINE, abe = 1
	gs_setPRIM(((GS_PRIM		*)&p[2]), GS_PRIM_LINE, 0, 0, 0, 1, 0, 0, 0, 0);
	*(GS_RGBAQ*)&p[3] = rgbaq;
	gs_setXYZ2(((GS_XYZ		*)&p[4]), (gsGlobal->draw_env.offset_x+x1)<<4,	(gsGlobal->draw_env.offset_y+y1)<<4, z<<4);
	gs_setXYZ2(((GS_XYZ		*)&p[5]), (gsGlobal->draw_env.offset_x+x2)<<4,	(gsGlobal->draw_env.offset_y+y2)<<4, z<<4);
}

void DrawSprite(struct UIDrawGlobal *gsGlobal, short int x1, short int y1, short int x2, short int y2, short int z, GS_RGBAQ rgbaq)
{
	u64 *p;

	//Use the uncached segment, to avoid needing to flush the data cache.
	p = (u64*)UNCACHED_SEG(GsGifPacketsAlloc(&gsGlobal->giftable, 3)); //Allocate 3 qwords for 1 untextured sprite
	LastGIFPacket = (GS_GIF_TAG*)p;

	gs_setGIF_TAG(((GS_GIF_TAG	*)&p[0]), 1,0,0,0,GS_GIF_REGLIST,4, gs_g_prim | gs_g_rgbaq << 4 | gs_g_xyz2 << 8 | gs_g_xyz2 << 12);
	//prim_type = GS_PRIM_SPRITE, abe = 1
	gs_setPRIM(((GS_PRIM		*)&p[2]), GS_PRIM_SPRITE, 0, 0, 0, 1, 0, 0, 0, 0);
	*(GS_RGBAQ*)&p[3] = rgbaq;
	gs_setXYZ2(((GS_XYZ		*)&p[4]), (gsGlobal->draw_env.offset_x+x1)<<4,	(gsGlobal->draw_env.offset_y+y1)<<4, z<<4);
	gs_setXYZ2(((GS_XYZ		*)&p[5]), (gsGlobal->draw_env.offset_x+x2)<<4,	(gsGlobal->draw_env.offset_y+y2)<<4, z<<4);
}

void DrawSpriteTextured(struct UIDrawGlobal *gsGlobal, GS_IMAGE *texture, short int x1, short int y1, short int u1, short int v1, short int x2, short int y2, short int u2, short int v2, short int z, GS_RGBAQ rgbaq)
{
	u64 *p;

	//Use the uncached segment, to avoid needing to flush the data cache.
	p = (u64*)UNCACHED_SEG(GsGifPacketsAlloc(&gsGlobal->giftable, 5)); //Allocate 5 qwords for 1 textured sprite
	LastGIFPacket = (GS_GIF_TAG*)p;

	gs_setGIF_TAG(((GS_GIF_TAG	*)&p[0]), 1,0,0,0,GS_GIF_REGLIST,8, gs_g_tex0_1 | gs_g_prim << 4 | gs_g_rgbaq << 8 | gs_g_uv << 12 | gs_g_xyz2 << 16 | gs_g_uv << 20 | gs_g_xyz2 << 24 | gif_rd_nop << 28);
	gs_setTEX0_1(((GS_TEX0		*)&p[2]), texture->vram_addr, texture->vram_width, texture->psm, twh(texture->width), twh(texture->height), 1, GS_TEX_MODULATE, 0, 0, 0, 0, 0);
	//prim_type = GS_PRIM_SPRITE, tme = 1, fst = 1, abe = 1
	gs_setPRIM(((GS_PRIM		*)&p[3]), GS_PRIM_SPRITE, 0, 1, 0, 1, 0, 1, 0, 0);
	*(GS_RGBAQ*)&p[4] = rgbaq;
	gs_setUV(((GS_UV		*)&p[5]), u1 << 4 | 8, v1 << 4 | 8);
	gs_setXYZ2(((GS_XYZ		*)&p[6]), (gsGlobal->draw_env.offset_x+x1)<<4,	(gsGlobal->draw_env.offset_y+y1)<<4, z<<4);
	gs_setUV(((GS_UV		*)&p[7]), u2 << 4 | 8, v2 << 4 | 8);
	gs_setXYZ2(((GS_XYZ		*)&p[8]), (gsGlobal->draw_env.offset_x+x2)<<4,	(gsGlobal->draw_env.offset_y+y2)<<4, z<<4);
	gs_setNOP(((GS_NOP		*)&p[9]));
}

void DrawSpriteTexturedClut(struct UIDrawGlobal *gsGlobal, GS_IMAGE *texture, GS_IMAGE *clut, short int x1, short int y1, short int u1, short int v1, short int x2, short int y2, short int u2, short int v2, short int z, GS_RGBAQ rgbaq)
{
	u64 *p;

	//Use the uncached segment, to avoid needing to flush the data cache.
	p = (u64*)UNCACHED_SEG(GsGifPacketsAlloc(&gsGlobal->giftable, 5)); //Allocate 5 qwords for 1 textured sprite with CLUT
	LastGIFPacket = (GS_GIF_TAG*)p;

	gs_setGIF_TAG(((GS_GIF_TAG	*)&p[0]), 1,0,0,0,GS_GIF_REGLIST,8, gs_g_tex0_1 | gs_g_prim << 4 | gs_g_rgbaq << 8 | gs_g_uv << 12 | gs_g_xyz2 << 16 | gs_g_uv << 20 | gs_g_xyz2 << 24 | gif_rd_nop << 28);
	gs_setTEX0_1(((GS_TEX0		*)&p[2]), texture->vram_addr, texture->vram_width, texture->psm, twh(texture->width), twh(texture->height), 1, GS_TEX_MODULATE, clut->vram_addr, clut->psm, 0, 0, 1);
	//prim_type = GS_PRIM_SPRITE, tme = 1, fst = 1, abe = 1
	gs_setPRIM(((GS_PRIM		*)&p[3]), GS_PRIM_SPRITE, 0, 1, 0, 1, 0, 1, 0, 0);
	*(GS_RGBAQ*)&p[4] = rgbaq;
	gs_setUV(((GS_UV		*)&p[5]), u1 << 4 | 8, v1 << 4 | 8);
	gs_setXYZ2(((GS_XYZ		*)&p[6]), (gsGlobal->draw_env.offset_x+x1)<<4,	(gsGlobal->draw_env.offset_y+y1)<<4, z<<4);
	gs_setUV(((GS_UV		*)&p[7]), u2 << 4 | 8, v2 << 4 | 8);
	gs_setXYZ2(((GS_XYZ		*)&p[8]), (gsGlobal->draw_env.offset_x+x2)<<4,	(gsGlobal->draw_env.offset_y+y2)<<4, z<<4);
	gs_setNOP(((GS_NOP		*)&p[9]));
}

void UploadClut(struct UIDrawGlobal *gsGlobal, GS_IMAGE *clut, const void *buffer)
{
	QWORD *p;

	ExecSyncClear(gsGlobal);
	GsLoadImage(buffer, clut);

	p = (QWORD*)UNCACHED_SEG(GsGifPacketsAlloc(&gsGlobal->giftable, 2));
	LastGIFPacket = (GS_GIF_TAG*)p;

	gs_setGIF_TAG(((GS_GIF_TAG	*)&p[0]), 1,0,0,0,GS_GIF_PACKED,1,gif_rd_ad);
	gs_setR_TEXFLUSH((GS_R_TEXFLUSH	*)&p[1]);
}

struct ButtonLayoutParameters{
	unsigned short int u, v;
	unsigned short int length, width;
};

static const struct ButtonLayoutParameters ButtonLayoutParameters[]=
{
	/* Triggers */
	{60, 0, 20, 16},
	{40, 0, 20, 16},
	{20, 0, 20, 16},
	{0, 0, 20, 16},
	/* D-pad buttons */
	{0, 16, 16, 16},
	{48, 16, 16, 16},
	{16, 16, 16, 16},
	{32, 16, 16, 16},
	/* CIRCLE, CROSS, SQUARE and TRIANGLE */
	{0, 32, 18, 17},
	{18, 32, 18, 17},
	{36, 32, 18, 17},
	{54, 32, 18, 17},
	/* START and SELECT */
	{80, 18, 38, 18},
	{80, 0, 38, 18}
};

void DrawButtonLegendWithFeedback(struct UIDrawGlobal *gsGlobal, GS_IMAGE* PadGraphicsTexture, unsigned char ButtonType, short int x, short int y, short int z, short int *xRel)
{
	DrawSpriteTextured(gsGlobal, PadGraphicsTexture,
				x, y,
				ButtonLayoutParameters[ButtonType].u, ButtonLayoutParameters[ButtonType].v,
				x+ButtonLayoutParameters[ButtonType].length, y+ButtonLayoutParameters[ButtonType].width,
				ButtonLayoutParameters[ButtonType].u+ButtonLayoutParameters[ButtonType].length, ButtonLayoutParameters[ButtonType].v+ButtonLayoutParameters[ButtonType].width,
				z, (GS_RGBAQ){0xFF,0xFF,0xFF,0x80,0x00});

	if(xRel != NULL)
		*xRel = ButtonLayoutParameters[ButtonType].length;
}

void DrawButtonLegend(struct UIDrawGlobal *gsGlobal, GS_IMAGE* PadGraphicsTexture, unsigned char ButtonType, short int x, short int y, short int z)
{
	DrawButtonLegendWithFeedback(gsGlobal, PadGraphicsTexture, ButtonType, x, y, z, NULL);
}

void DrawProgressBar(struct UIDrawGlobal *gsGlobal, float percentage, short int x, short int y, short int z, short int len, GS_RGBAQ colour)
{
	char CharBuffer[8];
	float ProgressBarFillEndX;

	/* Draw the progress bar. */
	DrawSprite(gsGlobal, x, y, x+len, y+20, z, GS_LGREY);
	ProgressBarFillEndX=x+ (len - 10) *percentage;
	/* FIXME: For some unknown reason, the progress bar fill is being offset by -10 pixels. */
	DrawSprite(gsGlobal, x+5, y+5+10, ProgressBarFillEndX, y-5+10, z, colour);
	snprintf(CharBuffer, sizeof(CharBuffer)/sizeof(char), "%u%%", (unsigned int)(percentage*100));
	FontPrintf(gsGlobal, x+len/2, y, z - 1, 1.0f, GS_WHITE_FONT, CharBuffer);
}

void SyncFlipFB(struct UIDrawGlobal *gsGlobal)
{
	if(LastGIFPacket != NULL)
	{
		LastGIFPacket->eop = 1;
		LastGIFPacket = NULL;
	}

	PollSema(VBlankStartSema);
	WaitSema(VBlankStartSema);

	GsGifPacketsExecute(&gsGlobal->giftable, 1);
	GsGifPacketsClear(&gsGlobal->giftable);
}

void ExecSyncClear(struct UIDrawGlobal *gsGlobal)
{
	if(LastGIFPacket != NULL)
	{
		LastGIFPacket->eop = 1;
		LastGIFPacket = NULL;
	}

	GsGifPacketsExecute(&gsGlobal->giftable, 1);
	GsGifPacketsClear(&gsGlobal->giftable);
}
