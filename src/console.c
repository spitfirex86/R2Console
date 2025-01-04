#include "framework.h"
#include "console.h"
#include "gfx/graphics.h"
#include "gfx/font.h"
#include "gfx/cursor.h"
#include "r2ptr.h"
#include "cvars.h"
#include "ext/ghost.h"
#include "ext/freeze.h"
#include "pick.h"


char const g_szVersion[] = "R2Console v1.3 (" __DATE__ " " __TIME__ ")";

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
	MTH2D_tdstVector stTL = g_stCurrentPos;
	MTH2D_tdstVector stBR = { 0 };
	GFX_fn_vAdd2DVector(&stBR, &g_stCurrentPos, g_p_stSize);

	stBR.x -= 0.001f; /* fullwidth fix for widescreen patch */

	if ( CON_bClassicStyle->bValue ) /* use old frame */
	{
		GFX_fn_vDisplayFrameWithZValue(&stTL, &stBR, C_ClassicTransparency, 1.1f, &GAM_g_stEngineStructure->stFixViewportAttr);
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
	GFX_vDraw2DGradientWithPercent(&GAM_g_stEngineStructure->stFixViewportAttr, stTL.x, stTL.y, stBR.x, stBR.y, a4_ulColors);

	/* frame 2 */
	a4_ulColors[0] = a4_ulColors[1] = M_ulPackRGBAndAlpha(C_FrameColor, C_Transparency);
	a4_ulColors[2] = a4_ulColors[3] = M_ulPackRGBAndAlpha(0x000000, 0);
	GFX_vDraw2DGradientWithPercent(&GAM_g_stEngineStructure->stFixViewportAttr, stTL.x, stBR.y, stBR.x, stBR.y + C_FrameBottomMargin, a4_ulColors);

	if ( GLI_FIX_bIsWidescreen() )
	{
		/* frame 3 */
		a4_ulColors[0] = a4_ulColors[3] = M_ulPackRGBAndAlpha(C_FrameColor, C_Transparency);
		a4_ulColors[1] = a4_ulColors[2] = M_ulPackRGBAndAlpha(C_FrameColor, 0);
		GFX_vDraw2DGradientWithPercent(&GAM_g_stEngineStructure->stFixViewportAttr, stTL.x - C_FrameSideMargin, stTL.y, stTL.x, stBR.y, a4_ulColors);

		/* frame 4 */
		a4_ulColors[0] = M_ulPackRGBAndAlpha(C_FrameColor, C_Transparency);
		a4_ulColors[1] = M_ulPackRGBAndAlpha(C_FrameColor, 0);
		a4_ulColors[2] = a4_ulColors[3] = M_ulPackRGBAndAlpha(0x000000, 0);
		GFX_vDraw2DGradientWithPercent(&GAM_g_stEngineStructure->stFixViewportAttr, stTL.x - C_FrameSideMargin, stBR.y, stTL.x, stBR.y + C_FrameBottomMargin, a4_ulColors);

		/* frame 5 */
		a4_ulColors[0] = a4_ulColors[3] = M_ulPackRGBAndAlpha(C_FrameColor, 0);
		a4_ulColors[1] = a4_ulColors[2] = M_ulPackRGBAndAlpha(C_FrameColor, C_Transparency);
		GFX_vDraw2DGradientWithPercent(&GAM_g_stEngineStructure->stFixViewportAttr, stBR.x, stTL.y, stBR.x + C_FrameSideMargin, stBR.y, a4_ulColors);

		/* frame 6 */
		a4_ulColors[0] = M_ulPackRGBAndAlpha(C_FrameColor, 0);
		a4_ulColors[1] = M_ulPackRGBAndAlpha(C_FrameColor, C_Transparency);
		a4_ulColors[2] = a4_ulColors[3] = M_ulPackRGBAndAlpha(0x000000, 0);
		GFX_vDraw2DGradientWithPercent(&GAM_g_stEngineStructure->stFixViewportAttr, stBR.x, stBR.y, stBR.x + C_FrameSideMargin, stBR.y + C_FrameBottomMargin, a4_ulColors);
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

			g_a_stCommands[i].p_stCommand(lCount, d_szArgs);
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
		return TRUE;

	case VK_BACK:
		fn_vBackspaceCharAtCaret();
		return TRUE;

	case VK_UP:
		fn_vSetPromptFromHistory(g_lHistoryIdx + 1);
		return TRUE;

	case VK_DOWN:
		fn_vSetPromptFromHistory(g_lHistoryIdx - 1);
		return TRUE;

	case VK_LEFT:
		(GetKeyState(VK_CONTROL) & 0x8000)
			? fn_vMoveCaretByWord(-1)
			: fn_vMoveCaret(-1);
		return TRUE;

	case VK_RIGHT:
		(GetKeyState(VK_CONTROL) & 0x8000)
			? fn_vMoveCaretByWord(1)
			: fn_vMoveCaret(1);
		return TRUE;

	case VK_HOME:
		g_ulCaretPos = 0;
		return TRUE;

	case VK_END:
		fn_vPutCaretAtEnd();
		return TRUE;

	case VK_PRIOR:
		fn_vScrollConsole(1);
		return TRUE;

	case VK_NEXT:
		fn_vScrollConsole(-1);
		return TRUE;

	case VK_INSERT:
		g_bReplaceMode = !g_bReplaceMode;
		return TRUE;

	case VK_ESCAPE:
		fn_vShowConsole();
		return TRUE;
	}

	return FALSE;
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

	fn_vPrint(g_szVersion);
	fn_vPrint("Type \"help\" for available commands");

	g_bIsInit = TRUE;
}

