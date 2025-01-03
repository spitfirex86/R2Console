#pragma once

#include "framework.h"

#define LIBRARY_API __declspec(dllimport)


/****************************************************************************
* Utils
****************************************************************************/

LIBRARY_API int fn_lSplitArgs( char *szString, char ***p_d_szArgsOut );
LIBRARY_API void fn_vToLower( char *szDst, char *szSrc );
LIBRARY_API int fn_vCharCountReverse( char *Str, char Ch, int lMaxChars );
LIBRARY_API int fn_vNotCharCountReverse( char *Str, char Ch, int lMaxChars );

LIBRARY_API BOOL fn_bParseBool( char *szArg, BOOL *p_bOut );
LIBRARY_API BOOL fn_bParseInt( char *szArg, int *p_lOut );
LIBRARY_API BOOL fn_bParseReal( char *szArg, MTH_tdxReal *p_xOut );
LIBRARY_API int fn_lParseCoordinates( int lSize, char **d_szArgs, MTH_tdxReal *d_xOut );

LIBRARY_API BOOL fn_bParsePtr( char *szArg, void **p_pOut );
LIBRARY_API BOOL fn_bParseObjectRef( char *szArg, HIE_tdstSuperObject **p_pstOut );

LIBRARY_API void fn_vMouseCoordToPercent( MTH2D_tdstVector *p_stOut, LPARAM lParam, HWND hWnd );

/* widescreen stuff */
BOOL fn_bInitWidescreenSupport( void );
void fn_vAdjustPercentForWidescreen( MTH2D_tdstVector *p_stVec );
extern BOOL (*GLI_FIX_bIsWidescreen)( void );
extern float (*GLI_FIX_xGetActualRatio)( void );
extern float g_xScreenRatio;


typedef enum tdeTimerId
{
	e_Timer_Null,
	e_Timer_PerfCmd,

	e_Nb_Timer
}
tdeTimerId;

void fn_vInitTimer( void );
void fn_vResetTimer( tdeTimerId eId );
float fn_xGetTimerElapsed( tdeTimerId eId );
