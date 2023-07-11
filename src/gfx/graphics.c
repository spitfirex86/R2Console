#include "../framework.h"
#include "graphics.h"


#define M_xLerp(a,b,t) ((a)+((t)*((b)-(a))))

void GFX_fn_vAdd2DVector(
	MTH2D_tdstVector *p_stResult,
	MTH2D_tdstVector const *p_stFirst,
	MTH2D_tdstVector const *p_stSecond
	)
{
	p_stResult->x = p_stFirst->x + p_stSecond->x;
	p_stResult->y = p_stFirst->y + p_stSecond->y;
}

void GFX_fn_vLerp2DVector(
	MTH2D_tdstVector *p_stResult,
	MTH2D_tdstVector const *p_stStart,
	MTH2D_tdstVector const *p_stEnd,
	MTH_tdxReal const xPercent
	)
{
	p_stResult->x = M_xLerp(p_stStart->x, p_stEnd->x, xPercent);
	p_stResult->y = M_xLerp(p_stStart->y, p_stEnd->y, xPercent);
}

void GFX_fn_vDisplayFrameWithZValue(
	MTH2D_tdstVector *p_stTopLeft,
	MTH2D_tdstVector *p_stBottomRight,
	MTH_tdxReal xAlpha,
	MTH_tdxReal xZValue,
	GLD_tdstViewportAttributes *p_stViewport
	)
{
	MTH_tdxReal xSaveZValue = *GLI_g_fZValueForSprite;
	*GLI_g_fZValueForSprite = xZValue;

	*GLI_g_bForceAAAColor = 0;
	GLI_vDisplayFrame(p_stTopLeft, p_stBottomRight, xAlpha, p_stViewport);

	*GLI_g_fZValueForSprite = xSaveZValue;
}