typedef struct tdstHersheyVertex
{
	int x;
	int y;
}
tdstHersheyVertex;

typedef struct tdstHersheyChar
{
	int lNbVertex;
	int lSpacing;
	tdstHersheyVertex aVertices[55];
}
tdstHersheyChar;

int simplex[95][112] = {
	0,16, /* Ascii 32 */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	8,10, /* Ascii 33 */
	5,21, 5, 7,-1,-1, 5, 2, 4, 1, 5, 0, 6, 1, 5, 2,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	5,16, /* Ascii 34 */
	4,21, 4,14,-1,-1,12,21,12,14,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	11,21, /* Ascii 35 */
	11,25, 4,-7,-1,-1,17,25,10,-7,-1,-1, 4,12,18,12,-1,-1, 3, 6,17, 6,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	26,20, /* Ascii 36 */
	8,25, 8,-4,-1,-1,12,25,12,-4,-1,-1,17,18,15,20,12,21, 8,21, 5,20, 3,
	18, 3,16, 4,14, 5,13, 7,12,13,10,15, 9,16, 8,17, 6,17, 3,15, 1,12, 0,
	8, 0, 5, 1, 3, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	31,24, /* Ascii 37 */
	21,21, 3, 0,-1,-1, 8,21,10,19,10,17, 9,15, 7,14, 5,14, 3,16, 3,18, 4,
	20, 6,21, 8,21,10,20,13,19,16,19,19,20,21,21,-1,-1,17, 7,15, 6,14, 4,
	14, 2,16, 0,18, 0,20, 1,21, 3,21, 5,19, 7,17, 7,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	34,26, /* Ascii 38 */
	23,12,23,13,22,14,21,14,20,13,19,11,17, 6,15, 3,13, 1,11, 0, 7, 0, 5,
	1, 4, 2, 3, 4, 3, 6, 4, 8, 5, 9,12,13,13,14,14,16,14,18,13,20,11,21,
	9,20, 8,18, 8,16, 9,13,11,10,16, 3,18, 1,20, 0,22, 0,23, 1,23, 2,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	7,10, /* Ascii 39 */
	5,19, 4,20, 5,21, 6,20, 6,18, 5,16, 4,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	10,14, /* Ascii 40 */
	11,25, 9,23, 7,20, 5,16, 4,11, 4, 7, 5, 2, 7,-2, 9,-5,11,-7,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	10,14, /* Ascii 41 */
	3,25, 5,23, 7,20, 9,16,10,11,10, 7, 9, 2, 7,-2, 5,-5, 3,-7,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	8,16, /* Ascii 42 */
	8,21, 8, 9,-1,-1, 3,18,13,12,-1,-1,13,18, 3,12,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	5,26, /* Ascii 43 */
	13,18,13, 0,-1,-1, 4, 9,22, 9,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	8,10, /* Ascii 44 */
	6, 1, 5, 0, 4, 1, 5, 2, 6, 1, 6,-1, 5,-3, 4,-4,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	2,26, /* Ascii 45 */
	4, 9,22, 9,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	5,10, /* Ascii 46 */
	5, 2, 4, 1, 5, 0, 6, 1, 5, 2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	2,22, /* Ascii 47 */
	20,25, 2,-7,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	17,20, /* Ascii 48 */
	9,21, 6,20, 4,17, 3,12, 3, 9, 4, 4, 6, 1, 9, 0,11, 0,14, 1,16, 4,17,
	9,17,12,16,17,14,20,11,21, 9,21,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	4,20, /* Ascii 49 */
	6,17, 8,18,11,21,11, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	14,20, /* Ascii 50 */
	4,16, 4,17, 5,19, 6,20, 8,21,12,21,14,20,15,19,16,17,16,15,15,13,13,
	10, 3, 0,17, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	15,20, /* Ascii 51 */
	5,21,16,21,10,13,13,13,15,12,16,11,17, 8,17, 6,16, 3,14, 1,11, 0, 8,
	0, 5, 1, 4, 2, 3, 4,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	6,20, /* Ascii 52 */
	13,21, 3, 7,18, 7,-1,-1,13,21,13, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	17,20, /* Ascii 53 */
	15,21, 5,21, 4,12, 5,13, 8,14,11,14,14,13,16,11,17, 8,17, 6,16, 3,14,
	1,11, 0, 8, 0, 5, 1, 4, 2, 3, 4,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	23,20, /* Ascii 54 */
	16,18,15,20,12,21,10,21, 7,20, 5,17, 4,12, 4, 7, 5, 3, 7, 1,10, 0,11,
	0,14, 1,16, 3,17, 6,17, 7,16,10,14,12,11,13,10,13, 7,12, 5,10, 4, 7,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	5,20, /* Ascii 55 */
	17,21, 7, 0,-1,-1, 3,21,17,21,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	29,20, /* Ascii 56 */
	8,21, 5,20, 4,18, 4,16, 5,14, 7,13,11,12,14,11,16, 9,17, 7,17, 4,16,
	2,15, 1,12, 0, 8, 0, 5, 1, 4, 2, 3, 4, 3, 7, 4, 9, 6,11, 9,12,13,13,
	15,14,16,16,16,18,15,20,12,21, 8,21,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	23,20, /* Ascii 57 */
	16,14,15,11,13, 9,10, 8, 9, 8, 6, 9, 4,11, 3,14, 3,15, 4,18, 6,20, 9,
	21,10,21,13,20,15,18,16,14,16, 9,15, 4,13, 1,10, 0, 8, 0, 5, 1, 4, 3,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	11,10, /* Ascii 58 */
	5,14, 4,13, 5,12, 6,13, 5,14,-1,-1, 5, 2, 4, 1, 5, 0, 6, 1, 5, 2,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	14,10, /* Ascii 59 */
	5,14, 4,13, 5,12, 6,13, 5,14,-1,-1, 6, 1, 5, 0, 4, 1, 5, 2, 6, 1, 6,
	-1, 5,-3, 4,-4,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	3,24, /* Ascii 60 */
	20,18, 4, 9,20, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	5,26, /* Ascii 61 */
	4,12,22,12,-1,-1, 4, 6,22, 6,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	3,24, /* Ascii 62 */
	4,18,20, 9, 4, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	20,18, /* Ascii 63 */
	3,16, 3,17, 4,19, 5,20, 7,21,11,21,13,20,14,19,15,17,15,15,14,13,13,
	12, 9,10, 9, 7,-1,-1, 9, 2, 8, 1, 9, 0,10, 1, 9, 2,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	55,27, /* Ascii 64 */
	18,13,17,15,15,16,12,16,10,15, 9,14, 8,11, 8, 8, 9, 6,11, 5,14, 5,16,
	6,17, 8,-1,-1,12,16,10,14, 9,11, 9, 8,10, 6,11, 5,-1,-1,18,16,17, 8,
	17, 6,19, 5,21, 5,23, 7,24,10,24,12,23,15,22,17,20,19,18,20,15,21,12,
	21, 9,20, 7,19, 5,17, 4,15, 3,12, 3, 9, 4, 6, 5, 4, 7, 2, 9, 1,12, 0,
	15, 0,18, 1,20, 2,21, 3,-1,-1,19,16,18, 8,18, 6,19, 5,
	8,18, /* Ascii 65 */ /* A */
	9,21, 1, 0,-1,-1, 9,21,17, 0,-1,-1, 4, 7,14, 7,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	23,21, /* Ascii 66 */
	4,21, 4, 0,-1,-1, 4,21,13,21,16,20,17,19,18,17,18,15,17,13,16,12,13,
	11,-1,-1, 4,11,13,11,16,10,17, 9,18, 7,18, 4,17, 2,16, 1,13, 0, 4, 0,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	18,21, /* Ascii 67 */
	18,16,17,18,15,20,13,21, 9,21, 7,20, 5,18, 4,16, 3,13, 3, 8, 4, 5, 5,
	3, 7, 1, 9, 0,13, 0,15, 1,17, 3,18, 5,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	15,21, /* Ascii 68 */
	4,21, 4, 0,-1,-1, 4,21,11,21,14,20,16,18,17,16,18,13,18, 8,17, 5,16,
	3,14, 1,11, 0, 4, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	11,19, /* Ascii 69 */
	4,21, 4, 0,-1,-1, 4,21,17,21,-1,-1, 4,11,12,11,-1,-1, 4, 0,17, 0,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	8,18, /* Ascii 70 */
	4,21, 4, 0,-1,-1, 4,21,17,21,-1,-1, 4,11,12,11,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	22,21, /* Ascii 71 */
	18,16,17,18,15,20,13,21, 9,21, 7,20, 5,18, 4,16, 3,13, 3, 8, 4, 5, 5,
	3, 7, 1, 9, 0,13, 0,15, 1,17, 3,18, 5,18, 8,-1,-1,13, 8,18, 8,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	8,22, /* Ascii 72 */
	4,21, 4, 0,-1,-1,18,21,18, 0,-1,-1, 4,11,18,11,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	2, 8, /* Ascii 73 */
	4,21, 4, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	10,16, /* Ascii 74 */
	12,21,12, 5,11, 2,10, 1, 8, 0, 6, 0, 4, 1, 3, 2, 2, 5, 2, 7,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	8,21, /* Ascii 75 */
	4,21, 4, 0,-1,-1,18,21, 4, 7,-1,-1, 9,12,18, 0,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	5,17, /* Ascii 76 */
	4,21, 4, 0,-1,-1, 4, 0,16, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	11,24, /* Ascii 77 */
	4,21, 4, 0,-1,-1, 4,21,12, 0,-1,-1,20,21,12, 0,-1,-1,20,21,20, 0,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	8,22, /* Ascii 78 */
	4,21, 4, 0,-1,-1, 4,21,18, 0,-1,-1,18,21,18, 0,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	21,22, /* Ascii 79 */
	9,21, 7,20, 5,18, 4,16, 3,13, 3, 8, 4, 5, 5, 3, 7, 1, 9, 0,13, 0,15,
	1,17, 3,18, 5,19, 8,19,13,18,16,17,18,15,20,13,21, 9,21,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	13,21, /* Ascii 80 */
	4,21, 4, 0,-1,-1, 4,21,13,21,16,20,17,19,18,17,18,14,17,12,16,11,13,
	10, 4,10,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	24,22, /* Ascii 81 */
	9,21, 7,20, 5,18, 4,16, 3,13, 3, 8, 4, 5, 5, 3, 7, 1, 9, 0,13, 0,15,
	1,17, 3,18, 5,19, 8,19,13,18,16,17,18,15,20,13,21, 9,21,-1,-1,12, 4,
	18,-2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	16,21, /* Ascii 82 */
	4,21, 4, 0,-1,-1, 4,21,13,21,16,20,17,19,18,17,18,15,17,13,16,12,13,
	11, 4,11,-1,-1,11,11,18, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	20,20, /* Ascii 83 */
	17,18,15,20,12,21, 8,21, 5,20, 3,18, 3,16, 4,14, 5,13, 7,12,13,10,15,
	9,16, 8,17, 6,17, 3,15, 1,12, 0, 8, 0, 5, 1, 3, 3,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	5,16, /* Ascii 84 */
	8,21, 8, 0,-1,-1, 1,21,15,21,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	10,22, /* Ascii 85 */
	4,21, 4, 6, 5, 3, 7, 1,10, 0,12, 0,15, 1,17, 3,18, 6,18,21,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	5,18, /* Ascii 86 */
	1,21, 9, 0,-1,-1,17,21, 9, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	11,24, /* Ascii 87 */
	2,21, 7, 0,-1,-1,12,21, 7, 0,-1,-1,12,21,17, 0,-1,-1,22,21,17, 0,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	5,20, /* Ascii 88 */
	3,21,17, 0,-1,-1,17,21, 3, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	6,18, /* Ascii 89 */
	1,21, 9,11, 9, 0,-1,-1,17,21, 9,11,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	8,20, /* Ascii 90 */
	17,21, 3, 0,-1,-1, 3,21,17,21,-1,-1, 3, 0,17, 0,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	11,14, /* Ascii 91 */
	4,25, 4,-7,-1,-1, 5,25, 5,-7,-1,-1, 4,25,11,25,-1,-1, 4,-7,11,-7,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	2,14, /* Ascii 92 */
	0,21,14,-3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	11,14, /* Ascii 93 */
	9,25, 9,-7,-1,-1,10,25,10,-7,-1,-1, 3,25,10,25,-1,-1, 3,-7,10,-7,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	10,16, /* Ascii 94 */
	6,15, 8,18,10,15,-1,-1, 3,12, 8,17,13,12,-1,-1, 8,17, 8, 0,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	2,16, /* Ascii 95 */
	0,-2,16,-2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	7,10, /* Ascii 96 */
	6,21, 5,20, 4,18, 4,16, 5,15, 6,16, 5,17,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	17,19, /* Ascii 97 */
	15,14,15, 0,-1,-1,15,11,13,13,11,14, 8,14, 6,13, 4,11, 3, 8, 3, 6, 4,
	3, 6, 1, 8, 0,11, 0,13, 1,15, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	17,19, /* Ascii 98 */
	4,21, 4, 0,-1,-1, 4,11, 6,13, 8,14,11,14,13,13,15,11,16, 8,16, 6,15,
	3,13, 1,11, 0, 8, 0, 6, 1, 4, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	14,18, /* Ascii 99 */
	15,11,13,13,11,14, 8,14, 6,13, 4,11, 3, 8, 3, 6, 4, 3, 6, 1, 8, 0,11,
	0,13, 1,15, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	17,19, /* Ascii 100 */
	15,21,15, 0,-1,-1,15,11,13,13,11,14, 8,14, 6,13, 4,11, 3, 8, 3, 6, 4,
	3, 6, 1, 8, 0,11, 0,13, 1,15, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	17,18, /* Ascii 101 */
	3, 8,15, 8,15,10,14,12,13,13,11,14, 8,14, 6,13, 4,11, 3, 8, 3, 6, 4,
	3, 6, 1, 8, 0,11, 0,13, 1,15, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	8,12, /* Ascii 102 */
	10,21, 8,21, 6,20, 5,17, 5, 0,-1,-1, 2,14, 9,14,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	22,19, /* Ascii 103 */
	15,14,15,-2,14,-5,13,-6,11,-7, 8,-7, 6,-6,-1,-1,15,11,13,13,11,14, 8,
	14, 6,13, 4,11, 3, 8, 3, 6, 4, 3, 6, 1, 8, 0,11, 0,13, 1,15, 3,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	10,19, /* Ascii 104 */
	4,21, 4, 0,-1,-1, 4,10, 7,13, 9,14,12,14,14,13,15,10,15, 0,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	8, 8, /* Ascii 105 */
	3,21, 4,20, 5,21, 4,22, 3,21,-1,-1, 4,14, 4, 0,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	11,10, /* Ascii 106 */
	5,21, 6,20, 7,21, 6,22, 5,21,-1,-1, 6,14, 6,-3, 5,-6, 3,-7, 1,-7,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	8,17, /* Ascii 107 */
	4,21, 4, 0,-1,-1,14,14, 4, 4,-1,-1, 8, 8,15, 0,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	2, 8, /* Ascii 108 */
	4,21, 4, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	18,30, /* Ascii 109 */
	4,14, 4, 0,-1,-1, 4,10, 7,13, 9,14,12,14,14,13,15,10,15, 0,-1,-1,15,
	10,18,13,20,14,23,14,25,13,26,10,26, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	10,19, /* Ascii 110 */
	4,14, 4, 0,-1,-1, 4,10, 7,13, 9,14,12,14,14,13,15,10,15, 0,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	17,19, /* Ascii 111 */
	8,14, 6,13, 4,11, 3, 8, 3, 6, 4, 3, 6, 1, 8, 0,11, 0,13, 1,15, 3,16,
	6,16, 8,15,11,13,13,11,14, 8,14,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	17,19, /* Ascii 112 */
	4,14, 4,-7,-1,-1, 4,11, 6,13, 8,14,11,14,13,13,15,11,16, 8,16, 6,15,
	3,13, 1,11, 0, 8, 0, 6, 1, 4, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	17,19, /* Ascii 113 */
	15,14,15,-7,-1,-1,15,11,13,13,11,14, 8,14, 6,13, 4,11, 3, 8, 3, 6, 4,
	3, 6, 1, 8, 0,11, 0,13, 1,15, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	8,13, /* Ascii 114 */
	4,14, 4, 0,-1,-1, 4, 8, 5,11, 7,13, 9,14,12,14,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	17,17, /* Ascii 115 */
	14,11,13,13,10,14, 7,14, 4,13, 3,11, 4, 9, 6, 8,11, 7,13, 6,14, 4,14,
	3,13, 1,10, 0, 7, 0, 4, 1, 3, 3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	8,12, /* Ascii 116 */
	5,21, 5, 4, 6, 1, 8, 0,10, 0,-1,-1, 2,14, 9,14,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	10,19, /* Ascii 117 */
	4,14, 4, 4, 5, 1, 7, 0,10, 0,12, 1,15, 4,-1,-1,15,14,15, 0,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	5,16, /* Ascii 118 */
	2,14, 8, 0,-1,-1,14,14, 8, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	11,22, /* Ascii 119 */
	3,14, 7, 0,-1,-1,11,14, 7, 0,-1,-1,11,14,15, 0,-1,-1,19,14,15, 0,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	5,17, /* Ascii 120 */
	3,14,14, 0,-1,-1,14,14, 3, 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	9,16, /* Ascii 121 */
	2,14, 8, 0,-1,-1,14,14, 8, 0, 6,-4, 4,-6, 2,-7, 1,-7,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	8,17, /* Ascii 122 */
	14,14, 3, 0,-1,-1, 3,14,14,14,-1,-1, 3, 0,14, 0,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	39,14, /* Ascii 123 */
	9,25, 7,24, 6,23, 5,21, 5,19, 6,17, 7,16, 8,14, 8,12, 6,10,-1,-1, 7,
	24, 6,22, 6,20, 7,18, 8,17, 9,15, 9,13, 8,11, 4, 9, 8, 7, 9, 5, 9, 3,
	8, 1, 7, 0, 6,-2, 6,-4, 7,-6,-1,-1, 6, 8, 8, 6, 8, 4, 7, 2, 6, 1, 5,
	-1, 5,-3, 6,-5, 7,-6, 9,-7,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	2, 8, /* Ascii 124 */
	4,25, 4,-7,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	39,14, /* Ascii 125 */
	5,25, 7,24, 8,23, 9,21, 9,19, 8,17, 7,16, 6,14, 6,12, 8,10,-1,-1, 7,
	24, 8,22, 8,20, 7,18, 6,17, 5,15, 5,13, 6,11,10, 9, 6, 7, 5, 5, 5, 3,
	6, 1, 7, 0, 8,-2, 8,-4, 7,-6,-1,-1, 8, 8, 6, 6, 6, 4, 7, 2, 8, 1, 9,
	-1, 9,-3, 8,-5, 7,-6, 5,-7,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	23,24, /* Ascii 126 */
	3, 6, 3, 8, 4,11, 6,12, 8,12,10,11,14, 8,16, 7,18, 7,20, 8,21,10,-1,
	-1, 3, 8, 4,10, 6,11, 8,11,10,10,14, 7,16, 6,18, 6,20, 7,21,10,21,12,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
};

