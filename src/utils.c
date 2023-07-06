#include "framework.h"
#include "console.h"


int fn_lSplitArgs( char *szString, char ***p_d_szArgsOut )
{
	int lCount = 0;
	char **d_szArgs = NULL;

	while ( *szString )
	{
		szString += strspn(szString, " ");
		int len = strcspn(szString, " ");

		if ( len > 0 )
		{
			int lIdx = lCount++;

			char **d_szTmp = realloc(d_szArgs, lCount * sizeof(char *));
			if ( !d_szTmp )
			{
				free(d_szArgs);
				*p_d_szArgsOut = NULL;
				return 0;
			}

			d_szArgs = d_szTmp;
			d_szArgs[lIdx] = szString;
			szString += len;

			if ( *szString )
				*szString++ = 0;
		}
	}

	*p_d_szArgsOut = d_szArgs;
	return lCount;
}

void fn_vToLower( char *szDst, char *szSrc )
{
	while ( *szSrc )
		*szDst++ = (char)tolower(*szSrc++);

	*szDst = 0;
}

int fn_vCharCountReverse( char *Str, char Ch, int lMaxChars )
{
	int i;
	for ( i = 0; i < lMaxChars; i++ )
	{
		if ( Str[-i] != Ch )
			break;
	}

	return i;
}

int fn_vNotCharCountReverse( char *Str, char Ch, int lMaxChars )
{
	int i;
	for ( i = 0; i < lMaxChars; i++ )
	{
		if ( Str[-i] == Ch )
			break;
	}

	return i;
}


BOOL fn_bParseBool( char *szArg, BOOL *p_bOut )
{
	BOOL bTmp;
	if ( _stricmp(szArg, "TRUE") == 0 )
	{
		bTmp = TRUE;
	}
	else if ( _stricmp(szArg, "FALSE") == 0 )
	{
		bTmp = FALSE;
	}
	else
		return FALSE;

	*p_bOut = bTmp;
	return TRUE;
}

BOOL fn_bParseInt( char *szArg, int *p_lOut )
{
	char *pEnd;
	int lTmp = strtol(szArg, &pEnd, 0);

	if ( *pEnd != 0 )
		return FALSE;

	*p_lOut = lTmp;
	return TRUE;
}

BOOL fn_bParseReal( char *szArg, MTH_tdxReal *p_xOut )
{
	char *pEnd;
	MTH_tdxReal xTmp = strtof(szArg, &pEnd);

	if ( *pEnd != 0 )
		return FALSE;

	*p_xOut = xTmp;
	return TRUE;
}

int fn_lParseCoordinates( int lSize, char **d_szArgs, MTH_tdxReal *d_xOut )
{
	MTH_tdxReal xTmp;
	int i;

	for ( i = 0; i < lSize; i++ )
	{
		char *szArg = d_szArgs[i];
		BOOL bRelative = FALSE;

		/* relative coordinates */
		if ( *szArg == '^' )
		{
			bRelative = TRUE;
			szArg++;
		}

		if ( !fn_bParseReal(szArg, &xTmp) )
			break;

		d_xOut[i] = (bRelative)
			? xTmp + d_xOut[i]
			: xTmp;
	}

	return i;
}

BOOL fn_bParsePtr( char *szArg, void **p_pOut )
{
	char *pEnd;
	unsigned long ulTmp = strtoul(szArg, &pEnd, 16);

	if ( *pEnd != 0 )
		return FALSE;

	*p_pOut = (void *)ulTmp;
	return TRUE;
}

BOOL fn_bParseObjectRef( char *szArg, HIE_tdstSuperObject **p_pstOut )
{
	void *pTmp;
	HIE_tdstSuperObject *pSuperObj = NULL;

	if ( fn_bParsePtr(szArg, &pTmp) )
	{
		HIE_tdstSuperObject *pRoot = *GAM_g_p_stDynamicWorld;
		HIE_tdstSuperObject *pChild;
		LST_M_DynamicForEach(pRoot, pChild)
		{
			if ( pTmp == pChild )
			{
				pSuperObj = pTmp;
				break;
			}
		}
	}
	else
	{
		pSuperObj = HIE_fn_p_stFindObjectByName(szArg);
	}

	if ( !pSuperObj )
		return FALSE;

	*p_pstOut = pSuperObj;
	return TRUE;
}
