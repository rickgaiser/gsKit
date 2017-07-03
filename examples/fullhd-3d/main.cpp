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
#include <gsToolkit.h>
#include <gsInline.h>
#include <dmaKit.h>
#include <malloc.h>
#include <math3d.h>
#include <kernel.h>

#include <draw.h>
#include <draw3d.h>

#include "1080p.h"

#include "teapot.c"


/*
 *  -------------------------------------------------------------------------
 *  2 pass rendering for 1920x1024i:
 *  Note: Minimum speed is 60fps == 120 passes/sec
 *  Note: Blocks represent 1920x256 pixels, or about 1MiB
 *  -------------------------------------------------------------------------
 *
 *            Render Pass1                      Render Pass2
 *
 *  +------------------------------+  +------------------------------+
 *  |         Front-Buffer         |  |             CRTC             |
 *  +------------------------------+  +------------------------------+
 *  |             CRTC             |  |         Front-Buffer         |
 *  +------------------------------+  +------------------------------+
 *
 *  +------------------------------+
 *  |         Depth-Buffer         |
 *  +------------------------------+
 *  |           Textures           |
 *  +------------------------------+
 *
 *
 *  -------------------------------------------------------------------------
 *  3 pass rendering for 1280x720p:
 *  Note: Minimum speed is 60fps == 180 passes/sec
 *  Note: Blocks represent 1280x256 pixels, or 640KiB
 *  -------------------------------------------------------------------------
 *
 *            Render Pass1                      Render Pass2                      Render Pass3
 *
 *  +------------------------------+  +------------------------------+  +------------------------------+
 *  |         Front-Buffer         |  |             CRTC             |  |         Depth-Buffer         |
 *  +------------------------------+  +------------------------------+  +------------------------------+
 *  |         Depth-Buffer         |  |         Front-Buffer         |  |             CRTC             |
 *  +------------------------------+  +------------------------------+  +------------------------------+
 *  |             CRTC             |  |         Depth-Buffer         |  |         Front-Buffer         |
 *  +------------------------------+  +------------------------------+  +------------------------------+
 *
 *  +------------------------------+
 *  |           Textures           |
 *  |           Textures           |
 *  |           Textures           |
 *  +------------------------------+
 *
 *
 *  -------------------------------------------------------------------------
 *  4 pass rendering for 1920x1024p:
 *  Note: Minimum speed is 60fps == 240 passes/sec
 *  Note: Blocks represent 1920x256 pixels, or about 1MiB
 *  -------------------------------------------------------------------------
 *
 *            Render Pass1                      Render Pass2                      Render Pass3                      Render Pass4
 *
 *  +------------------------------+  +------------------------------+  +------------------------------+  +------------------------------+
 *  |         Front-Buffer         |  |             CRTC             |  |         Depth-Buffer         |  |         Depth-Buffer         |
 *  +------------------------------+  +------------------------------+  +------------------------------+  +------------------------------+
 *  |           Textures           |  |         Front-Buffer         |  |             CRTC             |  |           Textures           |
 *  +------------------------------+  +------------------------------+  +------------------------------+  +------------------------------+
 *  |         Depth-Buffer         |  |         Depth-Buffer         |  |         Front-Buffer         |  |             CRTC             |
 *  +------------------------------+  +------------------------------+  +------------------------------+  +------------------------------+
 *  |             CRTC             |  |           Textures           |  |           Textures           |  |         Front-Buffer         |
 *  +------------------------------+  +------------------------------+  +------------------------------+  +------------------------------+
 *
 */


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
int hsync_callback()
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

#ifdef BG_DEBUG
	//GS_SET_BGCOLOR(hsync_count/3, hsync_count/3, hsync_count/3);
#endif

	return 0;
}

typedef struct SBlock {
	u32 begin;
	u32 end;
	u32 addr;
};

typedef struct SPass {
	SBlock * ScreenBlock;
	SBlock * DepthBlock;
	SBlock * TextBlock;
	SBlock * CRTCBlock;
	GSQUEUE Queue;
};

