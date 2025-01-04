#include "framework.h"
#include "console.h"
#include "utils.h"


LARGE_INTEGER g_llFreq;
LARGE_INTEGER g_a_llTimers[e_Nb_Timer];

char g_a_szMapsDesc[70][50] = {
	"Main Menu",
	"(C) Intro - Stormy Seas",
	"Intro - Jail",
	"Hall of Doors",
	"Woods of Light",
	"Fairy Glade 1",
	"Bonus Level",
	"Fairy Glade 2",
	"Fairy Glade 3",
	"Fairy Glade 4",
	"Fairy Glade 5",
	"Marshes of Awakening 1",
	"Cave of Bad Dreams 1",
	"Cave of Bad Dreams 2",
	"Marshes of Awakening 2",
	"(C) Meanwhile, on the Prison Ship",
	"Bayou 1",
	"Walk of Life",
	"Bayou 2",
	"Menhir Hills 1",
	"Menhir Hills 2",
	"Menhir Hills 3",
	"(C) Chamber of the Teensies",
	"Sanctuary of Water and Ice 1",
	"Sanctuary of Water and Ice 2",
	"Canopy 1",
	"Canopy 2",
	"Canopy 3",
	"Whale Bay 1",
	"Whale Bay 2",
	"Whale Bay 3",
	"Sanctuary of Stone and Fire 1",
	"Sanctuary of Stone and Fire 3",
	"(C) Echoing Caves Intro",
	"Echoing Caves 1",
	"Echoing Caves 2",
	"Echoing Caves 3",
	"(C) Meanwhile, on the Prison Ship - Grolgoth",
	"Precipice 1",
	"Precipice 2",
	"Precipice 3",
	"Top of the World 1",
	"Top of the World 2",
	"Sanctuary of Rock and Lava 1",
	"Sanctuary of Rock and Lava 2",
	"Walk of Power",
	"Sanctuary of Rock and Lava 3",
	"Beneath the Sanctuary of Rock and Lava 1",
	"Beneath the Sanctuary of Rock and Lava 2",
	"Beneath the Sanctuary of Rock and Lava 3",
	"Sanctuary of Stone and Fire 2",
	"Tomb of the Ancients 1",
	"Tomb of the Ancients 2",
	"Tomb of the Ancients 3",
	"Iron Mountains 1",
	"(C) Iron Mountains - Balloon",
	"Iron Mountains 2",
	"Iron Mountains 3",
	"Prison Ship 1",
	"Prison Ship 2",
	"Prison Ship 3",
	"Prison Ship 4",
	"Crow's Nest",
	"(C) Ending",
	"(C) Credits",
	"(C) Polokus - First Mask",
	"(C) Polokus - Second Mask",
	"(C) Polokus - Third Mask",
	"(C) Polokus - Fourth Mask",
	"Score Screen"
};


void fn_vInitTimer( void )
{
	QueryPerformanceFrequency(&g_llFreq);
	fn_vResetTimer(e_Timer_Null);
}

void fn_vResetTimer( tdeTimerId eId )
{
	if ( eId < 0 || eId >= e_Nb_Timer )
		return;

	QueryPerformanceCounter(&g_a_llTimers[eId]);
}

float fn_xGetTimerElapsed( tdeTimerId eId )
{
	LARGE_INTEGER llElapsed;

	if ( eId < 0 || eId >= e_Nb_Timer )
		return 0;

	QueryPerformanceCounter(&llElapsed);
	float xElapsedMs = (float)(llElapsed.QuadPart - g_a_llTimers[eId].QuadPart) * 1000.0f / (float)g_llFreq.QuadPart;
	return xElapsedMs;
}


int fn_lSplitArgs( char *szString, char ***p_d_szArgsOut )
{
	int lCount = 0;
	char **d_szArgs = NULL;

	while ( *szString )
	{
		szString += strspn(szString, " ");
		int len = strcspn(szString, " ");

		if ( len > 0 )
		{
			int lIdx = lCount++;

			char **d_szTmp = realloc(d_szArgs, lCount * sizeof(char *));
			if ( !d_szTmp )
			{
				free(d_szArgs);
				*p_d_szArgsOut = NULL;
				return 0;
			}

			d_szArgs = d_szTmp;
			d_szArgs[lIdx] = szString;
			szString += len;

			if ( *szString )
				*szString++ = 0;
		}
	}

	*p_d_szArgsOut = d_szArgs;
	return lCount;
}

void fn_vToLower( char *szDst, char *szSrc )
{
	while ( *szSrc )
		*szDst++ = (char)tolower(*szSrc++);

	*szDst = 0;
}

int fn_vCharCountReverse( char *Str, char Ch, int lMaxChars )
{
	int i;
	for ( i = 0; i < lMaxChars; i++ )
	{
		if ( Str[-i] != Ch )
			break;
	}

	return i;
}

int fn_vNotCharCountReverse( char *Str, char Ch, int lMaxChars )
{
	int i;
	for ( i = 0; i < lMaxChars; i++ )
	{
		if ( Str[-i] == Ch )
			break;
	}

	return i;
}


