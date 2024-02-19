#include "framework.h"
#include "pick.h"
#include "console.h"
#include "gfx/font.h"
#include "gfx/graphics.h"


#if defined(USE_PICK)


tdstPickInfo g_stPickInfo = { 0 };


void fn_vResetPickedMenu( void );
void fn_vPickedMenuCalcSize( void );

/* for hit test filtering*/
extern MTH2D_tdstVector g_stCurrentPos;
extern MTH2D_tdstVector *g_p_stSize;


void fn_vGetPosCamAndMouse(
	GLD_tdstViewportAttributes *p_stVpt,
	MTH2D_tdstVector *p_stMouse2D,
	MTH3D_tdstVector *p_stMouse3D,
	MTH3D_tdstVector *p_stCamera
	)
{
	GLI_tdstCamera *p_stCam = GLD_M_hGetSpecificTo3D(p_stVpt)->p_stCam;

	MTH_tdxReal xScreen = p_stCam->xScreen;
	GLI_tdst2DVertex stTrans = p_stCam->stTrans;
	MTH3D_tdstVector stTemp = {
		(p_stMouse2D->x - stTrans.xX) / p_stCam->stScale.xX,
		(p_stMouse2D->y - stTrans.xY) / p_stCam->stScale.xY,
		xScreen
	};

	POS_tdstCompletePosition stMatrix, stInvMatrix;
	MTH3D_tdstVector stSouris3D;

	stMatrix = p_stCam->stMatrix;
	POS_fn_vInvertMatrix(&stInvMatrix, &stMatrix);
	POS_fn_vMulMatrixVertex(&stSouris3D, &stInvMatrix, &stTemp);

	*p_stCamera = stInvMatrix.stPos;
	*p_stMouse3D = stSouris3D;
}

BOOL fn_bIsSuperObjHitByRay(
	HIE_tdstSuperObject *hSuperObj,
	MTH3D_tdstVector *p_stRayFrom,
	MTH3D_tdstVector *p_stDirNormal,
	MTH3D_tdstVector *p_stObjCenterOut
	)
{
	MTH_tdxReal xRadius;
	MTH3D_tdstVector stCenter;
	MTH3D_tdstVector *p_stObjPos = &hSuperObj->p_stGlobalMatrix->stPos;

	if ( !(hSuperObj->ulFlags & HIE_C_Flag_TypeOfBoundingVolume) && hSuperObj->pBoundingVolume ) /* sphere */
	{
		GEO_tdstBoundingSphere *p_stBV = hSuperObj->pBoundingVolume;
		xRadius = p_stBV->xRadius;
		if ( xRadius < 1.5f )
			xRadius = 1.5f;
		stCenter = p_stBV->stCenterPoint;
		MTH3D_M_vAddVector(&stCenter, &stCenter, p_stObjPos);
	}
	else /* box or nothing */
	{
		xRadius = 1.5f;
		stCenter = *p_stObjPos;
	}

	MTH3D_tdstVector stDiff;
	MTH3D_M_vSubVector(&stDiff, p_stRayFrom, &stCenter);

	MTH_tdxReal xNormSq = MTH3D_M_xSqrNormVector(p_stDirNormal);
	MTH_tdxReal xB = 2 * MTH3D_M_xDotProductVector(p_stDirNormal, &stDiff);
	MTH_tdxReal xC =  MTH3D_M_xSqrNormVector(&stDiff) - (xRadius * xRadius);
	MTH_tdxReal xD = (xB * xB) - (4 * xNormSq * xC);

	if ( xD >= 0 )
	{
		*p_stObjCenterOut = stCenter;
		return TRUE;
	}
	return FALSE;
}

void fn_vPickSuperObj(
	tdstPickInfo *p_stPickInfo,
	HIE_tdstSuperObject *hSuperObj,
	MTH3D_tdstVector *p_stMouse3D,
	MTH3D_tdstVector *p_stCamera,
	MTH3D_tdstVector *p_stVecAB,
	int lDepth
	)
{
	HIE_tdstSuperObject *hPicked = NULL;
	MTH3D_tdstVector stCenter;

	//if ( hSuperObj->ulFlags & HIE_C_Flag_NotPickable || p_stPickInfo->lNbPicked == C_MaxPicked )
	if ( p_stPickInfo->lNbPicked == C_MaxPicked )
		return;

	//HIE_fn_vPushMatrix(hSuperObj);

	if ( HIE_M_bSuperObjectIsActor(hSuperObj) && !HIE_M_hSuperObjectGetActor(hSuperObj)->hCineInfo )
	{
		if ( fn_bIsSuperObjHitByRay(hSuperObj, p_stCamera, p_stVecAB, &stCenter) )
		{
			int i = p_stPickInfo->lNbPicked++;
			p_stPickInfo->a_stPicked[i].hSuperObj = hSuperObj;
			p_stPickInfo->a_stPicked[i].stCenter = stCenter;
		}
	}

	if ( (lDepth--) > 0 )
	{
		HIE_tdstSuperObject *hChild;
		LST_M_DynamicForEach(hSuperObj, hChild)
		{
			fn_vPickSuperObj(p_stPickInfo, hChild, p_stMouse3D, p_stCamera, p_stVecAB, lDepth);
		}
	}

	//HIE_fn_bPopMatrix();
}

