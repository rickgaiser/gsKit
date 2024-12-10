#ifndef __GSDISPLAYLIST_H__
#define __GSDISPLAYLIST_H__


enum E_GSDL_COLOR_MODE {
    EDL_CM_NONE = 0, // Color set before transferring primitives
    EDL_CM_PRIM = 1, // Color set once per primitive (flat shading)
    EDL_CM_VERT = 2  // Color set per vertex (smooth shading)
};

enum E_GSDL_PRIM_MODE {
    EDL_PM_NONE     = 0,
    EDL_PM_SPRITE   = 2,
    EDL_PM_TRIANGLE = 3,
    EDL_PM_QUAD     = 4
};


struct gsDisplayList
{
    GSGLOBAL *gsGlobal;
    void *data __attribute__ ((aligned (64)));
    u64 *p_giftag;
    u64 *p_data;
    int qw;

    enum E_GSDL_COLOR_MODE eCM;
    enum E_GSDL_PRIM_MODE ePM;
    u64 current_rgbaq;
    int prim_count;
    int vert_count;
    int vpp; // vertices per primitive
};
typedef struct gsDisplayList GSDL;


GSDL * dlCreate(int qw);
void dlFree(GSDL *dl);

void dlBegin(GSDL *dl, enum E_GSDL_PRIM_MODE pm);
void dlEnd(GSDL *dl);

void dlColor4b(GSDL *dl, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
void dlColor3f(GSDL *dl, float r, float g, float b);

void dlVertex3x(GSDL *dl, int x, int y, int z);
void dlVertex3f(GSDL *dl, float x, float y, float z);


#endif