void GLI_fnv_Draw2DLine( MTH3D_tdstVector *p_stFirstPoint, MTH3D_tdstVector *p_stLastPoint, unsigned long ulColor )
{
	GLI_tdstInternalGlobalValuesFor3dEngine *pstGlobals = *GLI_BIG_GLOBALS;
	GLD_tdstViewportAttributes *p_stVpt = &GAM_g_stEngineStructure->stFixViewportAttr;

	GEO_tdstColor stColor;
	GLI_tdstAligned3DVector a2_stVertex[2];
	GLI_tdstAligned2DVector a2_st2DVertex[2];

	pstGlobals->hCurrentMaterial = NULL;
	pstGlobals->lCurrentDrawMask = 0xFFFFFFFF;
	pstGlobals->lHierachDrawMask = 0xFFFFFFFF;
	GLI_vDoMaterialSelection(pstGlobals);

	a2_st2DVertex[0].ulPackedColor = a2_st2DVertex[1].ulPackedColor = ulColor;

	stColor.xR = (float)((ulColor & 0x00FF0000) >> 16) * 1.0f;
	stColor.xG = (float)((ulColor & 0x0000FF00) >> 8) * 1.0f;
	stColor.xB = (float)((ulColor & 0x000000FF) >> 0) * 1.0f;
	stColor.xA = 255.f;

	a2_st2DVertex[0].xX = p_stFirstPoint->x * p_stVpt->dwWidth;
	a2_st2DVertex[0].xY = p_stFirstPoint->y * p_stVpt->dwHeight;
	a2_stVertex[0].xZ = a2_st2DVertex[0].xOoZ = 10000.0f;
	a2_st2DVertex[1].xX = p_stLastPoint->x * p_stVpt->dwWidth;
	a2_st2DVertex[1].xY = p_stLastPoint->y * p_stVpt->dwHeight;
	a2_stVertex[1].xZ = a2_st2DVertex[1].xOoZ = 10000.0f;

	(*GLI_DRV_vSendSingleLineToClip_)(
		p_stVpt,
		&a2_stVertex[0],
		&a2_st2DVertex[0],
		&a2_stVertex[1],
		&a2_st2DVertex[1],
		pstGlobals,
		GLI_C_Mat_lGouraudLineElement,
		&stColor
	);
}

