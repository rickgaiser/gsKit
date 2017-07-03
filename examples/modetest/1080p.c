#include "1080p.h"
#include <kernel.h>
#include <syscallnr.h>
#include <gsKit.h>


/* Taken from gs_privileged.h */
#define GS_REG_SMODE1		(volatile u64 *)0x12000010	// VHP,VCKSEL,SLCK2,NVCK,CLKSEL,PEVS,PEHS,PVS,PHS,GCONT,SPML,PCK2,XPCK,SINT,PRST,EX,CMOD,SLCK,T1248,LC,RC
#define GS_REG_SMODE2		(volatile u64 *)0x12000020	// Setting For Modes Related to Video Synchronization
#define GS_REG_SRFSH		(volatile u64 *)0x12000030	// DRAM Refresh Settings
#define GS_REG_SYNCH1		(volatile u64 *)0x12000040	// HS,HSVS,HSEQ,HBP,HFP
#define GS_REG_SYNCH2		(volatile u64 *)0x12000050	// HB,HF
#define GS_REG_SYNCHV		(volatile u64 *)0x12000060	// VS,VDP,VBPE,VBP,VFPE,VFP
#define GS_REG_CSR		(volatile u64 *)0x12001000	// System Status


static void (*ori_SetGsCrt)(unsigned short int interlace, unsigned short int mode, unsigned short int ffmd) = NULL;
static unsigned int hvParam;


static inline void Setup576P(void)
{
	unsigned int temp, i;

	temp=(hvParam&1)<<25;

	*GS_REG_SMODE1=0x00000017404B0504|temp;
	*GS_REG_SYNCH1=0x000402E02003C827;
	*GS_REG_SYNCH2=0x0019ca67;
	*GS_REG_SYNCHV=0x00A9000002700005;
	*GS_REG_SMODE2=0;
	*GS_REG_SRFSH=4;
	*GS_REG_SMODE1=0x0000001740490504|temp;

	for(i=0x7A120; i>0; i--) __asm("nop\nnop\nnop\nnop\nnop");

	*GS_REG_SMODE1=0x0000001740480504|temp;	/* Have bits 16 and 17 cleared. */
}

static inline void Setup1080P(void)
{
	unsigned int i, temp;

	temp=hvParam;

	*GS_REG_SMODE1=0x00000003402304B1|(temp&1)<<25;

	if((*GS_REG_CSR>>16&0xFF)>=0x19){
		*GS_REG_SYNCH1=0x000344200B04182D;
		*GS_REG_SYNCH2=0x0020FB61;
	}
	else{
		*GS_REG_SYNCH1=0x000344200B043829;
		*GS_REG_SYNCH2=0x0020EB63;
	}

	*GS_REG_SYNCHV=0x0150E00201C00005;
	*GS_REG_SMODE2=0;	/* We want 1080P. */
	*GS_REG_SRFSH=4;
	*GS_REG_SMODE1=0x00000003402204B1|(temp&1)<<25;

	for(i=0x7A120; i>0; i--){
		/* At the actual loop at 0x8000af90, it isn't 5 nops;
			It's 4 nops with the decrementing instruction in the middle. */
		__asm("nop\nnop\nnop\nnop\nnop");
	}

	*GS_REG_SMODE1=0x00000003402004B1|(temp&1)<<25;
}

/* This function will get called first, in place of the original SetGsCrt() syscall handler. It will check the video mode argument.
	If it's one of the extended video modes, do not call the original SetGsCrt() syscall handler, but handle GS configuration on our own.
	Otherwise, call the original SetGsCrt() syscall handler and let it configure the GS for us.
*/
static void SetGsCrt_patch(unsigned short int interlace, unsigned short int mode, unsigned short int ffmd)
{
	switch(mode){
		case GS_MODE_DTV_576P:
			Setup576P();
			break;
		case GS_MODE_DTV_1080P:
			Setup1080P();
			break;
		default:
			ori_SetGsCrt(interlace, mode, ffmd);
	}
}

void patch_1080p()
{
	if (ori_SetGsCrt == NULL) {
		hvParam = GetGsVParam();

		/* Patch syscall handlers. */
		ori_SetGsCrt = GetSyscallHandler(__NR_SetGsCrt);
		SetSyscall(__NR_SetGsCrt, &SetGsCrt_patch);
	}
}

void unpatch_1080p()
{
	if (ori_SetGsCrt != NULL) {
		/* Restore the original SetGsCrt() syscall handler. */
		SetSyscall(__NR_SetGsCrt, ori_SetGsCrt);
		ori_SetGsCrt = NULL;
	}
}
