#include <gsKit.h>
#include <dmaKit.h>


extern int test_box_main(int argc, char **argv);
extern int test_performance_main(int argc, char **argv);


int
main(int argc, char *argv[])
{
#if 1
	/*
	 *
	 * Init screen with gsKit
	 *
	 */
	GSGLOBAL *gsGlobal = gsKit_init_global();
	gsGlobal->Mode = GS_MODE_NTSC;
	gsGlobal->Interlace = GS_INTERLACED;
	gsGlobal->Field = GS_FRAME;
	gsGlobal->Width = 640;
	gsGlobal->Height = 448/2;//460/2;
	gsGlobal->DoubleBuffering = GS_SETTING_ON;
	gsGlobal->ZBuffering = GS_SETTING_ON;

	dmaKit_init(D_CTRL_RELE_OFF, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC,
		    D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);

	// Initialize the DMAC
	dmaKit_chan_init(DMA_CHANNEL_GIF);

	gsKit_init_screen(gsGlobal);
#endif

	return test_box_main(argc, argv);
//	return test_performance_main(argc, argv);
}
