//  ____     ___ |    / _____ _____
// |  __    |    |___/    |     |
// |___| ___|    |    \ __|__   |     gsKit Open Source Project.
// ----------------------------------------------------------------------
// Copyright 2004 - Chris "Neovanglist" Gilbert <Neovanglist@LainOS.org>
// Licenced under Academic Free License version 2.0
// Review gsKit README & LICENSE files for further details.
//
// textures.c - Example demonstrating gsKit texture operation.
//

#include <stdio.h>
#include <malloc.h>

#include <gsKit.h>
#include <dmaKit.h>
#include <gsToolkit.h>
#include <gsInline.h>
#include <gsTexture.h>

int main(int argc, char *argv[])
{
	GSGLOBAL *gsGlobal;
	GSTEXTURE atlas;

#ifdef HAVE_LIBJPEG
    GSTEXTURE Tex3;
#endif

	u64 White = GS_SETREG_RGBAQ(0xFF,0xFF,0xFF,0x00,0x00);
	u64 TexCol = GS_SETREG_RGBAQ(0x80,0x80,0x80,0x80,0x00);

	gsGlobal = gsKit_init_global();

	gsGlobal->PSM = GS_PSM_CT24;
	gsGlobal->PSMZ = GS_PSMZ_16S;
	// gsGlobal->DoubleBuffering = GS_SETTING_OFF;
	// gsGlobal->ZBuffering = GS_SETTING_OFF;

	dmaKit_init(D_CTRL_RELE_OFF,D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
		    D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);

	// Initialize the DMAC
	dmaKit_chan_init(DMA_CHANNEL_GIF);

	gsKit_init_screen(gsGlobal);
	gsKit_TexManager_init(gsGlobal);
	gsKit_mode_switch(gsGlobal, GS_ONESHOT);

	gsKit_clear(gsGlobal, White);

	gsKit_texture_png(gsGlobal, &atlas, "runner_atlas.png");
	printf("Atlas Height: %i\n",atlas.Height);
	printf("Atlas Width: %i\n",atlas.Width);

	printf("Atlas VRAM Range = 0x%X - 0x%X\n",atlas.Vram, atlas.Vram +gsKit_texture_size(atlas.Width, atlas.Height, atlas.PSM) - 1);

	gsKit_set_clamp(gsGlobal, GS_CMODE_CLAMP);

	gsKit_clear(gsGlobal, White);

	GSPRIMUVPOINT *verts = (GSPRIMUVPOINT*)malloc(sizeof(GSPRIMUVPOINT) * 3);
	verts[0].xyz2 = vertex_to_XYZ2(gsGlobal, 0, 0, 0);
	verts[0].rgbaq = color_to_RGBAQ(0x80, 0x80, 0x80, 0x80, 0);
	verts[0].uv = vertex_to_UV(&atlas, 0, 0);

	verts[1].xyz2 = vertex_to_XYZ2(gsGlobal, 324, 0, 0);
	verts[1].rgbaq = color_to_RGBAQ(0x80, 0x80, 0x80, 0x80, 0);
	verts[1].uv = vertex_to_UV(&atlas, atlas.Width, 0);

	verts[2].xyz2 = vertex_to_XYZ2(gsGlobal, 0, 324, 0);
	verts[2].rgbaq = color_to_RGBAQ(0x80, 0x80, 0x80, 0x80, 0);
	verts[2].uv = vertex_to_UV(&atlas, 0, atlas.Height);

	while(1)
	{
		gsKit_clear(gsGlobal, White);

		gsKit_TexManager_bind(gsGlobal, &atlas);

		gsKit_prim_list_triangle_goraud_texture_uv_3d(gsGlobal, &atlas, 3, verts);
		// gsKit_prim_sprite_texture(gsGlobal, &atlas, 0, 0, 0, 0, 324, 324, atlas.Width, atlas.Height, 0, TexCol);
		

		gsKit_queue_exec(gsGlobal);
		gsKit_sync_flip(gsGlobal);
		gsKit_TexManager_nextFrame(gsGlobal);
	}

	free(verts);

	return 0;
}
