#include "framework.h"
#include "r2console_api.h"
#include "console.h"
#include "gfx/graphics.h"
#include "gfx/font.h"
#include "gfx/cursor.h"
#include "r2ptr.h"
#include "cvars.h"
#include "ext/ghost.h"
#include "ext/freeze.h"
#include "pick.h"


char const g_szVersion[] = "R2Console v1.4 (" __DATE__ " " __TIME__ ")";

BOOL g_bIsInit = FALSE;
BOOL g_bShow = FALSE;
BOOL g_bTinyMode = FALSE;

MTH2D_tdstVector const g_stHiddenPos = { 0.0f, -M_FontToPercentY((C_LinesOnScreen + 1) * C_Font_xCharHeight) };
MTH2D_tdstVector const g_stHiddenPos_Tiny = { 0.0f, -M_FontToPercentY((C_LinesOnScreenTiny + 1) * C_Font_xCharHeight) };
MTH2D_tdstVector const g_stVisiblePos = { 0.0f, 0.0f };

MTH2D_tdstVector const g_stSize = { 100.0f, M_FontToPercentY((C_LinesOnScreen + 1) * C_Font_xCharHeight) };
MTH2D_tdstVector const g_stSize_Tiny = { 100.0f, M_FontToPercentY((C_LinesOnScreenTiny + 1) * C_Font_xCharHeight) };

MTH2D_tdstVector g_stCurrentPos = { 0.0f, 0.0f };
MTH2D_tdstVector const *g_p_stHiddenPos = &g_stHiddenPos;
MTH2D_tdstVector const *g_p_stSize = &g_stSize;
int g_lLinesOnScreen = C_LinesOnScreen;

int g_lAnimFrames = C_AnimFrames;
int g_lFrameCounter = 0;
int g_lCaretFrame = 0;


tdstLine g_a_stLines[C_NbLines] = { 0 };
unsigned long g_ulOnScreenOffset = 0;


/* prompt */
#define C_MaxPromptChars (C_MaxLine-3)
char g_szPrompt[C_MaxLine] = "";
unsigned long g_ulNbChars = 0;

/* caret */
#define C_CaretFrames 15
unsigned long g_ulCaretPos = 0;
BOOL g_bReplaceMode = FALSE;
void fn_vMoveCaret( char cDirection );
void fn_vPutCaretAtEnd( void );

/* history */
#define C_NbHistory 30
char g_a_szCmdHistory[C_NbHistory][C_MaxLine] = { 0 };
long g_lHistoryIdx = -1;

long g_lLastCommandId = -1;
void *g_pvLastCommandData = NULL;

BOOL g_bForceThisCommand = FALSE;

/* console vars */
tdstCVar *CON_bPauseGame = NULL;
tdstCVar *CON_bEnterHides = NULL;
tdstCVar *CON_bPerfCmd = NULL;
tdstCVar *CON_bClassicStyle = NULL;

/* highlighted word */
long g_lMouseOverLine = -1;
long g_lMouseOverChar = -1;
tdstHiLite *g_pstMouseOverWord = NULL;


void fn_vAnimOneStep( void )
{
	if ( g_lFrameCounter >= 0 && g_lFrameCounter <= g_lAnimFrames )
	{
		MTH_tdxReal xPercent = (MTH_tdxReal)g_lFrameCounter / (MTH_tdxReal)g_lAnimFrames;
		GFX_fn_vLerp2DVector(&g_stCurrentPos, g_p_stHiddenPos, &g_stVisiblePos, xPercent);

		g_lFrameCounter += g_bShow ? 1 : -1;
	}
}

#define C_FrameBottomMargin 1.2f
#define C_FrameSideMargin 6.0f

