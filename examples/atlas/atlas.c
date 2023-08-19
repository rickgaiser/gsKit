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

	u64 White = GS_SETREG_RGBAQ(0xFF,0xFF,0xFF,0x00,0x00);

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

	atlas.Delayed = 1;
	gsKit_texture_png(gsGlobal, &atlas, "runner_atlas.png");
	gsKit_TexManager_bind(gsGlobal, &atlas);
	printf("Atlas Height: %i\n",atlas.Height);
	printf("Atlas Width: %i\n",atlas.Width);

	printf("Atlas VRAM Range = 0x%X - 0x%X\n",atlas.Vram, atlas.Vram +gsKit_texture_size(atlas.Width, atlas.Height, atlas.PSM) - 1);

	gsKit_set_clamp(gsGlobal, GS_CMODE_CLAMP);

	struct SPoint {
		u64 uv;
		u64 xyz2;
	};
	struct SPoint verts[8];

	// sprite 1
	verts[0].uv    = GS_SETREG_UV(gsKit_float_to_int_u(&atlas, 0), gsKit_float_to_int_v(&atlas, 1));
	verts[0].xyz2  = GS_SETREG_XYZ2(gsKit_float_to_int_x(gsGlobal, 0), gsKit_float_to_int_y(gsGlobal, 0), 0);
	verts[1].uv    = GS_SETREG_UV(gsKit_float_to_int_u(&atlas, 324), gsKit_float_to_int_v(&atlas, 324));
	verts[1].xyz2  = GS_SETREG_XYZ2(gsKit_float_to_int_x(gsGlobal, 100), gsKit_float_to_int_y(gsGlobal, 100), 0);

	// sprite 2
	verts[2].uv    = GS_SETREG_UV(gsKit_float_to_int_u(&atlas, 0), gsKit_float_to_int_v(&atlas, 1));
	verts[2].xyz2  = GS_SETREG_XYZ2(gsKit_float_to_int_x(gsGlobal, 100), gsKit_float_to_int_y(gsGlobal, 0), 0);
	verts[3].uv    = GS_SETREG_UV(gsKit_float_to_int_u(&atlas, 324), gsKit_float_to_int_v(&atlas, 324));
	verts[3].xyz2  = GS_SETREG_XYZ2(gsKit_float_to_int_x(gsGlobal, 200), gsKit_float_to_int_y(gsGlobal, 100), 0);

	// sprite 3
	verts[4].uv    = GS_SETREG_UV(gsKit_float_to_int_u(&atlas, 0), gsKit_float_to_int_v(&atlas, 1));
	verts[4].xyz2  = GS_SETREG_XYZ2(gsKit_float_to_int_x(gsGlobal, 0), gsKit_float_to_int_y(gsGlobal, 100), 0);
	verts[5].uv    = GS_SETREG_UV(gsKit_float_to_int_u(&atlas, 324), gsKit_float_to_int_v(&atlas, 324));
	verts[5].xyz2  = GS_SETREG_XYZ2(gsKit_float_to_int_x(gsGlobal, 100), gsKit_float_to_int_y(gsGlobal, 200), 0);

	// sprite 4
	verts[6].uv    = GS_SETREG_UV(gsKit_float_to_int_u(&atlas, 0), gsKit_float_to_int_v(&atlas, 1));
	verts[6].xyz2  = GS_SETREG_XYZ2(gsKit_float_to_int_x(gsGlobal, 100), gsKit_float_to_int_y(gsGlobal, 100), 0);
	verts[7].uv    = GS_SETREG_UV(gsKit_float_to_int_u(&atlas, 324), gsKit_float_to_int_v(&atlas, 324));
	verts[7].xyz2  = GS_SETREG_XYZ2(gsKit_float_to_int_x(gsGlobal, 200), gsKit_float_to_int_y(gsGlobal, 200), 0);

	while(1)
	{
		gsKit_clear(gsGlobal, White);

		gskit_prim_list_sprite_texture_uv_3d(gsGlobal, &atlas, 4, verts);

		gsKit_queue_exec(gsGlobal);
		gsKit_sync_flip(gsGlobal);
		gsKit_TexManager_nextFrame(gsGlobal);
	}

	return 0;
}
