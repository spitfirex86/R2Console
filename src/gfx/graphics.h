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

void GFX_vDraw2DGradientWithPercent(
	GLD_tdstViewportAttributes *p_stVpt,
	MTH_tdxReal XMin,
	MTH_tdxReal YMin,
	MTH_tdxReal XMax,
	MTH_tdxReal YMax,
	unsigned int *a4_ulColors
);

void GFX_fn_vDraw2DLine( MTH3D_tdstVector *p_stFirstPoint, MTH3D_tdstVector *p_stLastPoint, unsigned long ulColor );


#define M_ulPackRGBA( R, G, B, A ) (unsigned long)(((A)&0xFF)<<24 | ((R)&0xFF)<<16 | ((G)&0xFF)<<8 | ((B)&0xFF))
#define M_ulPackRGB( R, G, B ) M_ulPackRGBA(R, G, B, 0xFF)
#define M_ulPackRGBAndAlpha( RGB, A ) (unsigned long)(((A)&0xFF)<<24 | ((RGB)&0xFFFFFF))