void fn_vDrawConsoleSprites( void )
{
	GLD_tdstViewportAttributes *p_stVpt = &GAM_g_stEngineStructure->stFixViewportAttr;

	MTH2D_tdstVector stTL = g_stCurrentPos;
	MTH2D_tdstVector stBR = { 0 };
	GFX_fn_vAdd2DVector(&stBR, &g_stCurrentPos, g_p_stSize);

	stBR.x -= 0.001f; /* fullwidth fix for widescreen patch */

	if ( CON_bClassicStyle->bValue ) /* use old frame */
	{
		GFX_fn_vDisplayFrameWithZValue(&stTL, &stBR, C_ClassicTransparency, 1.1f, p_stVpt);
		return;
	}

	/* The console frame:
		________________
		|3 |1       |5 |
		|__|________|__|
		|4_|2_______|6_|

		in 4:3 aspect, only frames 1 and 2 are rendered
		in widescreen, all frames are rendered
	*/

	MTH_tdxReal xSaveZValue = *GLI_g_fZValueForSprite;
	*GLI_g_fZValueForSprite = 1.1f;
	*GLI_g_bForceAAAColor = 0;

	unsigned int a4_ulColors[4];

	/* frame 1 */
	a4_ulColors[0] = a4_ulColors[1] = a4_ulColors[2] = a4_ulColors[3] = M_ulPackRGBAndAlpha(C_FrameColor, C_Transparency);
	GFX_vDraw2DGradientWithPercent(p_stVpt, stTL.x, stTL.y, stBR.x, stBR.y, a4_ulColors);

	/* frame 2 */
	a4_ulColors[0] = a4_ulColors[1] = M_ulPackRGBAndAlpha(C_FrameColor, C_Transparency);
	a4_ulColors[2] = a4_ulColors[3] = M_ulPackRGBAndAlpha(0x000000, 0);
	GFX_vDraw2DGradientWithPercent(p_stVpt, stTL.x, stBR.y, stBR.x, stBR.y + C_FrameBottomMargin, a4_ulColors);

	if ( GLI_FIX_bIsWidescreen() )
	{
		/* frame 3 */
		a4_ulColors[0] = a4_ulColors[3] = M_ulPackRGBAndAlpha(C_FrameColor, C_Transparency);
		a4_ulColors[1] = a4_ulColors[2] = M_ulPackRGBAndAlpha(C_FrameColor, 0);
		GFX_vDraw2DGradientWithPercent(p_stVpt, stTL.x - C_FrameSideMargin, stTL.y, stTL.x, stBR.y, a4_ulColors);

		/* frame 4 */
		a4_ulColors[0] = M_ulPackRGBAndAlpha(C_FrameColor, C_Transparency);
		a4_ulColors[1] = M_ulPackRGBAndAlpha(C_FrameColor, 0);
		a4_ulColors[2] = a4_ulColors[3] = M_ulPackRGBAndAlpha(0x000000, 0);
		GFX_vDraw2DGradientWithPercent(p_stVpt, stTL.x - C_FrameSideMargin, stBR.y, stTL.x, stBR.y + C_FrameBottomMargin, a4_ulColors);

		/* frame 5 */
		a4_ulColors[0] = a4_ulColors[3] = M_ulPackRGBAndAlpha(C_FrameColor, 0);
		a4_ulColors[1] = a4_ulColors[2] = M_ulPackRGBAndAlpha(C_FrameColor, C_Transparency);
		GFX_vDraw2DGradientWithPercent(p_stVpt, stBR.x, stTL.y, stBR.x + C_FrameSideMargin, stBR.y, a4_ulColors);

		/* frame 6 */
		a4_ulColors[0] = M_ulPackRGBAndAlpha(C_FrameColor, 0);
		a4_ulColors[1] = M_ulPackRGBAndAlpha(C_FrameColor, C_Transparency);
		a4_ulColors[2] = a4_ulColors[3] = M_ulPackRGBAndAlpha(0x000000, 0);
		GFX_vDraw2DGradientWithPercent(p_stVpt, stBR.x, stBR.y, stBR.x + C_FrameSideMargin, stBR.y + C_FrameBottomMargin, a4_ulColors);
	}

	*GLI_g_fZValueForSprite = xSaveZValue;
}

tdstHiLite * fn_p_stHiLiteFindWord( void )
{
	if ( g_lMouseOverLine < 0 || g_lMouseOverChar < 0 )
		return NULL;

	tdstLine *pLine = &g_a_stLines[g_lMouseOverLine];

	for ( int i = 0; i < C_MaxHiLite; ++i )
	{
		tdstHiLite *pHiLite = &pLine->stHiLite[i];
		if ( pHiLite->cTo > pHiLite->cFrom
			&& g_lMouseOverChar >= pHiLite->cFrom && g_lMouseOverChar < pHiLite->cTo )
		{
			return pHiLite;
		}
	}

	return NULL;
}

