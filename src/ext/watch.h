#pragma once

#include "../framework.h"
#include "../console.h"

#if !defined(USE_WATCH)
#define USE_WATCH
#endif


typedef void WAT_tdfnWatchCallback( void *Data, SPTXT_tdstTextInfo *p_stInfo );

tdfnCommand WAT_fn_vWatchCmd;

void WAT_fn_vHitTestClose( MTH2D_tdstVector *p_stPos );
void WAT_fn_vHitTestMove( MTH2D_tdstVector *p_stPos, BOOL bLeftButton );
