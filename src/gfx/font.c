#include "../framework.h"
#include "font.h"
#include "graphics.h"


GLI_tdstTexture g_stFontTex = { 0 };
GLI_tdstMaterial g_stFontMat = { 0 };

MTH2D_tdstVector g_stPixelSize = { 0, 0 };


long fn_lGetFmtStringLength( char const *szFmt, va_list args )
{
	long lSize = vsnprintf(NULL, 0, szFmt, args);
	return lSize + 1;
}

void FNT_fn_vDisplayString( MTH_tdxReal xX, MTH_tdxReal xY, char *szString )
{
	MTH_tdxReal x, dx, dx2, y, dy;
	MTH_tdxReal u, v;
	int lOffset = 0;

	GLD_tdstViewportAttributes *p_stVpt = &GAM_g_stEngineStructure->stFixViewportAttr;

	x = xX = xX * g_stPixelSize.x;
	dx = C_Font_xCharWidth * g_stPixelSize.x;
	dx2 = C_Font_xActualCharWidth * g_stPixelSize.x;
	y = xY = xY * g_stPixelSize.y;
	dy = C_Font_xCharHeight * g_stPixelSize.y;

	while ( (*szString) > 0 )
	{
		if ( (*szString) == '\n' )
		{
			x = xX;
			y += dy;
			szString++;
			continue;
		}
		if ( szString[0] == '/' && tolower(szString[1]) == 'o' && szString[3] == ':' )
		{
			lOffset = ((szString[2]-'0') % 3) * 6;
			szString += 4;
			continue;
		}

		char ch = (*szString) - ' ';

		if ( ch > 0 )
		{
			int lCharX = ch % 16;
			int lCharY = ch / 16 + lOffset;

			/* value = (index * (dimension + margin) + (margin + antibug)) / (textureSize - 1); */
			u = ((float)lCharX * (C_Font_xCharWidth + 2.0f) + 2.15f) / 255.0f;
			v = 1.0f - ((float)lCharY * (C_Font_xCharHeight + 2.0f) + 1.15f) / 255.0f;

			GFX_fn_vDraw2DSpriteWithUV(p_stVpt, x, x+dx, y, y+dy, u, u+0.039f, v, v-0.0468f, &g_stFontMat);
		}

		x += dx2;
		szString++;
	}
}

void FNT_fn_vDisplayStringFmt( MTH_tdxReal xX, MTH_tdxReal xY, char const *szFmt, ... )
{
	va_list args;
	va_start(args, szFmt);

	long lSize = fn_lGetFmtStringLength(szFmt, args);
	char *szBuffer = _alloca(lSize);

	if ( szBuffer )
	{
		vsprintf(szBuffer, szFmt, args);
		FNT_fn_vDisplayString(xX, xY, szBuffer);
	}

	va_end(args);
}

void FNT_fn_vFontInit( void )
{
	GLD_tdstViewportAttributes *p_stVpt = &GAM_g_stEngineStructure->stFixViewportAttr;

	g_stPixelSize.x = 1.0f / 640.0f * (float)p_stVpt->dwWidth;
	g_stPixelSize.y = 1.0f / 480.0f * (float)p_stVpt->dwHeight;
}

void FNT_fn_vLoadFontTexture( void )
{
	GFX_fn_vLoadTexture(&g_stFontTex, "Font10x12.tga");
	g_stFontTex.lTextureQuality = GLI_C_TEX_QHIGH;

	GLI_xSetMaterialTexture(&g_stFontMat, &g_stFontTex);
	g_stFontMat.ulMaterialType |= GLI_C_Mat_lTexturedElement;
	g_stFontMat.stAmbient.xR = g_stFontMat.stAmbient.xG = g_stFontMat.stAmbient.xB = 1.0f;
}