void fn_vDrawConsole( void )
{
	MTH_tdxReal x, y;

	if ( g_lFrameCounter <= 0 )
		return;

	fn_vAnimOneStep();
	fn_vDrawConsoleSprites();

	*GLI_g_fZValueForSprite = 1.2f;
	
	x = M_PercentToFontX(g_stCurrentPos.x);
	y = M_PercentToFontY(g_stCurrentPos.y);

	/* console history */
	for ( int i = g_lLinesOnScreen + g_ulOnScreenOffset - 1; i >= (int)g_ulOnScreenOffset; --i )
	{
		char *szText = g_a_stLines[i].szText;
		char cColor = FNT_M_lColorToChar(g_a_stLines[i].ucColor);
		char szTmp[C_MaxLine+2];

		if ( i == g_lMouseOverLine )
		{
			tdstHiLite *pHiLite = g_pstMouseOverWord;
			if ( pHiLite )
			{
				char *pTmp = szTmp;

				strncpy(pTmp, szText, pHiLite->cFrom);
				pTmp += pHiLite->cFrom;
				*pTmp++ = FNT_M_lColorToChar(3);

				int lInnerLength = pHiLite->cTo - pHiLite->cFrom;
				strncpy(pTmp, szText + pHiLite->cFrom, lInnerLength);
				pTmp += lInnerLength;
				*pTmp++ = cColor;

				strcpy(pTmp, szText + pHiLite->cTo);
				szText = szTmp;
			}
		}

		g_a_stLines[i].cPrefix
			? FNT_fn_vDisplayStringFmt(x, y, "%c%c%s", cColor, g_a_stLines[i].cPrefix, szText)
			: FNT_fn_vDisplayStringFmt(x, y, "%c%s", cColor, szText);

		y += C_Font_xCharHeight;

	}
	//y += C_Font_xCharHeight;

	/* prompt */
	FNT_fn_vDisplayStringFmt(x, y, "\021]%s", g_szPrompt);
	//FNT_fn_vDisplayStringFmt(x, y, "%d;%d \021]%s", g_lMouseOverLine, g_lMouseOverChar, g_szPrompt);

	*GLI_g_fZValueForSprite = 1.22f;

	MTH_tdxReal xCaretOffset = C_Font_xActualCharWidth * (float)(g_ulCaretPos + 1);
	g_lCaretFrame = (g_lCaretFrame + 1) % (C_CaretFrames * 2);

	if ( g_lCaretFrame < C_CaretFrames )
		FNT_fn_vDisplayString(x + xCaretOffset, y, (g_bReplaceMode ? "\022_" : "_"));
	
	if ( g_stAC.ucState > 0 )
		FNT_fn_vDisplayString(x + (C_Font_xActualCharWidth * (float)(g_stAC.ulPos + 1)), y+2, "\023_");

	/* scrollbar */
	MTH_tdxReal xScrollBarHeight = M_PercentToFontY(g_p_stSize->y) - (C_Font_xCharHeight * 2);
	long lScrollBarSteps = C_NbLines - g_lLinesOnScreen;
	MTH_tdxReal xScrollBarPos = (1 - (float)g_ulOnScreenOffset / (float)lScrollBarSteps) * xScrollBarHeight;
	
	x = M_PercentToFontX(g_stCurrentPos.x + g_p_stSize->x) - C_Font_xCharWidth;
	y = M_PercentToFontY(g_stCurrentPos.y) + xScrollBarPos;

	FNT_fn_vDisplayString(x, y, "\021[");

	*GLI_g_fZValueForSprite = 0.998f;
}

void fn_vShowConsole( void )
{
	static DWORD s_ulSaveNbEntry = 0;

	if ( !g_bIsInit )
		fn_vInitConsole();

	g_bShow = !g_bShow;
	g_lFrameCounter += g_bShow ? 1 : -1;

	if ( g_bShow )
	{
		s_ulSaveNbEntry = IPT_g_stInputStructure->ulNumberOfEntryElement;
		IPT_g_stInputStructure->ulNumberOfEntryElement = 0;

		if ( CON_bPauseGame->bValue )
			GAM_g_stEngineStructure->bEngineIsInPaused = TRUE;

		if ( g_bTinyMode )
		{
			g_p_stSize = &g_stSize_Tiny;
			g_p_stHiddenPos = &g_stHiddenPos_Tiny;
			g_lLinesOnScreen = C_LinesOnScreenTiny;
			g_lAnimFrames = C_AnimFramesTiny;
			if ( g_lFrameCounter > C_AnimFramesTiny )
				g_lFrameCounter = C_AnimFramesTiny;
		}
		else
		{
			g_p_stSize = &g_stSize;
			g_p_stHiddenPos = &g_stHiddenPos;
			g_lLinesOnScreen = C_LinesOnScreen;
			g_lAnimFrames = C_AnimFrames;
		}
	}
	else
	{
		IPT_g_stInputStructure->ulNumberOfEntryElement = s_ulSaveNbEntry;

		if ( CON_bPauseGame->bValue )
			GAM_g_stEngineStructure->bEngineIsInPaused = FALSE;
	}

#ifdef USE_PICK
	fn_vResetPickedMenu();
#endif
}

void fn_vPushHistory( char *szString )
{
	// Shift history by 1
	memmove(&g_a_szCmdHistory[1], &g_a_szCmdHistory[0], (C_NbHistory-1)*C_MaxLine);
	strcpy(g_a_szCmdHistory[0], szString);
}