void gsKit_setpass(GSGLOBAL *gsGlobal, const SPass * pass)
{
	u64 *p_data;
	u64 *p_store;
	int qsize = 8;

	p_data = p_store = (u64*)gsKit_heap_alloc(gsGlobal, qsize, (qsize * 16), GIF_AD);

	*p_data++ = GIF_TAG_AD(qsize);
	*p_data++ = GIF_AD;

	// Context 1
	*p_data++ = GS_SETREG_SCISSOR_1(0, gsGlobal->Width - 1, 0, pass->ScreenBlock->end - pass->ScreenBlock->begin);
	*p_data++ = GS_SCISSOR_1;
	*p_data++ = GS_SETREG_FRAME_1(pass->ScreenBlock->addr / 8192, gsGlobal->Width / 64, gsGlobal->PSM, 0);
	*p_data++ = GS_FRAME_1;
	*p_data++ = GS_SETREG_ZBUF_1(pass->DepthBlock->addr / 8192, gsGlobal->PSMZ, 0);
	*p_data++ = GS_ZBUF_1;
	*p_data++ = GS_SETREG_XYOFFSET_1(gsGlobal->OffsetX, gsGlobal->OffsetY + pass->ScreenBlock->begin*16);
	*p_data++ = GS_XYOFFSET_1;

	// Context 2
	*p_data++ = GS_SETREG_SCISSOR_1(0, gsGlobal->Width - 1, 0, pass->ScreenBlock->end - pass->ScreenBlock->begin);
	*p_data++ = GS_SCISSOR_2;
	*p_data++ = GS_SETREG_FRAME_1(pass->ScreenBlock->addr / 8192, gsGlobal->Width / 64, gsGlobal->PSM, 0);
	*p_data++ = GS_FRAME_2;
	*p_data++ = GS_SETREG_ZBUF_1(pass->DepthBlock->addr / 8192, gsGlobal->PSMZ, 0);
	*p_data++ = GS_ZBUF_2;
	*p_data++ = GS_SETREG_XYOFFSET_1(gsGlobal->OffsetX, gsGlobal->OffsetY + pass->ScreenBlock->begin*16);
	*p_data++ = GS_XYOFFSET_2;
}

void gsKit_queue_send(GSGLOBAL *gsGlobal, GSQUEUE *Queue)
{
	if(Queue->tag_size == 0)
		return;

	*(u64 *)Queue->dma_tag = DMA_TAG(Queue->tag_size, 0, DMA_END, 0, 0, 0);

	if(Queue->last_type != GIF_AD)
	{
		*(u64 *)Queue->last_tag = ((u64)Queue->same_obj | *(u64 *)Queue->last_tag);
	}

	dmaKit_wait_fast();
	dmaKit_send_chain_ucab(DMA_CHANNEL_GIF, Queue->pool[Queue->dbuf]);
}

