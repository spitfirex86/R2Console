#pragma once

#include "framework.h"
#include "utils.h"


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

#define C_ClassicTransparency 0xB0
#define C_ClassicFrameColor 0x000000

#define C_Transparency 0xDC
#define C_FrameColor 0x103070


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


typedef struct tdstAutocomplete tdstAutocomplete;
typedef unsigned long tdfnCmdComplete( tdstAutocomplete *p_stAutocomplete );
typedef void tdfnCommand( int lNbArgs, char **d_szArgs );

typedef struct tdstCommand
{
    char szName[C_MaxCmdName];
	tdfnCommand *p_fnCommand;
	tdfnCmdComplete *p_fnComplete;
#if defined(USE_WATCH) 
	WAT_tdfnWatchCallback *p_fnWatchCallback;
#endif
}
tdstCommand;


extern tdstCommand* g_a_stCommands;
extern int g_lNbCommands;
void fn_vInitCommands( void );
void fn_vRegisterCommand( char *szName, tdfnCommand *p_stCommand );
void fn_vRegisterCommandWithComplete( char *szName, tdfnCommand *p_fnCommand, tdfnCmdComplete *p_fnComplete );

extern void *g_pvLastCommandData;

extern BOOL g_bForceThisCommand;


/****************************************************************************
 * Autocomplete
 ****************************************************************************/

#define C_MaxMatches 20
#define C_MaxMatchLen 64

typedef struct tdstAutocomplete
{
	unsigned char ucState;
	unsigned char ucWhich;
	unsigned long ulPos;

	unsigned long ulNextMatch;
	unsigned long ulNbMatch;

	tdstCommand *p_stCommand;

	char szToComplete[C_MaxLine];
	char szLine[C_MaxLine];

	void *a_pMatches[C_MaxMatches];
	char a_szMatches[C_MaxMatches][C_MaxMatchLen];
}
tdstAutocomplete;

extern tdstAutocomplete g_stAC;
