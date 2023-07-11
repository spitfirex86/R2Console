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

void GFX_fn_vDraw2DSpriteWithZValueAndAlpha(
	MTH2D_tdstVector *p_stTopLeft,
	MTH2D_tdstVector *p_stBottomRight,
	MTH_tdxReal xAlpha,
	MTH_tdxReal xZValue,
	GLI_tdstMaterial *hMaterial,
	GLD_tdstViewportAttributes *p_stViewport
	)
{
	MTH_tdxReal xSaveZValue = *GLI_g_fZValueForSprite;
	MTH_tdxReal xSaveAlpha = GLI_fn_xGetGlobalAlpha();
	GEO_tdstColor stSaveAmbient = hMaterial->stAmbient;

	*GLI_g_fZValueForSprite = xZValue;
	GLI_vSetGlobalAlpha(xAlpha);

	*GLI_g_bForceAAAColor = 0;
	hMaterial->stAmbient.xR = hMaterial->stAmbient.xG = hMaterial->stAmbient.xB = 0.8f;

	GLI_vDraw2DSpriteWithPercent(p_stViewport, p_stTopLeft->x, p_stTopLeft->y, p_stBottomRight->x, p_stBottomRight->y,
								 hMaterial);

	hMaterial->stAmbient = stSaveAmbient;
	*GLI_g_fZValueForSprite = xSaveZValue;
	GLI_vSetGlobalAlpha(xSaveAlpha);
}

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
	)
{
	GLI_tdstAligned2DVector a4_st2DVertex[5];
	MTH_tdxReal a4_stUVVertex[10];
	GEO_tdstColor stLocalColor = { 0 };

	if ( xXMin < (*GLI_BIG_GLOBALS)->fXMinClipping || xXMax > (*GLI_BIG_GLOBALS)->fXMaxClipping )
		return;

	stLocalColor.xG = stLocalColor.xB = stLocalColor.xR = 0.0f;
	hMaterial->stAmbient = stLocalColor;
	stLocalColor.xG = stLocalColor.xB = stLocalColor.xR = 1.0f;
	hMaterial->stDiffuse = stLocalColor;
	stLocalColor.xG = stLocalColor.xB = stLocalColor.xR = 0.0f;
	hMaterial->stSpecular = stLocalColor;
	hMaterial->lSpecularExponent = 0;

	a4_st2DVertex[0].xX = (float)(long)xXMin;
	a4_st2DVertex[0].xY = (float)(long)xYMin;
	a4_st2DVertex[0].xOoZ = 1.0f;
	a4_st2DVertex[1].xX = (float)(long)xXMax;
	a4_st2DVertex[1].xY = (float)(long)xYMin;
	a4_st2DVertex[1].xOoZ = 1.0f;
	a4_st2DVertex[2].xX = (float)(long)xXMax;
	a4_st2DVertex[2].xY = (float)(long)xYMax;
	a4_st2DVertex[2].xOoZ = 1.0f;
	a4_st2DVertex[3].xX = (float)(long)xXMin;
	a4_st2DVertex[3].xY = (float)(long)xYMax;
	a4_st2DVertex[3].xOoZ = 1.0f;

	a4_stUVVertex[0] = xUMin;
	a4_stUVVertex[0 + 1] = xVMin;
	a4_stUVVertex[2] = xUMax;
	a4_stUVVertex[2 + 1] = xVMin;
	a4_stUVVertex[4] = xUMax;
	a4_stUVVertex[4 + 1] = xVMax;
	a4_stUVVertex[6] = xUMin;
	a4_stUVVertex[6 + 1] = xVMax;

	(*GLI_BIG_GLOBALS)->hCurrentMaterial = hMaterial;
	(*GLI_BIG_GLOBALS)->hCurrentMaterial->ulMaterialType = 0xFFFFFFFF - GLI_C_Mat_lIsTestingBackface;
	(*GLI_BIG_GLOBALS)->lCurrentDrawMask = 0xFFFFFFFF - GLI_C_Mat_lIsTestingBackface;
	(*GLI_BIG_GLOBALS)->lHierachDrawMask = 0xFFFFFFFF - GLI_C_Mat_lIsTestingBackface;

	GLI_vDoMaterialSelection(*GLI_BIG_GLOBALS);

	if ( GLI_g_bForceAAAColor )
	{
		/* Keep computed alpha */
		(*GLI_BIG_GLOBALS)->ulColorInitForSprite &= 0xff000000;
		/* Force color */
		(*GLI_BIG_GLOBALS)->ulColorInitForSprite |=
			GLI_a3_ForcedAAAColor[0] << 16 | GLI_a3_ForcedAAAColor[1] << 8 | GLI_a3_ForcedAAAColor[2];
	}

	//GLI_DRV_vSendSpriteToClipWithUV ( a4_st2DVertex ,a4_stUVVertex ,1.0f / GLI_C_xZClippingNear,GLI_BIG_GLOBALS);
	(*GLI_DRV_vSendSpriteToClipWithUV)(a4_st2DVertex, a4_stUVVertex, *GLI_g_fZValueForSprite, *GLI_BIG_GLOBALS);
}


