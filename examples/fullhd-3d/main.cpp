//  ____     ___ |    / _____ _____
// |  __    |    |___/    |     |
// |___| ___|    |    \ __|__   |     gsKit Open Source Project.
// ----------------------------------------------------------------------
// Copyright 2004 - Chris "Neovanglist" Gilbert <Neovanglist@LainOS.org>
// Licenced under Academic Free License version 2.0
// Review gsKit README & LICENSE files for further details.
//
// basic.c - Example demonstrating basic gsKit operation.
//
#include <gsKit.h>
#include <dmaKit.h>
#include <malloc.h>
#include <math3d.h>
#include <kernel.h>

#include <draw.h>
#include <draw3d.h>

#include "teapot.c"


#define BG_DEBUG


VECTOR object_position = { 0.00f, 8.00f, 0.00f, 1.00f };
VECTOR object_rotation = { 2.70f, 0.00f, 0.00f, 1.00f };

VECTOR camera_position = { 0.00f, 0.00f,  80.00f, 1.00f };
VECTOR camera_rotation = { 0.00f, 0.00f,   0.00f, 1.00f };

int light_count = 4;
int iXOffset=0, iYOffset=0;

VECTOR light_direction[4] = {
	{  0.00f,  0.00f,  0.00f, 1.00f },
	{  1.00f,  0.00f, -1.00f, 1.00f },
	{  0.00f,  1.00f, -1.00f, 1.00f },
	{ -1.00f, -1.00f, -1.00f, 1.00f }
};

VECTOR light_colour[4] = {
	{ 0.00f, 0.00f, 0.00f, 1.00f },
	{ 0.60f, 1.00f, 0.60f, 1.00f },
	{ 0.30f, 0.30f, 0.30f, 1.00f },
	{ 0.50f, 0.50f, 0.50f, 1.00f }
};

int light_type[4] = {
	LIGHT_AMBIENT,
	LIGHT_DIRECTIONAL,
	LIGHT_DIRECTIONAL,
	LIGHT_DIRECTIONAL
};


s32 hsync_callback_id = -1;
volatile u32 vsync_count = 0;
volatile u32 hsync_count = 0;
s32 hsync_callback(s32 cause)
{
	if (*GS_CSR & (1<<2)) {
		*GS_CSR = (1<<2);
		hsync_count++;
	}

	if (*GS_CSR & (1<<3)) {
		*GS_CSR = (1<<3);
		vsync_count++;
		hsync_count=0;
	}

	//GS_SET_BGCOLOR(hsync_count/3, hsync_count/3, hsync_count/3);

	return 0;
}

void hsync_init()
{
	DIntr();
	hsync_callback_id = AddIntcHandler(INTC_GS, hsync_callback, -1);
	EnableIntc(INTC_GS);
	EIntr();

	GsPutIMR(0x0000F300);
}

void gsKit_setscissor(GSGLOBAL *gsGlobal, u16 x0, u16 x1, u16 y0, u16 y1)
{
	u64 *p_data;
	u64 *p_store;
	int qws = 3;

	p_data = p_store = (u64 *)gsGlobal->dma_misc;

	*p_data++ = GIF_TAG( qws-1, 1, 0, 0, 0, 1 );
	*p_data++ = GIF_AD;

	// Context 1

	*p_data++ = GS_SETREG_SCISSOR_1(x0, x1, y0, y1);
	*p_data++ = GS_SCISSOR_1;

	// Context 2

	*p_data++ = GS_SETREG_SCISSOR_1(x0, x1, y0, y1);
	*p_data++ = GS_SCISSOR_2;

	dmaKit_wait_fast();
	dmaKit_send_ucab(DMA_CHANNEL_GIF, p_store, qws);
}