#define MAX_PASS_COUNT 4
int render(GSGLOBAL *gsGlobal, u32 iPassCount)
{
	GSTEXTURE bigtex;
	u64 White, Black, Red, Green, Blue, BlueTrans, RedTrans, GreenTrans, WhiteTrans, TexCol;
	u32 iPass;
	u32 iYStart;
	int i;
	SBlock block[MAX_PASS_COUNT];
	SPass  pass[MAX_PASS_COUNT];
	u32 iScanline = 0;
	u32 iPassSize = gsGlobal->Height / iPassCount;
	// Align to 64 line blocks
	iPassSize = (iPassSize + 32) & ~63;

	for (iPass = 0; iPass < iPassCount; iPass++) {
		block[iPass].begin = iScanline;
		block[iPass].addr = iScanline * gsGlobal->Width * 2;

		iScanline += iPassSize;
		if (iScanline > gsGlobal->Height)
			iScanline = gsGlobal->Height;

		block[iPass].end = iScanline - 1;

		gsKit_queue_init(gsGlobal, &pass[iPass].Queue, GS_PERSISTENT, 1024);

		printf("Block %d: %d - %d\n", iPass, block[iPass].begin, block[iPass].end);
	}

	pass[0].ScreenBlock = &block[0];
	pass[0].DepthBlock  = &block[2];
	pass[0].TextBlock   = &block[1];
	pass[0].CRTCBlock   = &block[3];

	pass[1].ScreenBlock = &block[1];
	pass[1].DepthBlock  = &block[2];
	pass[1].TextBlock   = &block[3];
	pass[1].CRTCBlock   = &block[0];

	pass[2].ScreenBlock = &block[2];
	pass[2].DepthBlock  = &block[0];
	pass[2].TextBlock   = &block[3];
	pass[2].CRTCBlock   = &block[1];

	pass[3].ScreenBlock = &block[3];
	pass[3].DepthBlock  = &block[0];
	pass[3].TextBlock   = &block[1];
	pass[3].CRTCBlock   = &block[2];

	bigtex.Width = 640;
	bigtex.Height = 480;
	bigtex.PSM = GS_PSM_CT24;
	bigtex.Filter = GS_FILTER_LINEAR;
	gsKit_texture_bmp(gsGlobal, &bigtex, "host:bigtex.bmp");

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

	iYStart = gsGlobal->StartY + iYOffset + 4;
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

	TexCol     = GS_SETREG_RGBAQ(0x80,0x80,0x80,0x80,0x00);

	gsKit_add_hsync_handler(hsync_callback);

	// Create the view_screen matrix.
	create_view_screen(view_screen, 16.0f/9.0f, -0.20f, 0.20f, -0.20f, 0.20f, 1.00f, 2000.00f);

	// Create pass queues
	for (iPass = 0; iPass < iPassCount; iPass++) {
		gsKit_queue_set(gsGlobal, &pass[iPass].Queue);
		gsKit_setpass(gsGlobal, &pass[iPass]);
	}

	// The main loop...
	for (;;)
	{
#ifdef BG_DEBUG
		// Rendering -> RED backrgound color
		GS_SET_BGCOLOR(128, 0, 0);
#endif

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
		 * Render into persistent queue, so it will be drawn for all passes
		 */
		gsKit_mode_switch(gsGlobal, GS_PERSISTENT);
		gsKit_clear(gsGlobal, GS_SETREG_RGBAQ(128, 128, 128, 0, 0));
#if 1
		//gsKit_prim_sprite_striped_texture(gsGlobal, &bigtex, 0.0f,  // X1
		gsKit_prim_sprite_texture(gsGlobal, &bigtex, 0.0f,  // X1
								 0.0f,  // Y2
								 0.0f,  // U1
								 0.0f,  // V1
								 gsGlobal->Width, // X2
								 gsGlobal->Height, // Y2
								 bigtex.Width, // U2
								 bigtex.Height, // V2
								 2,
								 TexCol);
#endif
#if 1
		gsKit_set_primalpha(gsGlobal, GS_SETREG_ALPHA(0, 1, 0, 1, 128), 0);
		gsGlobal->PrimAlphaEnable = GS_SETTING_OFF;
		gsGlobal->PrimAAEnable = GS_SETTING_ON;
		for (i = 0; i < points_count; i+=3) {
			float fX=gsGlobal->Width/2;
			float fY=gsGlobal->Height/2;
			gsKit_prim_triangle_gouraud_3d(gsGlobal
				, (temp_vertices[points[i+0]].x + 1.0f) * fX, (temp_vertices[points[i+0]].y + 1.0f) * fY, verts[points[i+0]].z
				, (temp_vertices[points[i+1]].x + 1.0f) * fX, (temp_vertices[points[i+1]].y + 1.0f) * fY, verts[points[i+1]].z
				, (temp_vertices[points[i+2]].x + 1.0f) * fX, (temp_vertices[points[i+2]].y + 1.0f) * fY, verts[points[i+2]].z
				, colors[points[i+0]].rgbaq, colors[points[i+1]].rgbaq, colors[points[i+2]].rgbaq);
		}
#endif

		/*
		 * Render in a single buffer using multiple passes
		 * Passes render into the oneshot queue.
		 * The oneshot queue must be executed first to setup the pass
		 */
		for (iPass = 0; iPass < iPassCount; iPass++) {
			int bgi = (255 / (iPassCount+1)) * (iPass+1);
			int iPrev = (iPass == 0) ? (iPassCount-1) : (iPass-1);

#ifdef BG_DEBUG
			GS_SET_BGCOLOR(bgi,bgi,bgi);
#endif

			// Wait for CRTC
			while ( (hsync_count <= (iYStart + pass[iPrev].CRTCBlock->end))
				  )
				;

#ifdef BG_DEBUG
			GS_SET_BGCOLOR(0, 0, 0);
#endif

			// Send current pass queue
			gsKit_queue_send(gsGlobal, &pass[iPass].Queue);
			// Send current frame queue
			gsKit_queue_send(gsGlobal, gsGlobal->Per_Queue);
		}
		gsKit_queue_reset(gsGlobal->Per_Queue);
		dmaKit_wait_fast();
	}

	// Free pass queues
	for (iPass = 0; iPass < iPassCount; iPass++) {
		gsKit_queue_free(gsGlobal, &pass[iPass].Queue);
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
	s8 dither_matrix[16] = {-4,2,-3,3,0,-2,1,-1,-3,3,-4,2,1,-1,0,-2};
	//s8 dither_matrix[16] = {4,2,5,3,0,6,1,7,5,3,4,2,1,7,0,6}; //different matrix
	GSGLOBAL *gsGlobal = gsKit_init_global();
	int i;

#if 0
	gsGlobal->Mode = GS_MODE_NTSC;
	gsGlobal->Interlace = GS_INTERLACED;
	//gsGlobal->Field = GS_FIELD;
	gsGlobal->Field = GS_FRAME;
	gsGlobal->Width = 704;
	gsGlobal->Height = 464;
	iXOffset = -32;
	iYOffset = 8;
#endif
#if 0
	gsGlobal->Mode = GS_MODE_PAL;
	gsGlobal->Interlace = GS_INTERLACED;
	//gsGlobal->Field = GS_FIELD;
	gsGlobal->Field = GS_FRAME;
	gsGlobal->Width = 704;
	gsGlobal->Height = 556;
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
#if 0
	gsGlobal->Mode = GS_MODE_DTV_720P;
	gsGlobal->Interlace = GS_NONINTERLACED;
	gsGlobal->Field = GS_FRAME;
	gsGlobal->Width = 1280;
	gsGlobal->Height = 720;//704;
	iXOffset = -114;
	iYOffset = -15;

	gsGlobal->PSM = GS_PSM_CT16;
	gsGlobal->PSMZ = GS_PSMZ_16;
	gsGlobal->Dithering = GS_SETTING_ON;
#endif
#if 1
	gsGlobal->Mode = GS_MODE_DTV_1080I;
	gsGlobal->Interlace = GS_INTERLACED;
	//gsGlobal->Field = GS_FIELD;
	gsGlobal->Field = GS_FRAME;
	gsGlobal->Width  = 1920;
	gsGlobal->Height = 1024;
	iXOffset = -66;
	iYOffset = -201;

	gsGlobal->PSM = GS_PSM_CT16;
	gsGlobal->PSMZ = GS_PSMZ_16;
	gsGlobal->Dithering = GS_SETTING_ON;
#endif
#if 0
	gsGlobal->Mode = GS_MODE_DTV_1080P;
	gsGlobal->Interlace = GS_NONINTERLACED;
	gsGlobal->Field = GS_FRAME;
	gsGlobal->Width  = 1920;
	gsGlobal->Height = 1024;
	iXOffset = -66;
	iYOffset = -201;

	gsGlobal->PSM = GS_PSM_CT16;
	gsGlobal->PSMZ = GS_PSMZ_16;
	gsGlobal->Dithering = GS_SETTING_ON;

	gsGlobal->DW=1920;
	gsGlobal->DH=1080;
	gsGlobal->StartX=300;
	gsGlobal->StartY=238;
#endif
#if 0
	gsGlobal->Mode = GS_MODE_VGA_1280_60;
	gsGlobal->Interlace = GS_NONINTERLACED;
	gsGlobal->Field = GS_FRAME;
	gsGlobal->Width = 1280;
	gsGlobal->Height = 1024;
	iXOffset = 0;
	iYOffset = 0;

	gsGlobal->PSM = GS_PSM_CT16;
	gsGlobal->PSMZ = GS_PSMZ_16;
	gsGlobal->Dithering = GS_SETTING_ON;
#endif


	if ((gsGlobal->Interlace == GS_INTERLACED) && (gsGlobal->Field == GS_FRAME))
		gsGlobal->Height /= 2;

#ifdef BG_DEBUG
	if (gsGlobal->Width < 1024)
		gsGlobal->Width -=  64;
	else
		gsGlobal->Width -= 128;
#endif

	patch_1080p();

	for(i = 0; i < 15; i++) {
		gsGlobal->DitherMatrix[i] = dither_matrix[i];
	}

	gsGlobal->DoubleBuffering = GS_SETTING_OFF;
	gsGlobal->ZBuffering = GS_SETTING_OFF;
	//gsGlobal->ZBuffering = GS_SETTING_ON;

	dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
		    D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);

	// Initialize the DMAC
	dmaKit_chan_init(DMA_CHANNEL_GIF);

	gsKit_init_screen(gsGlobal);
	gsKit_set_display_offset(gsGlobal, iXOffset, iYOffset);
	//printf("%d %d %d %d %d %d\n", gsGlobal->StartX, gsGlobal->StartY, gsGlobal->DW, gsGlobal->DH, gsGlobal->MagH, gsGlobal->MagV);

	//gsKit_set_test(gsGlobal, GS_ZTEST_OFF);
	//if (gsGlobal->ZBuffering == GS_SETTING_ON)
		gsKit_set_test(gsGlobal, GS_ZTEST_ON);

	render(gsGlobal, 4);

	unpatch_1080p();

	return 0;
}
