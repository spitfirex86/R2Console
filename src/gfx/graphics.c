#include "../framework.h"
#include "graphics.h"
#include "../utils.h"


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

GLI_tdstMaterial **GLI_gs_hDefaultMaterial = OFFSET(0x5036B0);

void (**GLI_DRV_vSendSpriteToClipWithColors_)( GLI_tdstAligned2DVector *a4_st2DVertex, void *_Colors, float xZ, GLI_tdstInternalGlobalValuesFor3dEngine *p_stGlobaleMT ) = OFFSET(0x504838);


/* The triangles are drawn as such:
	1---0
	|A /|	A -> 0 1 2
	| / |	B -> 3 2 0
	|/ B|
	2---3
*/
void GFX_vDraw2DGradientWithPercent( GLD_tdstViewportAttributes *p_stVpt, MTH_tdxReal XMin, MTH_tdxReal YMin, MTH_tdxReal XMax, MTH_tdxReal YMax, unsigned int *a4_ulColors )
{
	MTH_tdxReal x1, x2, y1, y2;
	GLI_tdstAligned2DVector a4_st2DVertex[4];
	
	GLI_tdstInternalGlobalValuesFor3dEngine *p_stGlobals = *GLI_BIG_GLOBALS;
	GLI_tdstMaterial *hMaterial = *GLI_gs_hDefaultMaterial;

	MTH_tdxReal xVptWidth = (MTH_tdxReal)p_stVpt->dwWidth;
	MTH_tdxReal xVptHeight = (MTH_tdxReal)p_stVpt->dwHeight;

	x1 = (XMin / 100.0f) * xVptWidth;
	x2 = (XMax / 100.0f) * xVptWidth;
	y1 = (YMin / 100.0f) * xVptHeight;
	y2 = (YMax / 100.0f) * xVptHeight;

	if ( GLI_FIX_bIsWidescreen() )
	{
		float ratio = 1.0f;
		float addToCenter = 0.0f;

		if ( XMin != 0.0f || XMax != 100.0f )
		{
			ratio = g_xScreenRatio / 0.75f;
			addToCenter = (xVptWidth - (xVptWidth * ratio)) / 2;
		}

		x1 = x1 * ratio + addToCenter;
		x2 = x2 * ratio + addToCenter;
	}

	a4_st2DVertex[0].xX = x2;
	a4_st2DVertex[0].xY = y1;
	a4_st2DVertex[0].xOoZ = 1.0f;
	a4_st2DVertex[1].xX = x1;
	a4_st2DVertex[1].xY = y1;
	a4_st2DVertex[1].xOoZ = 1.0f;
	a4_st2DVertex[2].xX = x1;
	a4_st2DVertex[2].xY = y2;
	a4_st2DVertex[2].xOoZ = 1.0f;
	a4_st2DVertex[3].xX = x2;
	a4_st2DVertex[3].xY = y2;
	a4_st2DVertex[3].xOoZ = 1.0f;

	MTH_tdxReal xSaveAlpha = GLI_fn_xGetGlobalAlpha();
	GLI_vSetGlobalAlpha(0.f);

	hMaterial->stAmbient.xR = hMaterial->stAmbient.xG = hMaterial->stAmbient.xB = 0;
	p_stGlobals->hCurrentMaterial = hMaterial;
	p_stGlobals->lCurrentDrawMask = p_stGlobals->lHierachDrawMask =
		p_stGlobals->hCurrentMaterial->ulMaterialType = 0xFFFFFFFF - GLI_C_Mat_lIsTestingBackface - GLI_C_Mat_lIsNotGrided;

	GLI_vDoMaterialSelection(p_stGlobals);
	p_stGlobals->xTextureDontAcceptFog = 1;

	(*GLI_DRV_vSendSpriteToClipWithColors_)(a4_st2DVertex, a4_ulColors, *GLI_g_fZValueForSprite, p_stGlobals);

	GLI_vSetGlobalAlpha(xSaveAlpha);
}

