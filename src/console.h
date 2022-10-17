#pragma once

#include "framework.h"


/****************************************************************************
 * Main
 ****************************************************************************/

#define C_NbLines 100
#define C_LinesOnScreen 14
#define C_MaxLine 72

typedef struct tdstLine
{
	unsigned char ucColor;
	char cPrefix;

	char szText[C_MaxLine];
}
tdstLine;


extern char const g_szVersion[];

extern tdstLine g_a_stLines[C_NbLines];


void fn_vInitConsole( void );

void fn_vPrintEx( char const *szString, unsigned char ucColor, char cPrefix );
void fn_vPrintCFmt( unsigned char ucColor, char *szFmt, ... );
void fn_vPrintC( unsigned char ucColor, char const *szString );
void fn_vPrint( char const *szString );

void fn_vResetScroll( void );


/****************************************************************************
 * Commands
 ****************************************************************************/

#define C_MaxCmdName 32


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

BOOL fn_bParseBool( char *szArg, BOOL *p_bOut );
BOOL fn_bParseInt( char *szArg, int *p_lOut );
BOOL fn_bParseReal( char *szArg, MTH_tdxReal *p_xOut );
int fn_lParseCoordinates( int lSize, char **d_szArgs, MTH_tdxReal *d_xOut );

BOOL fn_bParsePtr( char *szArg, void **p_pOut );
BOOL fn_bParseObjectRef( char *szArg, HIE_tdstSuperObject **p_pstOut );
