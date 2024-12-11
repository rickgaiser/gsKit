#include "gsKit.h"
#include "gsInline.h"

#include <malloc.h>


//---------------------------------------------------------------------------
GSDL * dlCreate(GSGLOBAL *gsGlobal, int qw)
{
    printf("%s\n", __FUNCTION__);

    GSDL *dl = calloc(1, sizeof(GSDL));
    dl->gsGlobal = gsGlobal;
    dl->data = gsKit_alloc_ucab(qw * 16);
    dl->p_data = dl->data;
    dl->qw = qw;
    dl->eCM = EDL_CM_PRIM;
    dl->ePM = EDL_PM_NONE;
    dl->current_rgbaq = GS_SETREG_RGBAQ(0, 0, 0, 0, 0);
    dl->prim_count = 0;
    dl->vert_count = 0;

    return dl;
}

//---------------------------------------------------------------------------
void dlFree(GSDL *dl)
{
    printf("%s\n", __FUNCTION__);

    gsKit_free_ucab(dl->data);
    free(dl);
}

//---------------------------------------------------------------------------
u32 dlSize(GSDL *dl)
{
    //printf("%s\n", __FUNCTION__);

    return (u8*)dl->p_data - (u8*)dl->data;
}

//---------------------------------------------------------------------------
u32 dlQWSize(GSDL *dl)
{
    //printf("%s\n", __FUNCTION__);

    return dlSize(dl) / 16;
}

//---------------------------------------------------------------------------
void dlQueue(GSDL *dl)
{
    printf("%s: size = %db / %dqw\n", __FUNCTION__, dlSize(dl), dlQWSize(dl));

	u64* p_data = gsKit_heap_alloc(dl->gsGlobal, dlQWSize(dl), dlSize(dl), GSKIT_GIF_PRIM_SPRITE);
    memcpy(p_data, dl->data, dlSize(dl));
}

//---------------------------------------------------------------------------
void dlReset(GSDL *dl)
{
    //printf("%s\n", __FUNCTION__);
}

//---------------------------------------------------------------------------
void dlBegin(GSDL *dl, enum E_GSDL_PRIM_MODE pm)
{
    printf("%s\n", __FUNCTION__);

    if (dl->ePM != EDL_PM_NONE) {
        printf("%s: ERROR\n", __FUNCTION__);
        return;
    }

    dl->ePM = pm;
    dl->prim_count = 0;
    dl->vert_count = 0;

    // Save GIFTAG for later
    dl->p_giftag = dl->p_data;
    dl->p_data += 2;

    switch (dl->ePM) {
        case EDL_PM_SPRITE:   dl->vpp = 2; break;
        case EDL_PM_TRIANGLE: dl->vpp = 3; break;
        case EDL_PM_QUAD:     dl->vpp = 4; break;
    }
}