void GFX_fn_vDraw2DLine( MTH3D_tdstVector *p_stFirstPoint, MTH3D_tdstVector *p_stLastPoint, unsigned long ulColor )
{
	GLI_tdstInternalGlobalValuesFor3dEngine *pstGlobals = *GLI_BIG_GLOBALS;
	GLD_tdstViewportAttributes *p_stVpt = &GAM_g_stEngineStructure->stFixViewportAttr;

	GEO_tdstColor stColor;
	GLI_tdstAligned3DVector a2_stVertex[2];
	GLI_tdstAligned2DVector a2_st2DVertex[2];

	pstGlobals->hCurrentMaterial = NULL;
	pstGlobals->lCurrentDrawMask = 0xFFFFFFFF;
	pstGlobals->lHierachDrawMask = 0xFFFFFFFF;
	GLI_vDoMaterialSelection(pstGlobals);

	a2_st2DVertex[0].ulPackedColor = a2_st2DVertex[1].ulPackedColor = ulColor;

	stColor.xR = (float)((ulColor & 0x00FF0000) >> 16) * 1.0f;
	stColor.xG = (float)((ulColor & 0x0000FF00) >> 8) * 1.0f;
	stColor.xB = (float)((ulColor & 0x000000FF) >> 0) * 1.0f;
	stColor.xA = 255.f;

	a2_st2DVertex[0].xX = p_stFirstPoint->x * p_stVpt->dwWidth;
	a2_st2DVertex[0].xY = p_stFirstPoint->y * p_stVpt->dwHeight;
	a2_stVertex[0].xZ = a2_st2DVertex[0].xOoZ = 10000.0f;
	a2_st2DVertex[1].xX = p_stLastPoint->x * p_stVpt->dwWidth;
	a2_st2DVertex[1].xY = p_stLastPoint->y * p_stVpt->dwHeight;
	a2_stVertex[1].xZ = a2_st2DVertex[1].xOoZ = 10000.0f;

	(*GLI_DRV_vSendSingleLineToClip_)(
		p_stVpt,
		&a2_stVertex[0],
		&a2_st2DVertex[0],
		&a2_stVertex[1],
		&a2_st2DVertex[1],
		pstGlobals,
		GLI_C_Mat_lGouraudLineElement,
		&stColor
		);
}


#if 0
/****************************************************************************
* GEO SPHERE
****************************************************************************/

