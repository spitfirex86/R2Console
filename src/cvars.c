#include "framework.h"
#include "console.h"
#include "cvars.h"


tdstCVar **g_d_pstCVars = NULL;
long g_lNbCVars = 0;


tdstCVar * fn_p_stGetCVar( char *szName )
{
	if ( !g_d_pstCVars )
		return NULL;

	for ( int i = 0; i < g_lNbCVars; i++ )
	{
		if ( _stricmp(szName, g_d_pstCVars[i]->szName) == 0 )
		{
			return g_d_pstCVars[i];
		}
	}

	return NULL;
}

tdstCVar * fn_p_stCreateCVar( char *szName, tdeCVarType eType )
{
	tdstCVar *pstVar = malloc(sizeof(tdstCVar));
	if ( !pstVar )
		return NULL;

	int i = g_lNbCVars++;
	tdstCVar **d_pstTmp = realloc(g_d_pstCVars, sizeof(tdstCVar *)*g_lNbCVars);
	if ( !d_pstTmp )
	{
		g_lNbCVars--;
		free(pstVar);
		return NULL;
	}

	g_d_pstCVars = d_pstTmp;
	g_d_pstCVars[i] = pstVar;

	strncpy(pstVar->szName, szName, C_MaxCVarName);
	pstVar->szName[C_MaxCVarName-1] = 0;
	pstVar->eType = eType;
	pstVar->lValue = 0;

	return pstVar;
}