void fn_vPickObjectFromWorld( unsigned short uwMouseX, unsigned short uwMouseY, MTH2D_tdstVector *p_stMousePercent )
{
	MTH2D_tdstVector stMouse2D = { uwMouseX, uwMouseY };
	MTH3D_tdstVector stMouse3D, stCamera;
	MTH3D_tdstVector stVecAB, stVecABNormal;

	MTH_tdxReal xMinY = g_stCurrentPos.y + g_p_stSize->y;

	if ( p_stMousePercent->y <= xMinY )
	{
		fn_vResetPickedMenu();
		return;
	}

	fn_vGetPosCamAndMouse(&GAM_g_stEngineStructure->stViewportAttr, &stMouse2D, &stMouse3D, &stCamera);

	MTH3D_M_vSubVector(&stVecAB, &stMouse3D, &stCamera);
	MTH3D_M_vNormalizeVector(&stVecABNormal, &stVecAB);

	fn_vResetPickedMenu();
	g_stPickInfo.stClickPos = *p_stMousePercent;
	fn_vPickSuperObj(&g_stPickInfo, *GAM_g_p_stDynamicWorld, &stMouse3D, &stCamera, &stVecABNormal, 2);
	fn_vPickedMenuCalcSize();
}


#define C_xMenuMargin 1.0f

MTH2D_tdstVector g_stMenuSize = { 0 };
BOOL g_bMenuCalculated = FALSE;
int g_lPickMenuSel = -1;


void fn_vPickedMenuCalcSize( void )
{
	unsigned int ulMostChars = 0;
	for ( int i = 0; i < g_stPickInfo.lNbPicked; i++ )
	{
		tdstSuperObjPickList *pItem = &g_stPickInfo.a_stPicked[i];
		char *szName = HIE_fn_szGetObjectPersonalName(pItem->hSuperObj);

		if ( szName )
			snprintf(pItem->szName, sizeof(pItem->szName), "%s", szName);
		else
			snprintf(pItem->szName, sizeof(pItem->szName), "%8p", pItem->hSuperObj);

		unsigned int ulStrLen = strlen(pItem->szName);
		if ( ulStrLen > ulMostChars )
			ulMostChars = ulStrLen;
	}

	g_stMenuSize.x = ulMostChars * C_Font_xCharWidthInPercent;
	g_stMenuSize.y = g_stPickInfo.lNbPicked * C_Font_xCharHeightInPercent;

	if ( g_stPickInfo.stClickPos.x + g_stMenuSize.x > 100.0f )
	{
		g_stPickInfo.stClickPos.x -= g_stMenuSize.x;
		if ( g_stPickInfo.stClickPos.x < 0 )
			g_stPickInfo.stClickPos.x = 0;
	}

	if ( g_stPickInfo.stClickPos.y + g_stMenuSize.y > 100.0f )
		g_stPickInfo.stClickPos.y -= g_stMenuSize.y;

	g_bMenuCalculated = TRUE;
}

void fn_vDrawPickedMenu( void )
{
	if ( g_stPickInfo.lNbPicked == 0 )
		return;

	MTH_tdxReal xSaveZValue = *GLI_g_fZValueForSprite;
	*GLI_g_fZValueForSprite = 1.4f;

	MTH_tdxReal x = M_PercentToFontX(g_stPickInfo.stClickPos.x);
	MTH_tdxReal y = M_PercentToFontY(g_stPickInfo.stClickPos.y);

	for ( int i = 0; i < g_stPickInfo.lNbPicked; i++ )
	{
		tdstSuperObjPickList *pItem = &g_stPickInfo.a_stPicked[i];
		char *szName = pItem->szName;

		if ( i == g_lPickMenuSel )
			FNT_fn_vDisplayStringFmt(x, y, "\023%s", szName);
		else
			FNT_fn_vDisplayStringFmt(x, y, "%s", szName);

		y += C_Font_xCharHeight;
	}

	MTH2D_tdstVector stFrom = {
		g_stPickInfo.stClickPos.x - C_xMenuMargin,
		g_stPickInfo.stClickPos.y - C_xMenuMargin,
	};
	MTH2D_tdstVector stTo = {
		g_stPickInfo.stClickPos.x + g_stMenuSize.x + C_xMenuMargin,
		g_stPickInfo.stClickPos.y + g_stMenuSize.y + C_xMenuMargin,
	};

	GFX_fn_vDisplayFrameWithZValue(&stFrom, &stTo, C_Transparency, 1.3f, &GAM_g_stEngineStructure->stFixViewportAttr);

	*GLI_g_fZValueForSprite = xSaveZValue;
}

