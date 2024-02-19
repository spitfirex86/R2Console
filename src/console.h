#pragma once

#include "framework.h"


/****************************************************************************
 * Main
 ****************************************************************************/

#define C_NbLines 100
#define C_LinesOnScreen 14
#define C_LinesOnScreenTiny 2
#define C_MaxLine 72
#define C_MaxHiLite 4

#define C_AnimFrames 30
#define C_AnimFramesTiny 6
#define C_Transparency 0xB0


typedef struct tdstHiLite
{
	char cFrom;
	char cTo;
}
tdstHiLite;

typedef struct tdstLine
{
	unsigned char ucColor;
	char cPrefix;

	tdstHiLite stHiLite[C_MaxHiLite];

	char szText[C_MaxLine];
}
tdstLine;


extern char const g_szVersion[];

extern tdstLine g_a_stLines[C_NbLines];


void fn_vEarlyInitConsole( void );
void fn_vInitConsole( void );

void fn_vPrintEx( char const *szString, unsigned char ucColor, char cPrefix );
void fn_vPrintCFmt( unsigned char ucColor, char *szFmt, ... );
void fn_vPrintC( unsigned char ucColor, char const *szString );
void fn_vPrint( char const *szString );

void fn_vResetScroll( void );
void fn_vPasteAtCaret( char *szStr, int lStrLen );


/****************************************************************************
 * Commands
 ****************************************************************************/

#define C_MaxCmdName 32

#define C_cHiLiteBegin	'\002'
#define C_cHiLiteEnd	'\003'
#define C_szHiLiteBegin	"\002"
#define C_szHiLiteEnd	"\003"

#define M_HiLite(str) C_szHiLiteBegin ## str ## C_szHiLiteEnd


typedef void tdfnCommand( int lNbArgs, char **d_szArgs );

typedef struct tdstCommand
{
    char szName[C_MaxCmdName];
	tdfnCommand *p_stCommand;
#if defined(USE_WATCH) 
	WAT_tdfnWatchCallback *p_fnWatchCallback;
#endif
}
tdstCommand;


extern tdstCommand g_a_stCommands[];
extern int const g_lNbCommands;

extern void *g_pvLastCommandData;

extern BOOL g_bForceThisCommand;


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