GEO_tdstGeometricObject *p_stObjectSphereReference = NULL;
void fn_vCreateReferenceSphere( void )
{
	MTH_tdxReal					xCosAlpha, xSinAlpha, xAlphaLatitude, xAlphaLongitude;
	ACP_tdxIndex				xCounter1, xCounter2, xTriangleToCreate, xCounter2P1;
	ACP_tdxIndex				hElement;
	MTH3D_tdstVector			aDEF_stProfile[GEO_C_lNbLatitudes];
	MTH3D_tdstVector			xRotatRef;
	MTH_tdxReal					xLocalSave;
	GEO_tdstColor				stColor;
	GLI_tdstMaterial			*hMaterial;
	GMT_tdstGameMaterial		*hGameMaterial;
	GLI_tdst2DUVValues			stUV;

	xTriangleToCreate = 0;
	stUV.xU = stUV.xV = 0.0f;

	GEO_vCreateGeometricObject(&p_stObjectSphereReference, GEO_C_lNbPointsInSphere, 1);

	xAlphaLatitude = MTH_C_Pi / (GEO_C_lNbLatitudes - 1);
	xCosAlpha = cosf(xAlphaLatitude);
	xSinAlpha = sinf(xAlphaLatitude);

	xRotatRef = (MTH3D_tdstVector){ 0, 0, -1 };
	GEO_vSetPointOfObject(p_stObjectSphereReference, &xRotatRef, 1);
	xRotatRef.z = 1;
	GEO_vSetPointOfObject(p_stObjectSphereReference, &xRotatRef, 0);

	for ( xCounter1 = 0; xCounter1 < GEO_C_lNbLatitudes - 2; xCounter1++ )
	{
		xLocalSave = xRotatRef.y;
		xRotatRef.y = xLocalSave * xCosAlpha + xRotatRef.z * xSinAlpha;
		xRotatRef.z = -xLocalSave * xSinAlpha + xRotatRef.z * xCosAlpha;
		aDEF_stProfile[xCounter1] = xRotatRef;
		GEO_vSetPointOfObject(p_stObjectSphereReference, &xRotatRef, (ACP_tdxIndex)(xCounter1 + 2));
	}

	xAlphaLongitude = MTH_C_2Pi / (GEO_C_lNbLongitudes);
	xCosAlpha = cosf(xAlphaLongitude);
	xSinAlpha = sinf(xAlphaLongitude);

	for ( xCounter2 = 0; xCounter2 < GEO_C_lNbLongitudes - 1; xCounter2++ )
	{
		for ( xCounter1 = 0; xCounter1 < GEO_C_lNbLatitudes - 2; xCounter1++ )
		{
			xRotatRef = aDEF_stProfile[xCounter1];
			xLocalSave = xRotatRef.y;
			xRotatRef.y = xLocalSave * xCosAlpha + xRotatRef.x * xSinAlpha;
			xRotatRef.x = -xLocalSave * xSinAlpha + xRotatRef.x * xCosAlpha;
			aDEF_stProfile[xCounter1] = xRotatRef;
			GEO_vSetPointOfObject(p_stObjectSphereReference, &xRotatRef, (ACP_tdxIndex)(xCounter1 + 2 + ((xCounter2 + 1) * (GEO_C_lNbLatitudes - 2))));
		}
	}

	GEO_vCreateElementIndexedTriangles(p_stObjectSphereReference, &hElement, GEO_C_lNbTrianglesInSphere, 1);
	GEO_vSetUVOfIndexedTriangles(p_stObjectSphereReference, hElement, 0, &stUV);
	GLI_xCreateMaterial(&hMaterial);
	hGameMaterial = GMT_fn_hCreateGameMaterial();
	GMT_fn_vSetVisualMaterial(hGameMaterial, hMaterial);
	hMaterial->ulMaterialType = GLI_C_Mat_lGouraudElement - GLI_C_Mat_lIsNotGrided - GLI_C_Mat_lIsNotOutlined;
	stColor.xA = 1.0f;
	stColor.xR = stColor.xG = stColor.xB = 1.0f;
	hMaterial->stDiffuse = stColor;
	hMaterial->stColor = stColor;
	stColor.xR = stColor.xG = stColor.xB = 0.0f;
	hMaterial->stAmbient = stColor;
	GEO_vSetGameMaterialOfIndexedTriangles(p_stObjectSphereReference, hElement, hGameMaterial);

	for ( xCounter2 = 0; xCounter2 < GEO_C_lNbTrianglesInSphere; xCounter2++ )
	{
		GEO_vSetIndexedUVOfFaceOfIndexedTriangles(p_stObjectSphereReference, hElement, xCounter2, 0, 0, 0);
		GEO_vSetFaceOfIndexedTriangles(p_stObjectSphereReference, hElement, xCounter2, 0, 0, 0);
	}

	for ( xCounter2 = 0; xCounter2 < GEO_C_lNbLongitudes; xCounter2++ )
	{
		xCounter2P1 = (xCounter2 + 1) % (GEO_C_lNbLongitudes);
		GEO_vSetFaceOfIndexedTriangles(p_stObjectSphereReference, hElement, xTriangleToCreate++, 0, (ACP_tdxIndex)(xCounter2 * (GEO_C_lNbLatitudes - 2) + 2), (ACP_tdxIndex)(xCounter2P1 * (GEO_C_lNbLatitudes - 2) + 2));

		for ( xCounter1 = 0; xCounter1 < (GEO_C_lNbLatitudes - 3); xCounter1++ )
		{
			GEO_vSetFaceOfIndexedTriangles
			(
				p_stObjectSphereReference,
				hElement,
				xTriangleToCreate++,
				(ACP_tdxIndex)(xCounter2 * (GEO_C_lNbLatitudes - 2) + 2 + xCounter1 + 1),
				(ACP_tdxIndex)(xCounter2P1 * (GEO_C_lNbLatitudes - 2) + 2 + xCounter1),
				(ACP_tdxIndex)(xCounter2 * (GEO_C_lNbLatitudes - 2) + 2 + xCounter1)
			);
			GEO_vSetFaceOfIndexedTriangles
			(
				p_stObjectSphereReference,
				hElement,
				xTriangleToCreate++,
				(ACP_tdxIndex)(xCounter2P1 * (GEO_C_lNbLatitudes - 2) + 2 + xCounter1),
				(ACP_tdxIndex)(xCounter2 * (GEO_C_lNbLatitudes - 2) + 2 + xCounter1 + 1),
				(ACP_tdxIndex)(xCounter2P1 * (GEO_C_lNbLatitudes - 2) + 2 + xCounter1 + 1)
			);

		}

		GEO_vSetFaceOfIndexedTriangles
		(
			p_stObjectSphereReference,
			hElement,
			xTriangleToCreate++,
			1,
			(ACP_tdxIndex)(xCounter2P1 * (GEO_C_lNbLatitudes - 2) + GEO_C_lNbLatitudes - 1),
			(ACP_tdxIndex)(xCounter2 * (GEO_C_lNbLatitudes - 2) + GEO_C_lNbLatitudes - 1)
		);
	}

	GEO_vEndModifyObject(p_stObjectSphereReference);
	GLI_xSetMaterialTexture(hMaterial, NULL);
}