void fn_vPushLineWithHiLite( char *szString, unsigned char ucColor, char cPrefix, tdstHiLite *a_stHiLite )
{
	// Shift lines by 1
	memmove(&g_a_stLines[1], &g_a_stLines[0], (C_NbLines-1)*sizeof(tdstLine));

	g_a_stLines[0].ucColor = ucColor;
	g_a_stLines[0].cPrefix = cPrefix;
	strcpy(g_a_stLines[0].szText, szString);

	if ( a_stHiLite )
		memcpy(g_a_stLines[0].stHiLite, a_stHiLite, C_MaxHiLite*sizeof(tdstHiLite));
	else
		ZeroMemory(g_a_stLines[0].stHiLite, C_MaxHiLite*sizeof(tdstHiLite));

	// Scroll to newest line
	g_ulOnScreenOffset = 0;
}

void fn_vPushLine( char *szString, unsigned char ucColor, char cPrefix )
{
	fn_vPushLineWithHiLite(szString, ucColor, cPrefix, NULL);
}

void fn_vPrintEx( char const *szString, unsigned char ucColor, char cPrefix )
{
	char szBuffer[C_MaxLine];
	char const *pChar = szString;
	int nChars = 0;
	tdstHiLite stHiLite[C_MaxHiLite] = { 0 };
	tdstHiLite *pHiLite = stHiLite;

	do
	{
		if ( pHiLite < &stHiLite[C_MaxHiLite] )
		{
			if ( *pChar == C_cHiLiteBegin )
			{
				pHiLite->cFrom = (char)nChars;
				continue;
			}
			if ( *pChar == C_cHiLiteEnd )
			{
				pHiLite->cTo = (char)nChars;
				pHiLite++;
				continue;
			}
		}

		szBuffer[nChars] = (*pChar == '\n') ? ' ' : *pChar;
		nChars++;

		if ( *pChar == 0 || *pChar == '\n' || nChars == C_MaxLine - 1 )
		{
			szBuffer[nChars] = '\0';
			nChars = 0;

			fn_vPushLineWithHiLite(szBuffer, ucColor, cPrefix, stHiLite);
			ZeroMemory(stHiLite, sizeof(stHiLite));
			pHiLite = stHiLite;
		}

	} while ( *pChar++ );
}

void fn_vPrintCFmt( unsigned char ucColor, char *szFmt, ... )
{
	va_list args;
	va_start(args, szFmt);

	long lSize = fn_lGetFmtStringLength(szFmt, args);
	char *szBuffer = _alloca(lSize);

	if ( szBuffer )
	{
		vsprintf(szBuffer, szFmt, args);
		fn_vPrintEx(szBuffer, ucColor, 0);
	}

	va_end(args);
}

void fn_vPrintC( unsigned char ucColor, char const *szString )
{
	fn_vPrintEx(szString, ucColor, 0);
}

void fn_vPrint( char const *szString )
{
	fn_vPrintEx(szString, 0, 0);
}

void fn_vParseCommand( char *szString )
{
	char szCommand[C_MaxCmdName];
	char szArgs[C_MaxLine];
	char **d_szArgs = NULL;

	BOOL bPerfCmd = CON_bPerfCmd->bValue;

	szString += strspn(szString, " ");
	int length = strcspn(szString, " ");

	if ( *szString == '!' )
	{
		g_bForceThisCommand = TRUE;
		szString++; length--;
	}

	if ( length <= 0 || length >= C_MaxCmdName )
	{
		fn_vPrintC(2, "Unknown command");
		return;
	}

	strncpy(szCommand, szString, length);
	szCommand[length] = 0;

	if ( bPerfCmd )
		fn_vResetTimer(e_Timer_PerfCmd);

	for ( int i = 0; i < g_lNbCommands; i++ )
	{
		if ( _stricmp(szCommand, g_a_stCommands[i].szName) == 0 )
		{
			strcpy(szArgs, szString + length);
			int lCount = fn_lSplitArgs(szArgs, &d_szArgs);

			g_a_stCommands[i].p_fnCommand(lCount, d_szArgs);
			g_lLastCommandId = i;

			if ( bPerfCmd )
			{
				float xTimeExec = fn_xGetTimerElapsed(e_Timer_PerfCmd);
				fn_vPrintCFmt(2, "--> (PERF: %.3f ms exec)", xTimeExec);
			}

			free(d_szArgs);
			return;
		}
	}

	fn_vPrintC(2, "Unknown command");
}

tdstAutocomplete g_stAC = { 0 };

void fn_vResetAutocomplete( void )
{
	g_stAC.ucState = 0;
	g_stAC.ulNbMatch = 0;
	g_stAC.ulCurrentMatch = 0;
	g_stAC.p_stCommand = NULL;
}

unsigned long fn_ulParseAutocompleteACmd( char const *szString )
{
	g_stAC.ucWhich = 0;
	fn_vToLower(g_stAC.szToComplete, szString);

	unsigned long ulNbMatch = 0;
	for ( int i = 0; i < g_lNbCommands && ulNbMatch < C_MaxMatches; i++ )
	{
		char const *szCmd = g_a_stCommands[i].szName;
		if ( strstr(szCmd, g_stAC.szToComplete) != szCmd )
			continue;

		g_stAC.a_pMatches[ulNbMatch++] = (void *)&g_a_stCommands[i];
	}
	g_stAC.ulNbMatch = ulNbMatch;

	return ulNbMatch;
}

