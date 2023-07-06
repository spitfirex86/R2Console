#pragma once

#include "../framework.h"


void GFX_fn_vAdd2DVector(
	MTH2D_tdstVector *p_stResult,
	MTH2D_tdstVector const *p_stFirst,
	MTH2D_tdstVector const *p_stSecond
);

void GFX_fn_vLerp2DVector(
	MTH2D_tdstVector *p_stResult,
	MTH2D_tdstVector const *p_stStart,
	MTH2D_tdstVector const *p_stEnd,
	MTH_tdxReal const xPercent
);

void GFX_fn_vDisplayFrameWithZValue(
	MTH2D_tdstVector *p_stTopLeft,
	MTH2D_tdstVector *p_stBottomRight,
	MTH_tdxReal xAlpha,
	MTH_tdxReal xZValue,
	GLD_tdstViewportAttributes *p_stViewport
);

void GFX_fn_vDraw2DSpriteWithZValueAndAlpha(
	MTH2D_tdstVector *p_stTopLeft,
	MTH2D_tdstVector *p_stBottomRight,
	MTH_tdxReal xAlpha,
	MTH_tdxReal xZValue,
	GLI_tdstMaterial *hMaterial,
	GLD_tdstViewportAttributes *p_stViewport
);

void GFX_fn_vDraw2DSpriteWithUV(
	GLD_tdstViewportAttributes *p_stVpt,
	MTH_tdxReal xXMin,
	MTH_tdxReal xXMax,
	MTH_tdxReal xYMin,
	MTH_tdxReal xYMax,
	MTH_tdxReal xUMin,
	MTH_tdxReal xUMax,
	MTH_tdxReal xVMin,
	MTH_tdxReal xVMax,
	GLI_tdstMaterial *hMaterial
);

GLI_tdstTexture * GFX_fn_pstFindTexture( char const *szName );
GLI_tdstMaterial * GFX_fn_hCreateMaterialWithTexture( GLI_tdstTexture *p_stTexture );
void GFX_fn_vLoadTexture( GLI_tdstTexture *p_stTexture, char const *szGFName );

void GFX_fn_vSetForcedColor( ACP_tdxBool bForce, unsigned char const *a3_Color );
