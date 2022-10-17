#pragma once

#include "../framework.h"


#define M_PercentToFontX(x) ((x)*6.4f)
#define M_PercentToFontY(y) ((y)*4.8f)
#define M_FontToPercentX(x) ((x)/6.4f)
#define M_FontToPercentY(y) ((y)/4.8f)

#define C_Font_xCharWidth 10.0f
#define C_Font_xCharHeight 12.0f

#define C_Font_xActualCharWidth (C_Font_xCharWidth-1.0f)


long fn_lGetFmtStringLength( char const *szFmt, va_list args );

void FNT_fn_vFontInit( void );
void FNT_fn_vLoadFontTexture( void );

void FNT_fn_vDisplayString( MTH_tdxReal xX, MTH_tdxReal xY, char *szString );
void FNT_fn_vDisplayStringFmt( MTH_tdxReal xX, MTH_tdxReal xY, char const *szFmt, ... );