unsigned long fn_ulParseAutocompleteAArg( char const *szString, char const *szCommand )
{
	tdstCommand *pCmd = NULL;

	for ( int i = 0; i < g_lNbCommands; i++ )
	{
		if ( _stricmp(szCommand, g_a_stCommands[i].szName) != 0 )
			continue;

		pCmd = &g_a_stCommands[i];
		break;
	}
	if ( !pCmd || !pCmd->p_fnComplete )
	{
		g_stAC.ucWhich = 1;
		g_stAC.ulNbMatch = 0;
		return 0;
	}

	unsigned char ucWhich = 0;
	int length = 0;
	do
	{
		szString += length;
		szString += strspn(szString, " !");
		length = strcspn(szString, " ");
		ucWhich++;
	}
	while ( szString[length] != 0 );

	g_stAC.p_stCommand = pCmd;
	g_stAC.ucWhich = ucWhich;
	fn_vToLower(g_stAC.szToComplete, szString);
	g_stAC.ulPos -= length;

	unsigned long ulNbMatch = pCmd->p_fnComplete(&g_stAC);
	g_stAC.ulNbMatch = ulNbMatch;
	return ulNbMatch;
}
 
unsigned long fn_ulParseAutocomplete( char const *szInitPrompt, unsigned long ulInitCaret )
{
	//char szToComplete[C_MaxCmdName];
	//char szArgs[C_MaxLine];

	strncpy(g_stAC.szLine, szInitPrompt, ulInitCaret);
	g_stAC.szLine[ulInitCaret] = 0;
	g_stAC.ulPos = ulInitCaret;
	g_stAC.ulCurrentMatch = 0;

	char const *szString = g_stAC.szLine;

	szString += strspn(szString, " !");
	int length = strcspn(szString, " ");

	if ( length <= 0 || length >= C_MaxCmdName )
		return 0;

	if ( szString[length] == 0 ) // matching command
		return fn_ulParseAutocompleteACmd(szString);

	// otherwise, matching args
	char szCommand[C_MaxCmdName];
	strncpy(szCommand, szString, length);
	szCommand[length] = 0;
	szString += length;

	return fn_ulParseAutocompleteAArg(szString, szCommand);
}

void fn_vInsertMatch( unsigned long ulIdx )
{
	if ( !g_stAC.ulNbMatch )
		return;

	unsigned long ulInsertAt = g_stAC.ulPos;
	ulIdx = ulIdx % g_stAC.ulNbMatch;

	if ( g_stAC.ucWhich == 0 ) // command
	{
		tdstCommand *pMatch = (tdstCommand*)g_stAC.a_pMatches[ulIdx];
		char const *szCmd = pMatch->szName;
		
		size_t skip = strlen(g_stAC.szToComplete);
		size_t length = strlen(szCmd) - skip;

		if ( ulInsertAt + length <= C_MaxPromptChars )
			strcpy(g_szPrompt + ulInsertAt, szCmd + skip);
	}
	else // args
	{
		char const *szArg = g_stAC.a_szMatches[ulIdx];
		size_t length = strlen(szArg);
		if ( ulInsertAt + length <= C_MaxPromptChars )
			strcpy(g_szPrompt + ulInsertAt, szArg);
	}

	g_ulNbChars = g_ulCaretPos = strlen(g_szPrompt);
}

void fn_vDoAutocomplete( BOOL bReverseDir )
{
	unsigned long ulMatch;

	switch ( g_stAC.ucState )
	{
	case 0:
		fn_ulParseAutocomplete(g_szPrompt, g_ulCaretPos);
		fn_vInsertMatch(g_stAC.ulCurrentMatch);
		g_stAC.ucState = 1;
		break;

	case 1:
		ulMatch = g_stAC.ulCurrentMatch + (bReverseDir ? -1 : 1);
		g_stAC.ulCurrentMatch = (ulMatch + g_stAC.ulNbMatch) % g_stAC.ulNbMatch;
		fn_vInsertMatch(g_stAC.ulCurrentMatch);
		break;
	}
}

void fn_vCancelAutocomplete( void )
{
	if ( g_stAC.ulNbMatch )
	{
		if ( g_stAC.ucWhich == 0 ) // command
			g_szPrompt[g_stAC.ulPos] = 0;
		else // args
			strcpy(g_szPrompt + g_stAC.ulPos, g_stAC.szToComplete);

		g_ulNbChars = g_ulCaretPos = strlen(g_szPrompt);
	}

	fn_vResetAutocomplete();
}