int render(GSGLOBAL *gsGlobal)
{
	u64 White, Black, Red, Green, Blue, BlueTrans, RedTrans, GreenTrans, WhiteTrans;
	u32 iPass;
	u32 iYStart;
	int i;

	// Matrices to setup the 3D environment and camera
	MATRIX local_world;
	MATRIX local_light;
	MATRIX world_view;
	MATRIX view_screen;
	MATRIX local_screen;

	VECTOR *temp_normals;
	VECTOR *temp_lights;
	color_f_t *temp_colours;
	vertex_f_t *temp_vertices;

	xyz_t   *verts;
	color_t *colors;

	iYStart = gsGlobal->StartY + iYOffset + 2;
	if (gsGlobal->Interlace == GS_INTERLACED)
		iYStart /= 2;

	// Allocate calculation space.
	temp_normals  = (VECTOR     *)memalign(128, sizeof(VECTOR)     * vertex_count);
	temp_lights   = (VECTOR     *)memalign(128, sizeof(VECTOR)     * vertex_count);
	temp_colours  = (color_f_t  *)memalign(128, sizeof(color_f_t)  * vertex_count);
	temp_vertices = (vertex_f_t *)memalign(128, sizeof(vertex_f_t) * vertex_count);

	// Allocate register space.
	verts  = (xyz_t   *)memalign(128, sizeof(xyz_t)   * vertex_count);
	colors = (color_t *)memalign(128, sizeof(color_t) * vertex_count);

	White      = GS_SETREG_RGBAQ(0xFF,0xFF,0xFF,0x00,0x00);
	Black      = GS_SETREG_RGBAQ(0x00,0x00,0x00,0x00,0x00);
	Red        = GS_SETREG_RGBAQ(0xFF,0x00,0x00,0x00,0x00);
	Green      = GS_SETREG_RGBAQ(0x00,0xFF,0x00,0x00,0x00);
	Blue       = GS_SETREG_RGBAQ(0x00,0x00,0xFF,0x00,0x00);

	BlueTrans  = GS_SETREG_RGBAQ(0x00,0x00,0xFF,0x40,0x00);
	RedTrans   = GS_SETREG_RGBAQ(0xFF,0x00,0x00,0x60,0x00);
	GreenTrans = GS_SETREG_RGBAQ(0x00,0xFF,0x00,0x50,0x00);
	WhiteTrans = GS_SETREG_RGBAQ(0xFF,0xFF,0xFF,0x50,0x00);

	hsync_init();

	// Create the view_screen matrix.
	create_view_screen(view_screen, 16.0f/9.0f, -0.20f, 0.20f, -0.20f, 0.20f, 1.00f, 2000.00f);

	gsKit_mode_switch(gsGlobal, GS_ONESHOT);

	gsGlobal->PrimAlphaEnable = GS_SETTING_OFF;

	// The main loop...
	for (;;)
	{
		// Rendering -> RED backrgound color
		GS_SET_BGCOLOR(128, 0, 0);

		// Spin the teapot a bit.
		//object_rotation[0] += 0.008f; while (object_rotation[0] > 3.14f) { object_rotation[0] -= 6.28f; }
		object_rotation[1] += 0.005f; while (object_rotation[1] > 3.14f) { object_rotation[1] -= 6.28f; }

		// Create the local_world matrix.
		create_local_world(local_world, object_position, object_rotation);

		// Create the local_light matrix.
		create_local_light(local_light, object_rotation);

		// Create the world_view matrix.
		create_world_view(world_view, camera_position, camera_rotation);

		// Create the local_screen matrix.
		create_local_screen(local_screen, local_world, world_view, view_screen);

		// Calculate the normal values.
		calculate_normals(temp_normals, vertex_count, normals, local_light);

		// Calculate the lighting values.
		calculate_lights(temp_lights, vertex_count, temp_normals, light_direction, light_colour, light_type, light_count);

		// Calculate the colour values after lighting.
		calculate_colours((VECTOR *)temp_colours, vertex_count, colours, temp_lights);

		// Calculate the vertex values.
		calculate_vertices((VECTOR *)temp_vertices, vertex_count, vertices, local_screen);

		// Convert floating point vertices to fixed point and translate to center of screen.
		draw_convert_xyz(verts, 2048, 2048, 16, vertex_count, temp_vertices);

		// Convert floating point colours to fixed point.
		draw_convert_rgbq(colors, vertex_count, temp_vertices, temp_colours, 0x80);

		/*
		 * Render in a single buffer using 2 passes
		 */
		for (iPass = 0; iPass < 2; iPass++) {

			// Rendering -> RED backrgound color
			GS_SET_BGCOLOR(255, 0, 0);

			if (iPass == 0) {
				// Render TOP HALF
				gsKit_setscissor(gsGlobal, 0, gsGlobal->Width - 1, 0, (gsGlobal->Height/2) - 1);
				gsKit_clear(gsGlobal, Black);
			}
			else {
				// Render BOTTOM HALF
				gsKit_setscissor(gsGlobal, 0, gsGlobal->Width - 1, (gsGlobal->Height/2), (gsGlobal->Height) - 1);
				gsKit_clear(gsGlobal, White);
			}

			for (i = 0; i < points_count; i+=3) {
				float fX=gsGlobal->Width/2;
				float fY=gsGlobal->Height/2;
				gsKit_prim_triangle_gouraud_3d(gsGlobal
					, (temp_vertices[points[i+0]].x + 1.0f) * fX, (temp_vertices[points[i+0]].y + 1.0f) * fY, verts[points[i+0]].z
					, (temp_vertices[points[i+1]].x + 1.0f) * fX, (temp_vertices[points[i+1]].y + 1.0f) * fY, verts[points[i+1]].z
					, (temp_vertices[points[i+2]].x + 1.0f) * fX, (temp_vertices[points[i+2]].y + 1.0f) * fY, verts[points[i+2]].z
					, colors[points[i+0]].rgbaq, colors[points[i+1]].rgbaq, colors[points[i+2]].rgbaq);
			}

			// Waiting-> GREEN backrgound color
			GS_SET_BGCOLOR(0, 255, 0);
			if (iPass == 0) {
				// Wait for CRTC to be done with the TOP HALF
				while (hsync_count < (iYStart + gsGlobal->Height / 2))
					;
			}
			else {
				// Wait for CRTC to be done with the BOTTOM HALF
				while (hsync_count < (iYStart + gsGlobal->Height))
					;
			}

			// Drawing-> BLUE backrgound color
			GS_SET_BGCOLOR(0, 0, 255);
			gsKit_queue_exec(gsGlobal);
		}

		//gsKit_sync_flip(gsGlobal);
		//gsKit_switch_context(gsGlobal);
		//gsKit_display_buffer(gsGlobal);
		gsKit_setactive(gsGlobal);
	}

	free(temp_normals);
	free(temp_lights);
	free(temp_colours);
	free(temp_vertices);
	free(verts);
	free(colors);

	return 0;

}