GLI_tdstTexture * GFX_fn_pstFindTexture( char const *szName )
{
	for ( int i = 0; i < 1024; i++ )
	{
		GLI_tdstTexture *pTexture = GLI_gs_aDEFTableOfTextureAlreadyRead[i];
		DWORD ulChannel = GLI_gs_aDEFTableOfTextureMemoryChannels[i];

		if ( ulChannel <= 0 || ulChannel == 0xC0DE0005 )
			continue;

		if ( !_stricmp(pTexture->szFileName, szName) )
			return pTexture;
	}

	return NULL;
}

void GFX_fn_vLoadTexture( GLI_tdstTexture *p_stTexture, char const *szGFName )
{
	// set textures folder name
	strcpy(GLI_fn_szGetPathOfTexture(), "Textures");

	// read texture format
	char szFilePath[MAX_PATH];
	sprintf(szFilePath, "Textures\\%s", szGFName);
	strcpy(strrchr(szFilePath, '.'), ".gf");

	FILE *pFile = fopen(szFilePath, "rb");
	if ( !pFile )
		return;

	FIL_tdstRealGFFileHeader stHeader = { 0 };
	fread(&stHeader, sizeof(FIL_tdstRealGFFileHeader), 1, pFile);
	fclose(pFile);

	char *pExt = strrchr(szGFName, '.');

	if ( !_stricmp(pExt, ".tga") )
		p_stTexture->ulTextureCaps = GLI_C_Tex_lTGATexture;
	else if ( !_stricmp(pExt, ".bmp") )
		p_stTexture->ulTextureCaps = GLI_C_Tex_lBMPTexture;

	if ( stHeader.ucBpp == 1 )
		p_stTexture->ulTextureCaps |= GLI_C_Tex_lPaletteTexture;
	if ( stHeader.ucBpp == 4 )
		p_stTexture->ulTextureCaps |= GLI_C_Tex_lAlphaTexture;

	p_stTexture->lTextureQuality = GLI_C_TEX_QNORMAL;

	p_stTexture->uwWidth = p_stTexture->uwRealWidth = (WORD)stHeader.ulWidth;
	p_stTexture->uwHeight = p_stTexture->uwRealHeight = (WORD)stHeader.ulHeight;

	strcpy(p_stTexture->szFileName, szGFName);

	// cycling mode (tile/mirror)
	if ( strstr(szGFName, "_txy") )
		p_stTexture->ucCyclingMode = GLI_C_lCyclingUV;
	else if ( strstr(szGFName, "_tx") )
		p_stTexture->ucCyclingMode = GLI_C_lCyclingU;
	else if ( strstr(szGFName, "_ty") )
		p_stTexture->ucCyclingMode = GLI_C_lCyclingV;
	else if ( strstr(szGFName, "_mxy") )
		p_stTexture->ucCyclingMode = GLI_C_lMirrorUV;
	else if ( strstr(szGFName, "_mx") )
		p_stTexture->ucCyclingMode = GLI_C_lMirrorU;
	else if ( strstr(szGFName, "_my") )
		p_stTexture->ucCyclingMode = GLI_C_lMirrorV;

	if ( p_stTexture->ucCyclingMode & GLI_C_lMirrorU )
	{
		p_stTexture->uwWidth *= 2;
		p_stTexture->uwRealWidth *= 2;
	}

	if ( p_stTexture->ucCyclingMode & GLI_C_lMirrorV )
	{
		p_stTexture->uwHeight *= 2;
		p_stTexture->uwRealHeight *= 2;
	}

	// other flags
	if ( strstr(szGFName, "_ad") )
		p_stTexture->ulTextureCaps |= GLI_C_Tex_lAddTransparencyTexture;

	if ( strstr(szGFName, "_aaa") )
		p_stTexture->ulTextureCaps |= GLI_C_Tex_lAAATexture;

	if ( strstr(szGFName, "_nz") )
		p_stTexture->ulTextureCaps |= GLI_C_Tex_lNZTexture + GLI_C_Tex_lNZFilteredTexture;

	// add to texture list
	for ( int i = 0; i < 1024; i++ )
	{
		if ( GLI_gs_aDEFTableOfTextureMemoryChannels[i] == GLI_C_TEXIsUnallocated )
		{
			GLI_gs_aDEFTableOfTextureAlreadyRead[i] = p_stTexture;
			GLI_gs_aDEFTableOfTextureMemoryChannels[i] = 2; // not sure but it works
			break;
		}
	}

	(*GLI_g_ulNumberOfLoadedTexture)++;
}

GLI_tdstMaterial * GFX_fn_hCreateMaterialWithTexture( GLI_tdstTexture *p_stTexture )
{
	GLI_tdstMaterial *hMaterial = NULL;

	GLI_xCreateMaterial(&hMaterial);
	GLI_xSetMaterialTexture(hMaterial, p_stTexture);

	return hMaterial;
}

void GFX_fn_vSetForcedColor( ACP_tdxBool bForce, unsigned char const *a3_Color )
{
	if ( bForce )
	{
		*GLI_g_bForceAAAColor = 1;
		GLI_a3_ForcedAAAColor[0] = a3_Color[0];
		GLI_a3_ForcedAAAColor[1] = a3_Color[1];
		GLI_a3_ForcedAAAColor[2] = a3_Color[2];
	}
	else
	{
		*GLI_g_bForceAAAColor = 0;
	}
}