void fn_vResetPrompt( void )
{
	g_szPrompt[0] = '\0';
	g_ulNbChars = 0;
	g_ulCaretPos = 0;

	g_lHistoryIdx = -1;
}

void fn_vSetPromptFromHistory( int lIdx )
{
	char *szNewPrompt = g_a_szCmdHistory[lIdx];

	if ( lIdx < 0 )
	{
		fn_vResetPrompt();
		return;
	}

	if ( lIdx >= C_NbHistory || !szNewPrompt[0] )
		return;

	strcpy(g_szPrompt, szNewPrompt);
	g_ulNbChars = strlen(szNewPrompt);
	fn_vPutCaretAtEnd();

	g_lHistoryIdx = lIdx;
}

void fn_vScrollConsole( long lLines )
{
	long lScreenOffset = (long)g_ulOnScreenOffset;
	if ( lLines > 0 ) // scrolling up
	{
		// upper bound of the buffer
		if ( lScreenOffset + lLines > (C_NbLines - g_lLinesOnScreen) )
			lLines = (C_NbLines - g_lLinesOnScreen) - lScreenOffset;

		// if the line we're trying to scroll to is empty, find next non-empty line
		while ( lLines > 0 && g_a_stLines[lScreenOffset + lLines + (g_lLinesOnScreen - 1)].szText[0] == 0 )
			lLines--;
	}
	else if ( lLines < 0 ) // scrolling down
	{
		// lower bound of the buffer
		if ( (long)lScreenOffset + lLines < 0 )
			lLines = -(long)lScreenOffset;
	}

	g_ulOnScreenOffset += lLines;
}

void fn_vResetScroll( void )
{
	g_ulOnScreenOffset = 0;
}

void fn_vMoveCaret( char cDirection )
{
	if ( cDirection > 0 && g_ulCaretPos < g_ulNbChars ) /* right */
	{
		g_ulCaretPos++;
	}
	else if ( cDirection < 0 && g_ulCaretPos > 0 ) /* left */
	{
		g_ulCaretPos--;
	}
}

void fn_vMoveCaretByWord( char cDirection )
{
	if ( cDirection > 0 ) /* right */
	{
		while ( g_ulCaretPos < g_ulNbChars )
		{
			if ( g_szPrompt[g_ulCaretPos] == ' ' )
			{
				g_ulCaretPos += strspn(&g_szPrompt[g_ulCaretPos], " ");
				return;
			}

			g_ulCaretPos++;
		}
	}
	else if ( cDirection < 0 ) /* left */
	{
		while ( g_ulCaretPos > 0 )
		{
			g_ulCaretPos--;

			if ( g_szPrompt[g_ulCaretPos] != ' ' )
			{
				g_ulCaretPos -= fn_vNotCharCountReverse(&g_szPrompt[g_ulCaretPos], ' ', (int)g_ulCaretPos+1) - 1;
				return;
			}
		}
	}
}

void fn_vPutCaretAtEnd( void )
{
	g_ulCaretPos = g_ulNbChars;
}

void fn_vInsertCharAtCaret( char ch )
{
	char *pAtCaret = &g_szPrompt[g_ulCaretPos];

	if ( g_ulCaretPos < g_ulNbChars )
	{
		int lCharsToMove = (int)g_ulNbChars - (int)g_ulCaretPos + 1;
		memmove(pAtCaret + 1, pAtCaret, lCharsToMove);
		*pAtCaret = ch;
	}
	else
	{
		pAtCaret[0] = ch;
		pAtCaret[1] = 0;
	}

	g_ulNbChars++;
	g_ulCaretPos++;
}

void fn_vReplaceCharAtCaret( char ch )
{
	char *pAtCaret = &g_szPrompt[g_ulCaretPos];

	if ( g_ulCaretPos < g_ulNbChars )
	{
		*pAtCaret = ch;
	}
	else
	{
		pAtCaret[0] = ch;
		pAtCaret[1] = 0;
		g_ulNbChars++;
	}

	g_ulCaretPos++;
}

void fn_vBackspaceCharAtCaret( void )
{
	char *pAtCaret = &g_szPrompt[g_ulCaretPos];

	if ( g_ulCaretPos == 0 )
		return;

	if ( g_ulCaretPos < g_ulNbChars )
	{
		int lCharsToMove = (int)g_ulNbChars - (int)g_ulCaretPos + 1;
		memmove(pAtCaret - 1, pAtCaret, lCharsToMove);
	}
	else
	{
		pAtCaret[-1] = 0;
	}

	g_ulNbChars--;
	g_ulCaretPos--;
}

