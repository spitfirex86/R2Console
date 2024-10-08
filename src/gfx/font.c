﻿#include "../framework.h"
#include "font.h"
#include "graphics.h"
#include "../utils.h"


GLI_tdstTexture *g_pstFontTex = NULL;
GLI_tdstMaterial g_stFontMat = { 0 };

MTH2D_tdstVector g_stPixelSize = { 0, 0 };
MTH2D_tdstVector g_stDisplayOffset = { 0, 0 };


unsigned char g_a_a3_Colors[][3] = {
	{ 240, 240, 160 },	/* yellow */
	{ 160, 160, 240 },	/* blue */
	{ 240, 160, 160 },	/* red */
	{ 160, 240, 160 },	/* green */
	{ 240, 240, 240 },	/* white? */
};


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

	x = xX = xX * g_stPixelSize.x + g_stDisplayOffset.x;
	dx = C_Font_xCharWidth * g_stPixelSize.x;
	dx2 = C_Font_xActualCharWidth * g_stPixelSize.x;
	y = xY = xY * g_stPixelSize.y + g_stDisplayOffset.y;
	dy = C_Font_xCharHeight * g_stPixelSize.y;

	/* set default color */
	GLI_fn_vSetForcedColor(g_a_a3_Colors[0]);

	while ( (*szString) > 0 )
	{
		if ( (*szString) == '\n' )
		{
			x = xX;
			y += dy;
			szString++;
			continue;
		}
		/*if ( szString[0] == '/' && tolower(szString[1]) == 'o' && szString[3] == ':' )
		{
			lOffset = ((szString[2]-'0') % 3) * 6;
			szString += 4;
			continue;
		}*/
		if ( FNT_M_bCharIsColor(*szString) )
		{
			int lColor = FNT_M_lCharToColor(*szString) % ARRAYSIZE(g_a_a3_Colors);
			/*if ( lColor >= ARRAYSIZE(g_a_a3_Colors) )
				lColor = 0;*/
			GLI_fn_vSetForcedColor(g_a_a3_Colors[lColor]);

			szString++;
			continue;
		}

		char ch = (*szString) - ' ';
		if ( ch > 0 )
		{
			int lCharX = ch % 16;
			int lCharY = ch / 16; // + lOffset;

			/* value = (index * (dimension + margin) + (margin + antibug)) / (textureSize - 1); */
			u = ((float)lCharX * (C_Font_xCharWidth + 2.0f) + 2.15f) / (C_Font_xTexWidth-1.0f);
			v = 1.0f - ((float)lCharY * (C_Font_xCharHeight + 2.0f) + 1.15f) / (C_Font_xTexHeight-1.0f);

			GLI_fn_vDraw2DSpriteWithUV(x, x+dx, y, y+dy, u, u+C_Font_xCharU, v, v-C_Font_XCharV, &g_stFontMat);
		}

		x += dx2;
		szString++;
	}

	GLI_fn_vSetForcedColor(NULL);
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

	/* pixel size */
	
	g_stPixelSize.x = 1.0f / 640.0f * (float)p_stVpt->dwWidth;
	g_stPixelSize.y = 1.0f / 480.0f * (float)p_stVpt->dwHeight;

	if ( GLI_FIX_bIsWidescreen() )
	{
		float xRatio = GLI_FIX_xGetActualRatio() / 0.75f;

		g_stPixelSize.x *= xRatio;
		g_stDisplayOffset.x = p_stVpt->dwWidth * (1 - xRatio) / 2;
	}
}

void FNT_fn_vLoadFontTexture( void )
{
	if ( g_pstFontTex )
		TXM_fn_bUnLoadTexture(g_pstFontTex);

	g_pstFontTex = TXM_fn_hLoadTextureGF("Font10x12.tga");
	g_pstFontTex->lTextureQuality = GLI_C_TEX_QHIGH;

	GLI_fn_vInitMaterialDefaults(&g_stFontMat);
	GLI_xSetMaterialTexture(&g_stFontMat, g_pstFontTex);
}
