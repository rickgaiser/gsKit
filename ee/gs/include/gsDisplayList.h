#ifndef __GSDISPLAYLIST_H__
#define __GSDISPLAYLIST_H__


#include "gsKit.h"


// gsKit adapted type
typedef struct gsTexture gsTexture_t;


//---------------------------------------------------------------------------
// Display List / Command Queue
//---------------------------------------------------------------------------
enum E_GSDL_COLOR_MODE {
    EDL_CM_NONE = 0, // Color set before transferring primitives
    EDL_CM_PRIM = 1, // Color set once per primitive (flat shading)
    EDL_CM_VERT = 2  // Color set per vertex (smooth shading)
};

enum E_GSDL_GIF_MODE {
    EDL_GM_NONE     = 0,
    EDL_GM_SPRITE   = 2,
    EDL_GM_TRIANGLE = 3,
    EDL_GM_QUAD     = 4,
    EDL_GM_AD       = 5
};

struct gsDisplayList {
    GSGLOBAL *gsGlobal;
    void *data;
    u64 *p_giftag;
    u64 *p_data;
    int qw;

    enum E_GSDL_COLOR_MODE eCM;
    enum E_GSDL_GIF_MODE eGM;
    u64 current_rgbaq;
    int prim_count;
    int vert_count;
    int vpp; // vertices per primitive
};
typedef struct gsDisplayList gsDisplayList_t;


gsDisplayList_t * dlCreate(GSGLOBAL *gsGlobal, int qw);
void dlFree(gsDisplayList_t *dl);

u32 dlSize(gsDisplayList_t *dl);
u32 dlQWSize(gsDisplayList_t *dl);
void dlReset(gsDisplayList_t *dl);

void dlBegin(gsDisplayList_t *dl, enum E_GSDL_GIF_MODE pm);
void dlEnd(gsDisplayList_t *dl);

void dlColorU64(gsDisplayList_t *dl, u64 color);
void dlColor4b(gsDisplayList_t *dl, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void dlColor3f(gsDisplayList_t *dl, float r, float g, float b);

void dlVertex3x(gsDisplayList_t *dl, int x, int y, int z); // fixed point pixels (x16), GS native format
void dlVertex3i(gsDisplayList_t *dl, int x, int y, int z);
void dlVertex3f(gsDisplayList_t *dl, float x, float y, float z);

void dlAD(gsDisplayList_t *dl, u64 a, u64 d);
void dlADSetDrawEnv1(gsDisplayList_t *dl, gsTexture_t *t);
void dlADSetDrawEnv2(gsDisplayList_t *dl, gsTexture_t *t);

void gsSetDisplay(gsTexture_t *t);

#endif