BOOL fn_bProcessKey( DWORD dwKeyCode )
{
	if ( dwKeyCode == VK_OEM_3 )
	{
		g_bTinyMode = GetKeyState(VK_SHIFT) & 0x8000 ? TRUE : FALSE;
		fn_vShowConsole();
		return TRUE;
	}

	if ( !g_bShow )
		return FALSE;

	switch ( dwKeyCode )
	{
	case VK_RETURN:
		if ( g_ulNbChars > 0 )
		{
			fn_vPushLine(g_szPrompt, 1, ']');
			fn_vPushHistory(g_szPrompt);
			fn_vParseCommand(g_szPrompt);
			fn_vResetPrompt();
			fn_vResetScroll();
			g_bForceThisCommand = FALSE;
		}
		else if ( CON_bEnterHides->bValue )
		{
			fn_vShowConsole();
		}
		break;

	case VK_BACK:
		fn_vBackspaceCharAtCaret();
		break;

	case VK_UP:
		fn_vSetPromptFromHistory(g_lHistoryIdx + 1);
		break;

	case VK_DOWN:
		fn_vSetPromptFromHistory(g_lHistoryIdx - 1);
		break;

	case VK_LEFT:
		(GetKeyState(VK_CONTROL) & 0x8000)
			? fn_vMoveCaretByWord(-1)
			: fn_vMoveCaret(-1);
		break;

	case VK_RIGHT:
		(GetKeyState(VK_CONTROL) & 0x8000)
			? fn_vMoveCaretByWord(1)
			: fn_vMoveCaret(1);
		break;

	case VK_HOME:
		g_ulCaretPos = 0;
		break;

	case VK_END:
		fn_vPutCaretAtEnd();
		break;

	case VK_PRIOR:
		fn_vScrollConsole(1);
		break;

	case VK_NEXT:
		fn_vScrollConsole(-1);
		break;

	case VK_INSERT:
		g_bReplaceMode = !g_bReplaceMode;
		break;

	case VK_ESCAPE:
		if ( g_stAC.ucState > 0 )
			fn_vCancelAutocomplete();
		else
			fn_vShowConsole();
		break;

	case VK_TAB:
		fn_vDoAutocomplete(GetKeyState(VK_SHIFT) & 0x8000);
		return TRUE;

	default:
		return FALSE;
	}

	fn_vResetAutocomplete();
	return TRUE;
}

BOOL fn_bProcessChar( DWORD dwChar )
{
	if ( !g_bShow )
		return FALSE;

	if ( dwChar < ' ' || dwChar == '`' || dwChar == '~' )
		return FALSE;

	if ( g_ulNbChars >= C_MaxPromptChars )
		return FALSE;

	g_bReplaceMode
		? fn_vReplaceCharAtCaret((char)dwChar)
		: fn_vInsertCharAtCaret((char)dwChar);

	fn_vResetAutocomplete();

	return TRUE;
}

void fn_vInitVars( void )
{
	CON_bPauseGame = fn_p_stCreateCVar("Con_PauseGame", E_CVarBool);
	CON_bPauseGame->bValue = FALSE;

	CON_bEnterHides = fn_p_stCreateCVar("Con_EnterHides", E_CVarBool);
	CON_bEnterHides->bValue = FALSE;

	CON_bPerfCmd = fn_p_stCreateCVar("Con_PerfCmd", E_CVarBool);
	CON_bPerfCmd->bValue = FALSE;

	CON_bClassicStyle = fn_p_stCreateCVar("Con_Classic", E_CVarBool);
	CON_bClassicStyle->bValue = FALSE;
}

void fn_vEarlyInitConsole( void )
{
	fn_vInitTimer();

	TXM_fn_vInit();

	FNT_fn_vLoadFontTexture();
	CUR_fn_vLoadCursorTexture();
}

void fn_vInitConsole( void )
{
	fn_bInitWidescreenSupport();
	FNT_fn_vFontInit();

	fn_vInitVars();
	fn_vInitCommands();

	fn_vPrint(g_szVersion);
	fn_vPrint("Type \"help\" for available commands");

	g_bIsInit = TRUE;
}

void MOD_AGO_vDisplayGAUGES( GLD_tdstViewportAttributes *p_stVpt )
{
	AGO_vDisplayGAUGES(p_stVpt);

	if ( g_bShow )
	{
		g_pstMouseOverWord = fn_p_stHiLiteFindWord();
		BOOL bAltCursor = (
			g_pstMouseOverWord != NULL
#ifdef USE_PICK
			|| g_lPickMenuSel > -1
#endif
			);
		CUR_fn_vSetCursorCxt(bAltCursor);
		CUR_fn_vDrawCursor();

#ifdef USE_PICK
		fn_vDrawPickedMenu();
#endif
	}

	fn_vDrawConsole();
}

extern unsigned char g_ucGhostModeCameraWorkaround;