void fn_vSendElementSphere( GLI_tdstInternalGlobalValuesFor3dEngine *p_stGlobals, ACP_tdxIndex xElement )
{
	GEO_tdstGeometricObject *p_stObj = p_stGlobals->p_stObj;
	GEO_tdstElementSpheres *p_stES = (GEO_tdstElementSpheres *)p_stObj->d_hListOfElements[xElement];

	if ( (p_stGlobals->lHierachDrawMask & GLI_C_Mat_lIsNotDrawCollideInformation) == 0 )
	{
		for ( ACP_tdxIndex xSphere = 0; xSphere < p_stES->xNbSpheres; xSphere++ )
		{
			GEO_tdstIndexedSphere *p_stSphere = &p_stES->d_stListOfSpheres[xSphere];
			MTH_tdxReal xRadius = p_stSphere->xRadius;
			MTH3D_tdstVector *p_stCenter = &p_stObj->d_stListOfPoints[p_stSphere->xCenterPoint];

			MTH3D_tdstVector a_stSpherePoints[GEO_C_lNbPointsInSphere];

			for ( ACP_tdxIndex i = 0; i < GEO_C_lNbPointsInSphere; i++ )
			{
				MTH3D_M_vMulVectorScalar(a_stSpherePoints + i, p_stObjectSphereReference->d_stListOfPoints + i, xRadius);
				MTH3D_M_vAddVector(a_stSpherePoints + i, a_stSpherePoints + i, p_stCenter);
			}

			p_stObjectSphereReference->xBoundingSphereRadius = xRadius;
			p_stObjectSphereReference->xBoundingSphereCenter = *p_stCenter;

			MTH3D_tdstVector *p_stSaveList = p_stObjectSphereReference->d_stListOfPoints;
			p_stObjectSphereReference->d_stListOfPoints = a_stSpherePoints;
			GLI_xSendObjectToViewportWithLights(p_stGlobals->p_stVpt, p_stObjectSphereReference, p_stGlobals->lHierachDrawMask);
			p_stObjectSphereReference->d_stListOfPoints = p_stSaveList;
		}
		return;
	}
}

void fn_vSendObjectElement( GLI_tdstInternalGlobalValuesFor3dEngine *p_stGlobals )
{
	GEO_tdstGeometricObject *p_stObj = p_stGlobals->p_stObj;
	for ( ACP_tdxIndex i = 0; i < p_stObj->xNbElements; i++ )
	{
		if ( p_stObj->d_xListOfElementsTypes[i] == GEO_C_xElementSpheres )
			fn_vSendElementSphere(p_stGlobals, i);
	}

	GLI_xSendObjectElement(p_stGlobals);
}

void fn_vDrawSphereTest( HIE_tdstSuperObject *hActor )
{
	static GMT_tdstGameMaterial stGameMat = { 0 };
	static GLI_tdstMaterial stMat = { 0 };

	static GEO_tdstGeometricObject *hObj = NULL;
	static ACP_tdxIndex hElem = 0;

	if ( !HIE_M_bSuperObjectIsActor(hActor) || (hActor->ulFlags & HIE_C_Flag_TypeOfBoundingVolume) )
		return;

	GEO_tdstBoundingSphere *hBdSphere = (GEO_tdstBoundingSphere *)hActor->pBoundingVolume;
	MTH3D_tdstVector stCenter = hBdSphere->stCenterPoint;
	MTH3D_M_vAddVector(&stCenter, &hBdSphere->stCenterPoint, &hActor->p_stGlobalMatrix->stPos);
	MTH_tdxReal xRadius = hBdSphere->xRadius;

	if ( !hObj )
	{
		fn_vCreateReferenceSphere();

		GEO_vCreateGeometricObject(&hObj, 1, 1);
		GEO_vSetPointOfObject(hObj, &stCenter, 0);

		GEO_vCreateElementSpheres(hObj, &hElem, 1);
		GEO_vSetCenterPointOfIndexedSphere(hObj, hElem, 0, 0);
		GEO_vSetRadiusOfIndexedSphere(hObj, hElem, 0, xRadius);

		/*
		GMT_fn_vSetVisualMaterial(&stGameMat, &stMat);

		GLI_fn_vInitMaterialDefaults(&stMat);
		stMat.stColor.xR = stMat.stColor.xG = stMat.stColor.xB = 0.8f;
		stMat.stColor.xA = 0.0f;

		GEO_vSetGameMaterialOfIndexedSphere(hObj, hElem, 0, &stGameMat);
		GEO_vSetMaterialOfIndexedSphere(hObj, hElem, 0, &stMat);
		*/
	}

	GEO_vSetPointOfObject(hObj, &stCenter, 0);

	GLI_xSendObjectToViewportWithLights(&GAM_g_stEngineStructure->stViewportAttr, hObj, GLI_C_lAllIsEnable-GLI_C_Mat_lIsNotDrawCollideInformation);
}

