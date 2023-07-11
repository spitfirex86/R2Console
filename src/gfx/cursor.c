#include "../framework.h"
#include "cursor.h"
#include "graphics.h"


GLI_tdstTexture g_stCursorTex = { 0 };
GLI_tdstTexture g_stCursorCxtTex = { 0 };
GLI_tdstMaterial g_stCursorMat = { 0 };
BOOL g_bIsCursorTexValid = FALSE;

MTH2D_tdstVector g_stCurA = { 0, 0 };
MTH2D_tdstVector g_stCurB = { 0, 0 };
MTH2D_tdstVector g_stCurSize = { 2.25f, 3.0f };


void CUR_fn_vLoadCursorTexture( void )
{
	GFX_fn_vLoadTexture(&g_stCursorTex, "curseur_nz.tga");
	GFX_fn_vLoadTexture(&g_stCursorCxtTex, "curseur_nz_drag.tga");
	GLI_xSetMaterialTexture(&g_stCursorMat, &g_stCursorTex);

	g_stCursorMat.ulMaterialType |= GLI_C_Mat_lTexturedElement;
	g_stCursorMat.stAmbient.xR = g_stCursorMat.stAmbient.xG = g_stCursorMat.stAmbient.xB = 1.0f;

	g_bIsCursorTexValid = TRUE;
}

void CUR_fn_vInvalidateCursor( void )
{
	if ( GAM_fn_ucGetEngineMode() == E_EM_ModeChangeLevel )
		g_bIsCursorTexValid = FALSE;
}

void CUR_fn_vDrawCursor( void )
{
	if ( g_bIsCursorTexValid )
	{
		GFX_fn_vAdd2DVector(&g_stCurB, &g_stCurA, &g_stCurSize);
		GFX_fn_vDraw2DSpriteWithZValueAndAlpha(&g_stCurA, &g_stCurB, 0xFF, 1.111f, &g_stCursorMat,
											   &GAM_g_stEngineStructure->stFixViewportAttr);
	}
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
		if ( g_stCursorMat.p_stTexture != &g_stCursorCxtTex )
			GLI_xSetMaterialTexture(&g_stCursorMat, &g_stCursorCxtTex);
	}
	else
	{
		if ( g_stCursorMat.p_stTexture != &g_stCursorTex )
			GLI_xSetMaterialTexture(&g_stCursorMat, &g_stCursorTex);
	}
}
