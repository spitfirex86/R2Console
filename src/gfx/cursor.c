#include "../framework.h"
#include "cursor.h"
#include "graphics.h"
#include "../utils.h"



GLI_tdstTexture *g_pstCursorTex = NULL;
GLI_tdstTexture *g_pstCursorCxtTex = NULL;
GLI_tdstMaterial g_stCursorMat = { 0 };
//BOOL g_bIsCursorTexValid = FALSE;

MTH2D_tdstVector g_stCurA = { 0, 0 };
MTH2D_tdstVector g_stCurB = { 0, 0 };
MTH2D_tdstVector g_stCurSize = { 2.25f, 3.0f };


void CUR_fn_vLoadCursorTexture( void )
{
	if ( g_pstCursorTex )
		TXM_fn_bUnLoadTexture(g_pstCursorTex);
	if ( g_pstCursorCxtTex )
		TXM_fn_bUnLoadTexture(g_pstCursorCxtTex);

	g_pstCursorTex = TXM_fn_hLoadTextureGF("curseur_nz.tga");
	g_pstCursorCxtTex = TXM_fn_hLoadTextureGF("curseur_nz_drag.tga");

	GLI_fn_vInitMaterialDefaults(&g_stCursorMat);
	GLI_xSetMaterialTexture(&g_stCursorMat, g_pstCursorTex);
	g_stCursorMat.stAmbient.xR = g_stCursorMat.stAmbient.xG = g_stCursorMat.stAmbient.xB = 0.8f;
}

void CUR_fn_vDrawCursor( void )
{
	GFX_fn_vAdd2DVector(&g_stCurB, &g_stCurA, &g_stCurSize);
	GLI_fn_vSetForcedColor(NULL);
	GLI_fn_vDraw2DSpriteWithZValueAndAlpha(&g_stCurA, &g_stCurB, 0xFF, 1.111f, &g_stCursorMat);
}

void CUR_fn_vMoveCursor( MTH2D_tdstVector *p_stPos )
{
	g_stCurA = *p_stPos;
	g_stCurA.x -= g_stCurSize.x;
}

void CUR_fn_vSetCursorCxt( BOOL bCxt )
{
	if ( bCxt )
	{
		if ( g_stCursorMat.p_stTexture != g_pstCursorCxtTex )
			GLI_xSetMaterialTexture(&g_stCursorMat, g_pstCursorCxtTex);
	}
	else
	{
		if ( g_stCursorMat.p_stTexture != g_pstCursorTex )
			GLI_xSetMaterialTexture(&g_stCursorMat, g_pstCursorTex);
	}
}
