/****************************************************************************
 * R2Console public API
 ****************************************************************************/

#pragma once

// requires ACP_Ray2
#include <ACP_Ray2.h>


#ifdef R2CONSOLE_EXPORTS
#define R2CON_API __declspec(dllexport)
#else
#define R2CON_API __declspec(dllimport)
#endif


// command handling

typedef void tdfnCommand( int lNbArgs, char **d_szArgs );

R2CON_API void fn_vRegisterCommand( char *szName, tdfnCommand *p_stCommand );


// output

R2CON_API void fn_vPrintEx( char const *szString, unsigned char ucColor, char cPrefix );
R2CON_API void fn_vPrintCFmt( unsigned char ucColor, char *szFmt, ... );
R2CON_API void fn_vPrintC( unsigned char ucColor, char const *szString );
R2CON_API void fn_vPrint( char const *szString );


// utils

R2CON_API void fn_vToLower( char *szDst, char *szSrc );

R2CON_API BOOL fn_bParseBool( char *szArg, BOOL *p_bOut );
R2CON_API BOOL fn_bParseInt( char *szArg, int *p_lOut );
R2CON_API BOOL fn_bParseReal( char *szArg, MTH_tdxReal *p_xOut );
R2CON_API int fn_lParseCoordinates( int lSize, char **d_szArgs, MTH_tdxReal *d_xOut );

R2CON_API BOOL fn_bParsePtr( char *szArg, void **p_pOut );
R2CON_API BOOL fn_bParseObjectRef( char *szArg, HIE_tdstSuperObject **p_pstOut );
