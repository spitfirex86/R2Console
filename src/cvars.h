#pragma once

#include "framework.h"


#define C_MaxCVarName 32

typedef enum tdeCVarType
{
	E_CVarBool,
	E_CVarInt,
	E_CVarReal
}
tdeCVarType;

typedef struct tdstCVar
{
	char szName[C_MaxCVarName];
	tdeCVarType eType;

	union
	{
		BOOL bValue;
		int lValue;
		MTH_tdxReal xValue;
	};
}
tdstCVar;


extern tdstCVar **g_d_pstCVars;
extern long g_lNbCVars;

tdstCVar * fn_p_stGetCVar( char *szName );
tdstCVar * fn_p_stCreateCVar( char *szName, tdeCVarType eType );