void MOD_fn_vEngine( void )
{
	static unsigned char s_ucSaveGhostMode;

	if ( g_ucGhostModeCameraWorkaround )
	{
		switch ( g_ucGhostModeCameraWorkaround-- )
		{
			case 2:
				s_ucSaveGhostMode = *GAM_g_ucIsEdInGhostMode;
				*GAM_g_ucIsEdInGhostMode = 1;
				break;

			case 1:
				*GAM_g_ucIsEdInGhostMode = s_ucSaveGhostMode;
				break;
		}
	}

	/* noclip */
	GST_fn_vDoGhostMode();

	GAM_fn_vEngine();

	/* freeze / one-step */
	FRZ_fn_vDoEngineFreeze();
}

void fn_vPasteAtCaret( char *szStr, int lStrLen )
{
	int lFreeSpace = C_MaxPromptChars - strlen(g_szPrompt);
	if ( lStrLen > lFreeSpace )
		lStrLen = lFreeSpace;

	/* move rest of the prompt */
	char *pAtCaret = &g_szPrompt[g_ulCaretPos];
	int lRemaining = strlen(pAtCaret) + 1;
	memmove(pAtCaret+lStrLen, pAtCaret, lRemaining);

	/* insert the string at caret */
	strncpy(pAtCaret, szStr, lStrLen);
	g_ulNbChars = strlen(g_szPrompt);
	g_ulCaretPos += lStrLen;
}

void fn_vHiLiteHitTest( MTH2D_tdstVector *p_stPos )
{
	MTH_tdxReal xPosX = p_stPos->x - g_stCurrentPos.x;
	MTH_tdxReal xPosY = p_stPos->y - g_stCurrentPos.y;

	if ( xPosX > 0 && xPosX < g_p_stSize->x
		&& xPosY > 0 && p_stPos->y < g_p_stSize->y )
	{
		int lLineFromTop = (int)(xPosY / M_FontToPercentY(C_Font_xCharHeight)) + 1;

		g_lMouseOverLine = g_lLinesOnScreen + g_ulOnScreenOffset - lLineFromTop;
		g_lMouseOverChar = (int)(xPosX / M_FontToPercentX(C_Font_xActualCharWidth));
	}
	else
	{
		g_lMouseOverLine = -1;
		g_lMouseOverChar = -1;
	}
}

void fn_vHiLiteSelect( MTH2D_tdstVector *p_stPos )
{
	/* hit test just to be sure */
	fn_vHiLiteHitTest(p_stPos);

	tdstLine *pLine = &g_a_stLines[g_lMouseOverLine];
	tdstHiLite *pHiLite = fn_p_stHiLiteFindWord();
	if ( !pHiLite )
		return;

	int lLength = pHiLite->cTo - pHiLite->cFrom;
	fn_vPasteAtCaret(pLine->szText + pHiLite->cFrom, lLength);
}


LRESULT CALLBACK MOD_WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	static MTH2D_tdstVector stPos = { 0 };

	if ( uMsg == WM_KEYDOWN )
	{
		if ( fn_bProcessKey(wParam) )
			return 0;
	}
	else if ( uMsg == WM_CHAR )
	{
		if ( fn_bProcessChar(wParam) )
			return 0;
	}

	if ( !g_bShow )
		return GAM_fn_WndProc(hWnd, uMsg, wParam, lParam);

	if ( uMsg == WM_LBUTTONDOWN )
	{
		fn_vMouseCoordToPercent(&stPos, lParam, hWnd);
		fn_vHiLiteSelect(&stPos);
#ifdef USE_PICK
		fn_vPickMenuSelect(&stPos);
#endif
		return 0;
	}
	else if ( uMsg == WM_RBUTTONDOWN )
	{
		fn_vMouseCoordToPercent(&stPos, lParam, hWnd);
#ifdef USE_PICK
		fn_vPickObjectFromWorld(LOWORD(lParam), HIWORD(lParam), &stPos);
#endif
#ifdef USE_WATCH
		WAT_fn_vHitTestClose(&stPos);
#endif
		return 0;
	}
	else if ( uMsg == WM_MOUSEMOVE )
	{
		fn_vMouseCoordToPercent(&stPos, lParam, hWnd);
		CUR_fn_vMoveCursor(&stPos);
		fn_vHiLiteHitTest(&stPos);
#ifdef USE_PICK
		fn_vPickMenuHitTest(&stPos);
#endif
#ifdef USE_WATCH
		WAT_fn_vHitTestMove(&stPos, (wParam & MK_LBUTTON));
#endif
		return 0;
	}
	else if ( uMsg == WM_MOUSEWHEEL )
	{
		short wLines = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;

		stPos.x -= g_stCurrentPos.x;
		stPos.y -= g_stCurrentPos.y;

		// hit test
		if ( stPos.x > 0 && stPos.x < g_p_stSize->x
			&& stPos.y > 0 && stPos.y < g_p_stSize->y )
		{
			fn_vScrollConsole(wLines);
		}
		return 0;
	}

	return GAM_fn_WndProc(hWnd, uMsg, wParam, lParam);
}
