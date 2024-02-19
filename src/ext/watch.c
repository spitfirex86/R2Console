#include "../framework.h"
#include "watch.h"


#if defined(USE_WATCH)


typedef struct tdstWatch
{
	void *Data;
	WAT_tdfnWatchCallback *p_fnCallback;

	MTH2D_tdstVector stPos;
	MTH2D_tdstVector stSize;
	BOOL bShouldClose;
}
tdstWatch;

#define C_MaxWatch 16
long lNbWatch = 0;
tdstWatch a_stWatchTab[C_MaxWatch] = { 0 };

BOOL bWatchInit = FALSE;

/*
void WAT_fn_vWatchCmd( char const *szArgs )
{
	if ( g_lLastCommandId < 0 )
	{
		fn_vPrintC("No command to watch", 2);
		return;
	}

	tdstCommand *pCommand = &g_a_stCommands[g_lLastCommandId];

	if ( pCommand->p_fnWatchCallback == NULL )
	{
		fn_vPrintC("Cannot watch this command", 2);
		return;
	}

	fn_vAddWatch(g_pvLastCommandData, pCommand->p_fnWatchCallback);
}


void CALLBACK fn_vWatchCallback( SPTXT_tdstTextInfo *pInfo )
{
	*GLI_p_fZValueForSprite = 1.2f;
	pInfo->xSize = 6;
	pInfo->ucAlpha = 0xD0;
	pInfo->bWantExtents = TRUE;

	for ( int i = 0; i < lNbWatch; i++ )
	{
		tdstWatch *pWatch = &a_stWatchTab[i];

		if ( pWatch->bShouldClose )
		{
			memmove(pWatch, pWatch+1, sizeof(tdstWatch) * (C_MaxWatch-1-i));
			lNbWatch--;

			if ( i >= lNbWatch )
				break;
		}

		pInfo->xExtentX = pInfo->xExtentY = 0;
		pInfo->X = 10 * (pWatch->stPos.x + 1);
		pInfo->Y = 10 * (pWatch->stPos.y + 1);

		// do callback
		pWatch->p_fnCallback(pWatch->Data, pInfo);

		pWatch->stSize.x = pInfo->xExtentX + 2;
		pWatch->stSize.y = pInfo->xExtentY + 2;

		MTH2D_tdstVector stBR = { pWatch->stPos.x + pWatch->stSize.x, pWatch->stPos.y + pWatch->stSize.y };
		GFX_fn_vDisplayFrameWithZValue(&pWatch->stPos, &stBR, 0xC0, 1.1f, &GAM_g_stEngineStructure->stFixViewportAttr);
	}

	*GLI_p_fZValueForSprite = 0.998f;
}

void fn_vAddWatch( void *Data, WAT_tdfnWatchCallback *pCallback )
{
	if ( !bWatchInit )
	{
		SPTXT_vAddTextCallback(fn_vWatchCallback);
		bWatchInit = TRUE;
	}

	tdstWatch *pWatch = &a_stWatchTab[lNbWatch++];
	pWatch->Data = Data;
	pWatch->p_fnCallback = pCallback;
	pWatch->stPos.x = 10;
	pWatch->stPos.y = 40;
}

void WAT_fn_vMap( void *Data, SPTXT_tdstTextInfo *p_stInfo )
{
	SPTXT_vPrintLine(TXT_Red("map"));
	SPTXT_vPrintFmtLine("%s", GAM_g_stEngineStructure->szLevelName);
}

void WAT_fn_vListObj( void *Data, SPTXT_tdstTextInfo *p_stInfo )
{
	BOOL bOnlyVisible = (BOOL)Data;
	char szBuffer[128];

	SPTXT_vPrintFmtLine(TXT_Red("listobj = %s"), bOnlyVisible ? "vis" : "all");
	SPTXT_vNewLine();

	HIE_tdstSuperObject *pRoot = *GAM_pp_stDynamicWorld;
	HIE_tdstSuperObject *pChild;
	LST_M_DynamicForEach(pRoot, pChild)
	{
		if ( pChild->ulType == HIE_C_Type_Actor )
		{
			if ( bOnlyVisible && (pChild->hLinkedObject.p_stCharacter->hStandardGame->ulCustomBits & Std_C_CustBit_OutOfVisibility) )
				continue;

			SPTXT_vPrintFmtLine("%8p = %s", pChild, XHIE_fn_szGetSuperObjectPersonalName(pChild));
		}
	}
}

void WAT_fn_vGetSetPos( void *Data, SPTXT_tdstTextInfo *p_stInfo )
{
	HIE_tdstSuperObject *pSpo = (HIE_tdstSuperObject *)Data;

	SPTXT_vPrintFmtLine(TXT_Red("pos = %8p"), pSpo);
	SPTXT_vPrintFmtLine(TXT_Yellow("%s"), XHIE_fn_szGetSuperObjectPersonalName(pSpo));
	SPTXT_vNewLine();

	MTH3D_tdstVector *pPos = &pSpo->p_stGlobalMatrix->stPos;
	SPTXT_vPrintFmtLine("x = %.3f", pPos->x);
	SPTXT_vPrintFmtLine("y = %.3f", pPos->y);
	SPTXT_vPrintFmtLine("z = %.3f", pPos->z);
}

void WAT_fn_vHexView( void *Data, SPTXT_tdstTextInfo *p_stInfo )
{
	unsigned char *ptr = Data;

	SPTXT_vPrintFmtLine(TXT_Red("hex = %8p"), ptr);
	SPTXT_vNewLine();

	SPTXT_vPrintFmtLine("%8p = %02.2x %02.2x %02.2x %02.2x = %c%c%c%c",
			ptr, ptr[0], ptr[1], ptr[2], ptr[3],
			isprint(ptr[0]) ? ptr[0] : '.', isprint(ptr[1]) ? ptr[1] : '.',
			isprint(ptr[2]) ? ptr[2] : '.', isprint(ptr[3]) ? ptr[3] : '.'
	);
	ptr += 4;

	SPTXT_vPrintFmtLine("%8p = %02.2x %02.2x %02.2x %02.2x = %c%c%c%c",
			ptr, ptr[0], ptr[1], ptr[2], ptr[3],
			isprint(ptr[0]) ? ptr[0] : '.', isprint(ptr[1]) ? ptr[1] : '.',
			isprint(ptr[2]) ? ptr[2] : '.', isprint(ptr[3]) ? ptr[3] : '.'
	);
	ptr += 4;

	SPTXT_vPrintFmtLine("%8p = %02.2x %02.2x %02.2x %02.2x = %c%c%c%c",
			ptr, ptr[0], ptr[1], ptr[2], ptr[3],
			isprint(ptr[0]) ? ptr[0] : '.', isprint(ptr[1]) ? ptr[1] : '.',
			isprint(ptr[2]) ? ptr[2] : '.', isprint(ptr[3]) ? ptr[3] : '.'
	);
	ptr += 4;

	SPTXT_vPrintFmtLine("%8p = %02.2x %02.2x %02.2x %02.2x = %c%c%c%c",
			ptr, ptr[0], ptr[1], ptr[2], ptr[3],
			isprint(ptr[0]) ? ptr[0] : '.', isprint(ptr[1]) ? ptr[1] : '.',
			isprint(ptr[2]) ? ptr[2] : '.', isprint(ptr[3]) ? ptr[3] : '.'
	);
}

*/