void fn_vDrawTooFarLimitTest( HIE_tdstSuperObject *hActor )
{
	static GEO_tdstGeometricObject *hOctahedron = NULL;
	static ACP_tdxIndex hElem = 0;

	if ( !HIE_M_bSuperObjectIsActor(hActor) || !HIE_M_hSuperObjectGetStdGame(hActor) )
		return;

	if ( !hOctahedron )
	{
		GEO_vCreateGeometricObject(&hOctahedron, 6, 1);
		GEO_vCreateElementIndexedTriangles(hOctahedron, &hElem, 8, 1);

		GLI_tdst2DUVValues stUV = { 0, 0 };
		GEO_vSetUVOfIndexedTriangles(hOctahedron, hElem, 0, &stUV);

		GLI_tdstMaterial *hMaterial;
		GLI_xCreateMaterial(&hMaterial);

		GMT_tdstGameMaterial *hGameMaterial = GMT_fn_hCreateGameMaterial();
		GMT_fn_vSetVisualMaterial(hGameMaterial, hMaterial);
		hMaterial->ulMaterialType = GLI_C_Mat_lGouraudElement - GLI_C_Mat_lIsNotGrided - GLI_C_Mat_lIsNotOutlined;

		GEO_tdstColor stColor = { 1.0f, 0.0f, 0.0f, 1.0f };
		hMaterial->stDiffuse = stColor;
		hMaterial->stColor = stColor;
		hMaterial->stAmbient = stColor;

		GLI_xSetMaterialTexture(hMaterial, NULL);
		GEO_vSetGameMaterialOfIndexedTriangles(hOctahedron, hElem, hGameMaterial);

		MTH3D_tdstVector a_stPoints[6] = {
			{  1,  0,  0 },
			{  0,  1,  0 },
			{  0,  0,  1 },
			{ -1,  0,  0 },
			{  0, -1,  0 },
			{  0,  0, -1 }
		};
		ACP_tdxIndex aa_xFaces[8][3] = {
			{ 0, 1, 2 }, /* +x, +y, +z */
			{ 1, 3, 2 }, /* -x, +y, +z */
			{ 3, 4, 2 }, /* -x, -y, +z */
			{ 4, 3, 5 }, /* -x, -y, -z */
			{ 4, 5, 0 }, /* +x, -y, -z */
			{ 1, 0, 5 }, /* +x, +y, -z */
			{ 4, 0, 2 }, /* +x, -y, +z */ 
			{ 3, 1, 5 }, /* -x, +y, -z */
		};

		for ( ACP_tdxIndex i = 0; i < 6; i++ )
			GEO_vSetPointOfObject(hOctahedron, a_stPoints + i, i);

		for ( ACP_tdxIndex i = 0; i < 8; i++ )
		{
			ACP_tdxIndex *a3_xPoints = aa_xFaces[i];
			GEO_vSetFaceOfIndexedTriangles(hOctahedron, hElem, i, a3_xPoints[0], a3_xPoints[1], a3_xPoints[2]);
		}

		GEO_vEndModifyObject(hOctahedron);
	}

	int lTooFar = HIE_M_hSuperObjectGetStdGame(hActor)->ucTooFarLimit;
	MTH3D_tdstVector *p_stPos = &hActor->p_stGlobalMatrix->stPos;

	MTH3D_tdstVector a_stPoints[6];
	for ( ACP_tdxIndex i = 0; i < 6; i++ )
	{
		MTH3D_M_vMulVectorScalar(a_stPoints + i, hOctahedron->d_stListOfPoints + i, lTooFar);
		MTH3D_M_vAddVector(a_stPoints + i, a_stPoints + i, p_stPos);
	}

	hOctahedron->xBoundingSphereRadius = lTooFar;
	hOctahedron->xBoundingSphereCenter = *p_stPos;

	MTH3D_tdstVector *p_stSaveList = hOctahedron->d_stListOfPoints;
	hOctahedron->d_stListOfPoints = a_stPoints;
	GLI_xSendObjectToViewportWithLights(&GAM_g_stEngineStructure->stViewportAttr, hOctahedron, GLI_C_lAllIsEnable);
	hOctahedron->d_stListOfPoints = p_stSaveList;
}

/****************************************************************************/
#endif