int main(int argc, char *argv[])
{
	GSGLOBAL *gsGlobal = gsKit_init_global();

#if 0
	gsGlobal->Mode = GS_MODE_NTSC;
	gsGlobal->Interlace = GS_INTERLACED;
	//gsGlobal->Field = GS_FIELD;
	gsGlobal->Field = GS_FRAME;
	gsGlobal->Width = 704;
	gsGlobal->Height = 464/2;
	iXOffset = -32;
	iYOffset = 8;
#endif
#if 0
	gsGlobal->Mode = GS_MODE_PAL;
	gsGlobal->Interlace = GS_INTERLACED;
	//gsGlobal->Field = GS_FIELD;
	gsGlobal->Field = GS_FRAME;
	gsGlobal->Width = 704;
	gsGlobal->Height = 556/2;
	iXOffset = -10;
	iYOffset = 5;
#endif
#if 0
	gsGlobal->Mode = GS_MODE_DTV_480P;
	gsGlobal->Interlace = GS_NONINTERLACED;
	gsGlobal->Field = GS_FRAME;
	gsGlobal->Width = 704;
	gsGlobal->Height = 462;
	iXOffset = -4;
	iYOffset = 3;

	gsGlobal->PSM = GS_PSM_CT16;
	gsGlobal->PSMZ = GS_PSMZ_16;
	gsGlobal->Dithering = GS_SETTING_ON;
#endif
#if 0
	gsGlobal->Mode = GS_MODE_DTV_576P;
	gsGlobal->Interlace = GS_NONINTERLACED;
	gsGlobal->Field = GS_FRAME;
	gsGlobal->Width = 704;
	gsGlobal->Height = 556;
	iXOffset = 0;
	iYOffset = 0;

	gsGlobal->PSM = GS_PSM_CT16;
	gsGlobal->PSMZ = GS_PSMZ_16;
	gsGlobal->Dithering = GS_SETTING_ON;
#endif
#if 1
	gsGlobal->Mode = GS_MODE_DTV_720P;
	gsGlobal->Interlace = GS_NONINTERLACED;
	gsGlobal->Field = GS_FRAME;
	gsGlobal->Width = 1280;
	gsGlobal->Height = 704;
	iXOffset = -114;
	iYOffset = -15;

	gsGlobal->PSM = GS_PSM_CT16;
	gsGlobal->PSMZ = GS_PSMZ_16;
	gsGlobal->Dithering = GS_SETTING_ON;
#endif
#if 0
	gsGlobal->Mode = GS_MODE_DTV_1080I;
	gsGlobal->Interlace = GS_INTERLACED;
	//gsGlobal->Field = GS_FIELD;
	gsGlobal->Field = GS_FRAME;
	gsGlobal->Width  = 1920-128;
	gsGlobal->Height = 1024 / 2;
	iXOffset = -50;
	iYOffset = -190;

	gsGlobal->PSM = GS_PSM_CT16;
	gsGlobal->PSMZ = GS_PSMZ_16;
	gsGlobal->Dithering = GS_SETTING_ON;
#endif

#ifdef BG_DEBUG
	gsGlobal->Width -= 64;
#endif

	gsGlobal->DoubleBuffering = GS_SETTING_OFF;
	gsGlobal->ZBuffering = GS_SETTING_ON;

	dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
		    D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);

	// Initialize the DMAC
	dmaKit_chan_init(DMA_CHANNEL_GIF);

	gsKit_init_screen(gsGlobal);
	gsKit_set_display_offset(gsGlobal, iXOffset, iYOffset);
	//printf("%d %d %d %d %d %d\n", gsGlobal->StartX, gsGlobal->StartY, gsGlobal->DW, gsGlobal->DH, gsGlobal->MagH, gsGlobal->MagV);

	gsKit_set_test(gsGlobal, GS_ZTEST_OFF);
	if (gsGlobal->ZBuffering == GS_SETTING_ON)
		gsKit_set_test(gsGlobal, GS_ZTEST_ON);

	render(gsGlobal);

	return 0;
}