void WAT_fn_vHitTestClose( MTH2D_tdstVector *p_stPos )
{
	for ( int i = 0; i < lNbWatch; i++ )
	{
		tdstWatch *pWatch = &a_stWatchTab[i];

		if ( p_stPos->x > pWatch->stPos.x && p_stPos->x < (pWatch->stPos.x + pWatch->stSize.x)
			&& p_stPos->y > pWatch->stPos.y && p_stPos->y < (pWatch->stPos.y + pWatch->stSize.y) )
		{
			pWatch->bShouldClose = TRUE;
			break;
		}
	}
}

void WAT_fn_vHitTestMove( MTH2D_tdstVector *p_stPos, BOOL bLeftButton )
{
	static MTH2D_tdstVector stOffset = { 0 };
	static long lPressedCount = 0;
	static BOOL bShouldMove = FALSE;
	static MTH2D_tdstVector *pPosToMove = NULL;

	if ( bLeftButton )
	{
		lPressedCount++;

		if ( lPressedCount == 1 )
		{
			// hit test all watch windows
			for ( int i = 0; i < lNbWatch; i++ )
			{
				tdstWatch *pWatch = &a_stWatchTab[i];

				if ( p_stPos->x > pWatch->stPos.x && p_stPos->x < (pWatch->stPos.x + pWatch->stSize.x)
					&& p_stPos->y > pWatch->stPos.y && p_stPos->y < (pWatch->stPos.y + pWatch->stSize.y) )
				{
					bShouldMove = TRUE;
					pPosToMove = &pWatch->stPos;
					stOffset.x = p_stPos->x - pWatch->stPos.x;
					stOffset.y = p_stPos->y - pWatch->stPos.y;

					break;
				}
			}
		}

		if ( bShouldMove )
		{
			// move the window
			pPosToMove->x = p_stPos->x - stOffset.x;
			pPosToMove->y = p_stPos->y - stOffset.y;
		}
	}
	else
	{
		lPressedCount = 0;
		bShouldMove = FALSE;
		pPosToMove = NULL;
		stOffset.x = stOffset.y = 0;
	}
}


#endif