BOOL fn_bParseBool( char *szArg, BOOL *p_bOut )
{
	BOOL bTmp;
	if ( _stricmp(szArg, "TRUE") == 0 )
	{
		bTmp = TRUE;
	}
	else if ( _stricmp(szArg, "FALSE") == 0 )
	{
		bTmp = FALSE;
	}
	else
		return FALSE;

	*p_bOut = bTmp;
	return TRUE;
}

BOOL fn_bParseInt( char *szArg, int *p_lOut )
{
	char *pEnd;
	int lTmp = strtol(szArg, &pEnd, 0);

	if ( *pEnd != 0 )
		return FALSE;

	*p_lOut = lTmp;
	return TRUE;
}

BOOL fn_bParseReal( char *szArg, MTH_tdxReal *p_xOut )
{
	char *pEnd;
	MTH_tdxReal xTmp = strtof(szArg, &pEnd);

	if ( *pEnd != 0 )
		return FALSE;

	*p_xOut = xTmp;
	return TRUE;
}

int fn_lParseCoordinates( int lSize, char **d_szArgs, MTH_tdxReal *d_xOut )
{
	MTH_tdxReal xTmp;
	int i;

	for ( i = 0; i < lSize; i++ )
	{
		char *szArg = d_szArgs[i];
		BOOL bRelative = FALSE;

		/* relative coordinates */
		if ( *szArg == '^' )
		{
			bRelative = TRUE;
			szArg++;
		}

		if ( !fn_bParseReal(szArg, &xTmp) )
			break;

		d_xOut[i] = (bRelative)
			? xTmp + d_xOut[i]
			: xTmp;
	}

	return i;
}

BOOL fn_bParsePtr( char *szArg, void **p_pOut )
{
	char *pEnd;
	unsigned long ulTmp = strtoul(szArg, &pEnd, 16);

	if ( *pEnd != 0 )
		return FALSE;

	*p_pOut = (void *)ulTmp;
	return TRUE;
}

BOOL fn_bParseObjectRef( char *szArg, HIE_tdstSuperObject **p_pstOut )
{
	void *pTmp;
	HIE_tdstSuperObject *pSuperObj = NULL;

	if ( fn_bParsePtr(szArg, &pTmp) )
	{
		HIE_tdstSuperObject *pRoot = *GAM_g_p_stDynamicWorld;
		HIE_tdstSuperObject *pChild;
		LST_M_DynamicForEach(pRoot, pChild)
		{
			if ( pTmp == pChild )
			{
				pSuperObj = pTmp;
				break;
			}
		}
	}
	else
	{
		pSuperObj = HIE_fn_p_stFindObjectByName(szArg);
	}

	if ( !pSuperObj )
		return FALSE;

	*p_pstOut = pSuperObj;
	return TRUE;
}

void fn_vMouseCoordToPercent( MTH2D_tdstVector *p_stOut, LPARAM lParam, HWND hWnd )
{
	RECT rc;
	GetClientRect(hWnd, &rc);

	MTH2D_tdstVector p_stCoord = {
		(float)LOWORD(lParam) / (float)(rc.right - rc.left) * 100.0f,
		(float)HIWORD(lParam) / (float)(rc.bottom - rc.top) * 100.0f
	};

	if ( GLI_FIX_bIsWidescreen() )
		fn_vAdjustPercentForWidescreen(&p_stCoord);

	*p_stOut = p_stCoord;
}


/* widescreen stuff */

BOOL fn_bIsWidescreen_Null( void ) { return FALSE; }
float fn_xGetActualRatio_Null( void ) { return 0.75f; }

BOOL (*GLI_FIX_bIsWidescreen)( void ) = fn_bIsWidescreen_Null;
float (*GLI_FIX_xGetActualRatio)( void ) = fn_xGetActualRatio_Null;

BOOL g_bIsWidescreenInit = FALSE;
float g_xScreenRatio = 0.75f;

BOOL fn_bInitWidescreenSupport( void )
{
	if ( g_bIsWidescreenInit )
		return TRUE;

	HMODULE hFixModule = GetModuleHandle("GliFixVf");
	if ( !hFixModule )
		return FALSE;

	GLI_FIX_bIsWidescreen = (BOOL (*)(void))GetProcAddress(hFixModule, "GLI_FIX_bIsWidescreen");
	GLI_FIX_xGetActualRatio = (float (*)(void))GetProcAddress(hFixModule, "GLI_FIX_xGetActualRatio");

	if ( !GLI_FIX_bIsWidescreen || !GLI_FIX_xGetActualRatio )
	{
		GLI_FIX_bIsWidescreen = fn_bIsWidescreen_Null;
		GLI_FIX_xGetActualRatio = fn_xGetActualRatio_Null;
		return FALSE;
	}

	g_bIsWidescreenInit = TRUE;
	g_xScreenRatio = GLI_FIX_xGetActualRatio();
	return TRUE;
}

void fn_vAdjustPercentForWidescreen( MTH2D_tdstVector *p_stVec )
{
	float xRatio = g_xScreenRatio / 0.75f;
	p_stVec->x -= (1 - xRatio) / 2 * 100.f;
	p_stVec->x /= xRatio;
}
