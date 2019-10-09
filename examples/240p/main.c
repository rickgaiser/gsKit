//  ____     ___ |    / _____ _____
// |  __    |    |___/    |     |
// |___| ___|    |    \ __|__   |     gsKit Open Source Project.
// ----------------------------------------------------------------------
// Copyright 2004 - Chris "Neovanglist" Gilbert <Neovanglist@LainOS.org>
// Licenced under Academic Free License version 2.0
// Review gsKit README & LICENSE files for further details.
//
// pixelperfect.c - Example demonstrating gsKit texture operation.
//                  Aligning the texels exactly to pixels
//

#include <stdio.h>
#include <malloc.h>

#include <gsKit.h>
#include <dmaKit.h>
#include <gsToolkit.h>


#define HIRES_MODE

void create_tex_effect(GSTEXTURE *eff, u32 height) {
	u32 x,y;
	u32 *pPixel;

	eff->Width = 64; // Minimum width?
	eff->Height = height;
	eff->PSM = GS_PSM_CT32;
	eff->Mem = malloc(gsKit_texture_size_ee(eff->Width, eff->Height, eff->PSM));
	eff->Vram = 0;
	eff->Filter = GS_FILTER_NEAREST;
	gsKit_setup_tbw(eff);
	pPixel = eff->Mem;

	for(y=0; y<eff->Height; y++) {
		for(x=0; x<eff->Width; x++) {
			*pPixel++ = (y&1) ? GS_SETREG_RGBA(0x00,0x00,0x00,0x00) : GS_SETREG_RGBA(0x00,0x00,0x00,0x80);
		}
	}
}


int main(int argc, char *argv[])
{
	GSGLOBAL *gsGlobal = gsKit_init_global();
	GSFONTM *gsFontM = gsKit_init_fontm();
	GSTEXTURE tex;
	GSTEXTURE tex_effect;
	char tempstr[256];
	int i=0;
	int toggle=0;
	unsigned int iScaleW;
	unsigned int iScaleH;

	u64 Black = GS_SETREG_RGBA(0x00,0x00,0x00,0x80);
	u64 White = GS_SETREG_RGBA(0xFF,0xFF,0xFF,0x80);
	u64 TexCol = GS_SETREG_RGBA(0x80,0x80,0x80,0x80);

	gsGlobal->Mode = GS_MODE_DTV_1080I;
	gsGlobal->Interlace = GS_INTERLACED;
	gsGlobal->Field = GS_FRAME;
	gsGlobal->Width = 1920;
	gsGlobal->Height = 540;
	iScaleW = 4;
	iScaleH = 2;

	gsGlobal->PSM = GS_PSM_CT16;
	gsGlobal->PSMZ = GS_PSMZ_16S;
	gsGlobal->DoubleBuffering = GS_SETTING_ON;
	gsGlobal->ZBuffering = GS_SETTING_OFF;

	dmaKit_init(D_CTRL_RELE_OFF,D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
		    D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);

	// Initialize the DMAC
	dmaKit_chan_init(DMA_CHANNEL_GIF);

#ifdef HIRES_MODE
	gsKit_hires_init_screen(gsGlobal, 3);
#else
	gsKit_init_screen(gsGlobal);
#endif

	gsKit_fontm_upload(gsGlobal, gsFontM);

	// Load textures
	tex.Delayed = 1;
	gsKit_texture_png(gsGlobal, &tex, "host:240p_2.png");
	create_tex_effect(&tex_effect, tex.Height * iScaleH);

	gsKit_set_clamp(gsGlobal, GS_CMODE_CLAMP);
	gsKit_set_test(gsGlobal, GS_ZTEST_OFF);
	gsKit_set_primalpha(gsGlobal, GS_SETREG_ALPHA(0, 1, 0, 1, 0), 0);
	gsGlobal->PrimAlphaEnable = GS_SETTING_ON;

	while(1) {
#ifndef HIRES_MODE
		// HIRES mode automatically clears the screen
		// So only clear the screen in normal mode
		gsKit_clear(gsGlobal, Black);
#endif

		i++;
		if (i >= 60) {
			i=0;
			toggle = toggle ? 0 : 1;
		}

		// Pixel perfect texture mapping:
		// - Move to the upper-left corner of the pixel
		float fOffsetX = -0.5f;
		float fOffsetY = -0.5f;
		float fOffsetU = 0.0f;
		float fOffsetV = 0.0f;
		unsigned int iDispWidth  = tex.Width  * iScaleW;
		unsigned int iDispHeight = tex.Height * iScaleH;

		// Center on screen
		float fPosX = (gsGlobal->Width  - iDispWidth)  / 2;
		float fPosY = (gsGlobal->Height - iDispHeight) / 2;

		//tex.Filter = toggle ? GS_FILTER_LINEAR : GS_FILTER_NEAREST;
		tex.Filter = GS_FILTER_NEAREST;
		gsKit_TexManager_bind(gsGlobal, &tex);
		gsKit_prim_sprite_texture(gsGlobal, &tex,
			fOffsetX+fPosX,             // X1
			fOffsetY+fPosY,             // Y2
			fOffsetU+0.0f,              // U1
			fOffsetV+0.0f,              // V1
			fOffsetX+fPosX+iDispWidth,  // X2
			fOffsetY+fPosY+iDispHeight, // Y2
			fOffsetU+tex.Width,         // U2
			fOffsetV+tex.Height,        // V2
			2,
			TexCol);

		if (toggle) {
			gsKit_TexManager_bind(gsGlobal, &tex_effect);
			gsKit_prim_sprite_texture(gsGlobal, &tex_effect,
				fOffsetX+fPosX,             // X1
				fOffsetY+fPosY,             // Y2
				fOffsetU+0.0f,              // U1
				fOffsetV+0.0f,              // V1
				fOffsetX+fPosX+iDispWidth,  // X2
				fOffsetY+fPosY+iDispHeight, // Y2
				fOffsetU+tex_effect.Width,  // U2
				fOffsetV+tex_effect.Height, // V2
				2,
				TexCol);
		}

		//sprintf(tempstr, "fOffsetXY = %.3f %.3f", fOffsetX, fOffsetY);
		//gsKit_fontm_print_scaled(gsGlobal, gsFontM, 20, 400, 2, 0.5f, Black, tempstr);
		//sprintf(tempstr, "fOffsetUV = %.3f %.3f", fOffsetU, fOffsetV);
		//gsKit_fontm_print_scaled(gsGlobal, gsFontM, 20, 430, 2, 0.5f, Black, tempstr);

#ifdef HIRES_MODE
		gsKit_hires_sync(gsGlobal);
		gsKit_hires_flip(gsGlobal);
#else
		gsKit_queue_exec(gsGlobal);
		gsKit_sync_flip(gsGlobal);
#endif
		gsKit_TexManager_nextFrame(gsGlobal);
	}

	return 0;
}
