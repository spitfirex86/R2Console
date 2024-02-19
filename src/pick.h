#pragma once

#include "framework.h"


/* uncomment this line to use the module */
//#define USE_PICK

#if defined(USE_PICK)


#define C_MaxPicked 10

typedef struct tdstSuperObjPickList
{
	HIE_tdstSuperObject *hSuperObj;
	MTH3D_tdstVector stCenter;
	char szName[48];
}
tdstSuperObjPickList;

typedef struct tdstPickInfo
{
	tdstSuperObjPickList a_stPicked[C_MaxPicked];
	int lNbPicked;
	MTH2D_tdstVector stClickPos;
}
tdstPickInfo;


extern tdstPickInfo g_stPickInfo;
extern int g_lPickMenuSel;


void fn_vPickObjectFromWorld( unsigned short uwMouseX, unsigned short uwMouseY, MTH2D_tdstVector *p_stMousePercent );

void fn_vDrawPickedMenu( void );
void fn_vResetPickedMenu( void );
void fn_vPickMenuHitTest( MTH2D_tdstVector *p_stPos );
void fn_vPickMenuSelect( MTH2D_tdstVector *p_stPos );


#endif