//---------------------------------------------------------------------------
void dlEnd(GSDL *dl)
{
    printf("%s: %d primitives\n", __FUNCTION__, dl->prim_count);

    if (dl->ePM == EDL_PM_NONE) {
        printf("%s: ERROR 1\n", __FUNCTION__);
        return;
    }

    if (dl->vert_count != 0) {
        printf("%s: ERROR 2\n", __FUNCTION__);
        return;
    }

    if (dl->prim_count == 0) {
        printf("%s: ERROR 3\n", __FUNCTION__);
        return;
    }

    // Complete the giftag
    switch (dl->ePM) {
        case EDL_PM_SPRITE:
            dl->p_giftag[0] = GIF_TAG(
                dl->prim_count,
                1, // End of Packet ?
                1, // PRIM enable
                GS_SETREG_PRIM(
                    GS_PRIM_PRIM_SPRITE,
                    0,
                    0,
                    dl->gsGlobal->PrimFogEnable,
                    dl->gsGlobal->PrimAlphaEnable,
                    dl->gsGlobal->PrimAAEnable,
                    0,
                    dl->gsGlobal->PrimContext,
                    0),
                GSKIT_GIF_FLG_REGLIST,
                3);
            dl->p_giftag[1] =
                ((u64)(GS_RGBAQ) << 0) | \
                ((u64)(GS_XYZ2)  << 4) | \
                ((u64)(GS_XYZ2)  << 8);
            break;
        case EDL_PM_TRIANGLE:
            dl->p_giftag[0] = GIF_TAG(
                dl->prim_count,
                1, // End of Packet ?
                1, // PRIM enable
                GS_SETREG_PRIM(
                    GS_PRIM_PRIM_TRIANGLE,
                    0,
                    0,
                    dl->gsGlobal->PrimFogEnable,
                    dl->gsGlobal->PrimAlphaEnable,
                    dl->gsGlobal->PrimAAEnable,
                    0,
                    dl->gsGlobal->PrimContext,
                    0),
                GSKIT_GIF_FLG_REGLIST,
                4);
            dl->p_giftag[1] =
                ((u64)(GS_RGBAQ) <<  0) | \
                ((u64)(GS_XYZ2)  <<  4) | \
                ((u64)(GS_XYZ2)  <<  8) | \
                ((u64)(GS_XYZ2)  << 12);
            break;
        case EDL_PM_QUAD:
            dl->p_giftag[0] = GIF_TAG(
                dl->prim_count,
                1, // End of Packet ?
                1, // PRIM enable
                GS_SETREG_PRIM(
                    GS_PRIM_PRIM_TRISTRIP,
                    0,
                    0,
                    dl->gsGlobal->PrimFogEnable,
                    dl->gsGlobal->PrimAlphaEnable,
                    dl->gsGlobal->PrimAAEnable,
                    0,
                    dl->gsGlobal->PrimContext,
                    0),
                GSKIT_GIF_FLG_REGLIST,
                5);
            dl->p_giftag[1] =
                ((u64)(GS_RGBAQ) <<  0) | \
                ((u64)(GS_XYZ2)  <<  4) | \
                ((u64)(GS_XYZ2)  <<  8) | \
                ((u64)(GS_XYZ2)  << 12) | \
                ((u64)(GS_XYZ2)  << 16);
            break;
    }

    // Align to QW
    if (dlSize(dl) & 15)
        *dl->p_data++ = 0;

    dl->ePM = EDL_PM_NONE;
}

//---------------------------------------------------------------------------
void dlColorU64(GSDL *dl, u64 color)
{
    //printf("%s\n", __FUNCTION__);

    // Set current color
    dl->current_rgbaq = color;
}

//---------------------------------------------------------------------------
void dlColor4b(GSDL *dl, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    //printf("%s\n", __FUNCTION__);

    dlColorU64(dl, GS_SETREG_RGBAQ(r, g, b, a, 0));
}

//---------------------------------------------------------------------------
void dlColor3f(GSDL *dl, float r, float g, float b)
{
    //printf("%s\n", __FUNCTION__);

    dlColor4b(dl, r*255, g*255, b*255, 0);
}

//---------------------------------------------------------------------------
void dlVertex3x(GSDL *dl, int x, int y, int z)
{
    //printf("%s\n", __FUNCTION__);

    if (dl->ePM == EDL_PM_NONE) {
        printf("%s: ERROR\n", __FUNCTION__);
        return;
    }

    // Add color
    if(dl->eCM == EDL_CM_VERT || (dl->eCM == EDL_CM_PRIM && dl->vert_count == 0)) {
        *dl->p_data++ = dl->current_rgbaq;
    }

    // Add vertex
    *dl->p_data++ = GS_SETREG_XYZ2(x + dl->gsGlobal->OffsetX, y + dl->gsGlobal->OffsetY, z);

    // Count vertices and primitives
    dl->vert_count++;
    if (dl->vert_count >= dl->vpp) {
        dl->prim_count++;
        dl->vert_count = 0;
    }
}

//---------------------------------------------------------------------------
void dlVertex3i(GSDL *dl, int x, int y, int z)
{
    //printf("%s\n", __FUNCTION__);

    dlVertex3x(dl, x*16, y*16, z*16);
}

//---------------------------------------------------------------------------
void dlVertex3f(GSDL *dl, float x, float y, float z)
{
    //printf("%s\n", __FUNCTION__);

    dlVertex3x(dl, x*16, y*16, z*16);
}