void fn_vResetPickedMenu( void )
{
	g_stPickInfo.lNbPicked = 0;
	g_bMenuCalculated = FALSE;
	g_lPickMenuSel = -1;
}

void fn_vPickMenuHitTest( MTH2D_tdstVector *p_stPos )
{
	if ( g_stPickInfo.lNbPicked == 0 )
		return;

	MTH2D_tdstVector *p_stMenu = &g_stPickInfo.stClickPos;

	MTH_tdxReal xPosX = p_stPos->x - p_stMenu->x;
	MTH_tdxReal xPosY = p_stPos->y - p_stMenu->y;

	if ( xPosX > 0 && xPosX < g_stMenuSize.x
		&& xPosY > 0 && xPosY < g_stMenuSize.y )
	{
		int lLineFromTop = (int)(xPosY / C_Font_xCharHeightInPercent);
		g_lPickMenuSel = lLineFromTop;
	}
	else
	{
		g_lPickMenuSel = -1;
	}
}

void fn_vPickMenuSelect( MTH2D_tdstVector *p_stPos )
{
	if ( g_stPickInfo.lNbPicked == 0 || g_lPickMenuSel == -1 )
	{
		fn_vResetPickedMenu();
		return;
	}

	tdstSuperObjPickList *p_stPicked = &g_stPickInfo.a_stPicked[g_lPickMenuSel];
	char *szName = p_stPicked->szName;

	fn_vPasteAtCaret(szName, strlen(szName));

	fn_vResetPickedMenu();
}

#if 0
void (*CAM_fn_vCameraManagement)( void ) = OFFSET(0x40BAD0);
void fn_vCameraManagement( void )
{

	if ( g_stPickInfo.lNbPicked > 0 )
	{
		HIE_tdstSuperObject *hCamera = GAM_g_stEngineStructure->g_hStdCamCharacter;
		CAM_tdstCineinfo *hCineinfo = hCamera->hLinkedObject.p_stActor->hCineInfo;

		if ( g_lPickMenuSel > -1 )
		{
			tdstSuperObjPickList *p_stPicked = &g_stPickInfo.a_stPicked[g_lPickMenuSel];
			MTH3D_tdstVector stObjPos = p_stPicked->stCenter;

			CAM_fn_vSetCineinfoWorkFromCurrent(hCineinfo);
			HIE_tdstSuperObject *hOldTarget = hCineinfo->hWork->hSuperObjectTargeted;
			MTH3D_tdstVector stOldObjPos = hOldTarget->p_stGlobalMatrix->stPos;
			stOldObjPos.z += 5.0f;

			DNM_tdstDynam *hDynam = HIE_M_hSuperObjectGetActor(p_stPicked->hSuperObj)->hDynam;
			if ( hDynam && hDynam->p_stDynamics->stDynamicsBase.p_stReport )
			{
				hCineinfo->hWork->hSuperObjectTargeted = p_stPicked->hSuperObj;
				stObjPos = p_stPicked->hSuperObj->p_stGlobalMatrix->stPos;

				hCineinfo->hWork->uwIAFlags &= ~CAM_C_IAFlags_PositionIsAlreadyComputed;
				hCineinfo->hWork->uwIAFlags &= ~CAM_C_IAFlags_TargetIsAlreadyComputed;
			}
			else
			{
				MTH3D_tdstVector stTemp;
				MTH3D_M_vSubVector(&stTemp, &stOldObjPos, &stObjPos);
				MTH3D_M_vNormalizeVector(&stTemp, &stTemp);
				MTH3D_M_vMulVectorScalar(&stTemp, &stTemp, 8.0f);
				MTH3D_M_vAddVector(&stTemp, &stObjPos, &stTemp);

				hCineinfo->stForcePosition = stTemp;
				hCineinfo->hWork->uwIAFlags |= CAM_C_IAFlags_PositionIsAlreadyComputed;

				hCineinfo->stForceTarget = stObjPos;
				hCineinfo->hWork->uwIAFlags |= CAM_C_IAFlags_TargetIsAlreadyComputed;
			}
		}

		hCineinfo->hWork->xLinearSpeed = 30.0f;
		hCineinfo->hWork->xAngularSpeed = 40.0f;
		hCineinfo->hWork->xTargetSpeed = 30.0f;
		hCineinfo->hWork->xLinearIncreaseSpeed = 15.0f;
		hCineinfo->hWork->xAngularIncreaseSpeed = 15.0f;
		hCineinfo->hWork->xTargetIncreaseSpeed = 15.0f;

		hCineinfo->hWork->uwIAFlags |= CAM_C_IAFlags_NoDynamicTarget | CAM_C_IAFlags_NoDynSpeed;
		hCineinfo->eState = CAM_e_State_GoToOptimal;

		CAM_fn_vUpdateTargetPosition(hCineinfo);
	}

	CAM_fn_vCameraManagement();
}
#endif


#endif
