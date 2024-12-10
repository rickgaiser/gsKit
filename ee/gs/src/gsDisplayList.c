#include "gsKit.h"

#include <malloc.h>


//---------------------------------------------------------------------------
GSDL * dlCreate(GSGLOBAL *gsGlobal, int qw)
{
    GSDL *dl = calloc(1, sizeof(GSDL));
    dl->gsGlobal = gsGlobal;
    dl->data = gsKit_alloc_ucab(qw * 16);
    dl->qw = qw;
    dl->eCM = EDL_CM_PRIM;
    dl->ePM = EDL_PM_NONE;
    dl->current_rgbaq = GS_SETREG_RGBAQ(0, 0, 0, 0, 0);
    dl->prim_count = 0;
    dl->vert_count = 0;
}

//---------------------------------------------------------------------------
void dlFree(GSDL *dl)
{
    free(dl->data);
}

//---------------------------------------------------------------------------
void dlBegin(GSDL *dl, enum E_GSDL_PRIM_MODE pm)
{
    u64* p_data;

    if (dl->ePM != EDL_PM_NONE) {
        printf("%s: ERROR\n", __FUNCTION__);
        return;
    }

    dl->ePM = pm;
    dl->prim_count = 0;
    dl->vert_count = 0;
    dl->p_giftag = dl->p_data++;

    switch (dl->ePM) {
        case EDL_PM_SPRITE:
            dl->vpp = 2;
            *dl->p_data++ = GIF_TAG_SPRITE_REGS;
            *dl->p_data++ = GS_SETREG_PRIM(GS_PRIM_PRIM_SPRITE, 0, 0, dl->gsGlobal->PrimFogEnable,
                        dl->gsGlobal->PrimAlphaEnable, dl->gsGlobal->PrimAAEnable,
                        0, dl->gsGlobal->PrimContext, 0);
            break;
        case EDL_PM_TRIANGLE:
            dl->vpp = 3;
            *dl->p_data++ = GIF_TAG_TRIANGLE_REGS;
            *dl->p_data++ = GS_SETREG_PRIM(GS_PRIM_PRIM_TRIANGLE, 0, 0, dl->gsGlobal->PrimFogEnable,
                        dl->gsGlobal->PrimAlphaEnable, dl->gsGlobal->PrimAAEnable,
                        0, dl->gsGlobal->PrimContext, 0);
            break;
        case EDL_PM_QUAD:
            dl->vpp = 4;
            *dl->p_data++ = GIF_TAG_QUAD_REGS;
            *dl->p_data++ = GS_SETREG_PRIM(GS_PRIM_PRIM_TRISTRIP, 0, 0, dl->gsGlobal->PrimFogEnable,
                        dl->gsGlobal->PrimAlphaEnable, dl->gsGlobal->PrimAAEnable,
                        0, dl->gsGlobal->PrimContext, 0);
            break;
    }
}

//---------------------------------------------------------------------------
void dlEnd(GSDL *dl)
{
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
            *dl->p_giftag = GIF_TAG_SPRITE(dl->prim_count);
            break;
        case EDL_PM_TRIANGLE:
            *dl->p_giftag = GIF_TAG_TRIANGLE(dl->prim_count);
            break;
        case EDL_PM_QUAD:
            *dl->p_giftag = GIF_TAG_QUAD(dl->prim_count);
            break;
    }

    dl->ePM = EDL_PM_NONE;
}

//---------------------------------------------------------------------------
void dlColor4b(GSDL *dl, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    // Set current color
    dl->current_rgbaq = GS_SETREG_RGBAQ(r, g, b, a, 0);
}

//---------------------------------------------------------------------------
void dlColor3f(GSDL *dl, float r, float g, float b)
{
    dlColor4b(dl, r*255, g*255, b*255, 0);
}

//---------------------------------------------------------------------------
void dlVertex3x(GSDL *dl, int x, int y, int z)
{
    if (dl->ePM == EDL_PM_NONE) {
        printf("%s: ERROR\n", __FUNCTION__);
        return;
    }

    // Add color
    if(dl->eCM == EDL_CM_VERT || (dl->eCM == EDL_CM_PRIM && dl->vert_count == 0)) {
        *dl->p_data++ = dl->current_rgbaq;
    }

    // Add vertex
    *dl->p_data++ = GS_SETREG_XYZ2(x, y, z);

    // Count vertices and primitives
    dl->vert_count++;
    if (dl->vert_count >= dl->vpp) {
        dl->prim_count++;
        dl->vert_count = 0;
    }
}

//---------------------------------------------------------------------------
void dlVertex3f(GSDL *dl, float x, float y, float z)
{
    dlVertex3i(dl, x*16, y*16, z*16);
}
