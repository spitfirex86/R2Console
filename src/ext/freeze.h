#pragma once

#include "../console.h"


extern BOOL g_bFreezeEngine;
extern int g_lFreezeRequest;


void FRZ_fn_vDoEngineFreeze( void );

tdfnCommand FRZ_fn_vFreezeCmd;
