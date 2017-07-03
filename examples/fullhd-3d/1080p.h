#ifndef PATCH_1080P_H
#define PATCH_1080P_H

#ifdef __cplusplus
extern "C" {
#endif


/* Added modes. */
#define GS_MODE_DTV_576P	0x53
#define GS_MODE_DTV_1080P	0x54


void patch_1080p();
void unpatch_1080p();


#ifdef __cplusplus
}
#endif

#endif