void GLI_fnv_Draw2DLineForHershey( MTH3D_tdstVector *p_stFirstPoint, MTH3D_tdstVector *p_stLastPoint, unsigned long ulColor, MTH_tdxReal xScale )
{
	GLD_tdstViewportAttributes *p_stVpt = &GAM_g_stEngineStructure->stFixViewportAttr;
	GLI_tdstInternalGlobalValuesFor3dEngine *pstGlobals = *GLI_BIG_GLOBALS;

	GEO_tdstColor stColor;
	GLI_tdstAligned3DVector a2_stVertex[2];
	GLI_tdstAligned2DVector a2_st2DVertex[2];

	pstGlobals->hCurrentMaterial = NULL;
	pstGlobals->lCurrentDrawMask = 0xFFFFFFFF;
	pstGlobals->lHierachDrawMask = 0xFFFFFFFF;
	GLI_vDoMaterialSelection(pstGlobals);

	a2_st2DVertex[0].ulPackedColor = a2_st2DVertex[1].ulPackedColor = ulColor;

	stColor.xR = (float)((ulColor & 0x00FF0000) >> 16) * 1.0f;
	stColor.xG = (float)((ulColor & 0x0000FF00) >> 8) * 1.0f;
	stColor.xB = (float)((ulColor & 0x000000FF) >> 0) * 1.0f;
	stColor.xA = 255.f;

	float xRatio = g_xScreenRatio / 0.75f;
	float xOffset = p_stVpt->dwWidth * (1 - xRatio) / 2;

	a2_st2DVertex[0].xX = p_stFirstPoint->x * 0.001f * xScale * p_stVpt->dwWidth * xRatio + xOffset;
	a2_st2DVertex[0].xY = p_stFirstPoint->y * 0.001f * xScale * p_stVpt->dwHeight;
	a2_stVertex[0].xZ = a2_st2DVertex[0].xOoZ = 10000.0f;
	a2_st2DVertex[1].xX = p_stLastPoint->x * 0.001f * xScale * p_stVpt->dwWidth * xRatio + xOffset;
	a2_st2DVertex[1].xY = p_stLastPoint->y * 0.001f * xScale * p_stVpt->dwHeight;
	a2_stVertex[1].xZ = a2_st2DVertex[1].xOoZ = 10000.0f;

	pstGlobals->lCurrentDrawMask = GLI_C_Mat_lGouraudLineElement;

	(*GLI_DRV_vSendSingleLineToClip_)(
		p_stVpt,
		&a2_stVertex[0],
		&a2_st2DVertex[0],
		&a2_stVertex[1],
		&a2_st2DVertex[1],
		pstGlobals,
		GLI_C_Mat_lGouraudLineElement,
		&stColor
	);
}

