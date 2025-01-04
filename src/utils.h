#pragma once

#include "framework.h"


/****************************************************************************
* Utils
****************************************************************************/

int fn_lSplitArgs( char *szString, char ***p_d_szArgsOut );
void fn_vToLower( char *szDst, char *szSrc );
int fn_vCharCountReverse( char *Str, char Ch, int lMaxChars );
int fn_vNotCharCountReverse( char *Str, char Ch, int lMaxChars );

BOOL fn_bParseBool( char *szArg, BOOL *p_bOut );
BOOL fn_bParseInt( char *szArg, int *p_lOut );
BOOL fn_bParseReal( char *szArg, MTH_tdxReal *p_xOut );
int fn_lParseCoordinates( int lSize, char **d_szArgs, MTH_tdxReal *d_xOut );

BOOL fn_bParsePtr( char *szArg, void **p_pOut );
BOOL fn_bParseObjectRef( char *szArg, HIE_tdstSuperObject **p_pstOut );

void fn_vMouseCoordToPercent( MTH2D_tdstVector *p_stOut, LPARAM lParam, HWND hWnd );

/* widescreen stuff */
BOOL fn_bInitWidescreenSupport( void );
void fn_vAdjustPercentForWidescreen( MTH2D_tdstVector *p_stVec );
extern BOOL (*GLI_FIX_bIsWidescreen)( void );
extern float (*GLI_FIX_xGetActualRatio)( void );
extern float g_xScreenRatio;

extern char g_a_szMapsDesc[70][50];


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