int fn_lDrawHershey( char ch, MTH_tdxReal xX, MTH_tdxReal xY, unsigned long ulColor )
{
	if ( ch < ' ' || ch > '~' )
		return 0;

	ch -= ' ';

	tdstHersheyChar *pData = (tdstHersheyChar*)simplex[ch];
	tdstHersheyVertex *pPrev = NULL;

	for ( int i = 0; i < pData->lNbVertex; ++i )
	{
		tdstHersheyVertex *pCur = &pData->aVertices[i];
		if ( pCur->x == -1 && pCur->y == -1 )
			pCur = NULL;
		else if ( pPrev )
		{
			MTH3D_tdstVector stA = {
				(float)pPrev->x + xX,
				(float)pPrev->y * -1 + 32 + xY
			};
			MTH3D_tdstVector stB = {
				(float)pCur->x + xX,
				(float)pCur->y * -1 + 32 + xY
			};

			GLI_fnv_Draw2DLineForHershey(&stA, &stB, ulColor, 1.f);
		}
		pPrev = pCur;
	}

	return pData->lSpacing;
}

void fn_vPrintHershey( char const *szTxt, MTH_tdxReal xX, MTH_tdxReal xY, unsigned long ulColor )
{
	MTH_tdxReal xLineX = xX;

	if ( !szTxt || strlen(szTxt) == 0 )
		return;

	for ( ; *szTxt; szTxt++ )
	{
		if ( *szTxt == '\n' )
		{
			xY += 32;
			xX = xLineX;
			continue;
		}
		xX += fn_lDrawHershey(*szTxt, xX, xY, ulColor);
	}
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

	MTH3D_tdstVector stTopLeft = { 0.0685f, 0.125f, 0 };
	MTH3D_tdstVector stBottomRight = { 1.0f-0.0684f, 1.0f-0.0625f, 0 };
	//GLI_fnv_Draw2DLine(&stTopLeft, &stBottomRight, 0xFFFFFF);

	/*
	fn_vPrintHershey("they will not protect us\n(we will fall)", 0, 0, 0xFFFFFF);
	fn_vPrintHershey("they will not defend us", 16, 70, 0xFFFFFF);
	fn_vPrintHershey("[we'll be left behind]", 16, 102, 0xFF8080);
	*/
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
