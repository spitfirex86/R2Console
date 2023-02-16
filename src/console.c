#include "framework.h"
#include "console.h"
#include "gfx/graphics.h"
#include "gfx/font.h"
#include "gfx/cursor.h"
#include "r2ptr.h"
#include "cvars.h"
#include "ext/ghost.h"


char const g_szVersion[] = "R2Console v1.0 (" __DATE__ ")";

BOOL g_bIsInit = FALSE;
BOOL g_bShow = FALSE;

MTH2D_tdstVector const g_stHiddenPos = { 0.0f, -M_FontToPercentY((C_LinesOnScreen + 1) * C_Font_xCharHeight) };
MTH2D_tdstVector const g_stVisiblePos = { 0.0f, 0.0f };

MTH2D_tdstVector g_stCurrentPos = { .0f, .0f };
MTH2D_tdstVector g_stSize = { 100.0f, M_FontToPercentY((C_LinesOnScreen + 1) * C_Font_xCharHeight) };


int g_lFrameCounter = 0;
int g_lCaretFrame = 0;

#define C_AnimFrames 30
#define C_Transparency 0xB0


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


void fn_vAnimOneStep( void )
{
	if ( g_lFrameCounter >= 0 && g_lFrameCounter <= C_AnimFrames )
	{
		MTH_tdxReal xPercent = (MTH_tdxReal)g_lFrameCounter / (MTH_tdxReal)C_AnimFrames;
		GFX_fn_vLerp2DVector(&g_stCurrentPos, &g_stHiddenPos, &g_stVisiblePos, xPercent);

		g_lFrameCounter += g_bShow ? 1 : -1;
	}
}

void fn_vDrawConsoleSprites( void )
{
	MTH2D_tdstVector stTL = g_stCurrentPos;
	MTH2D_tdstVector stBR = { 0 };
	GFX_fn_vAdd2DVector(&stBR, &g_stCurrentPos, &g_stSize);

	GFX_fn_vDisplayFrameWithZValue(&stTL, &stBR, C_Transparency, 1.1f, &GAM_g_stEngineStructure->stFixViewportAttr);

	CUR_fn_vDrawCursor();
}

void fn_vDrawConsole( void )
{
	MTH_tdxReal x, y;

	if ( g_lFrameCounter <= 0 )
		return;

	fn_vAnimOneStep();
	fn_vDrawConsoleSprites();

	*GLI_p_fZValueForSprite = 1.2f;
	
	x = M_PercentToFontX(g_stCurrentPos.x);
	y = M_PercentToFontY(g_stCurrentPos.y);

	/* console history */
	for ( int i = C_LinesOnScreen + g_ulOnScreenOffset - 1; i >= (int)g_ulOnScreenOffset; i-- )
	{
		g_a_stLines[i].cPrefix
			? FNT_fn_vDisplayStringFmt(x, y, "/o%d:%c%s", g_a_stLines[i].ucColor, g_a_stLines[i].cPrefix, g_a_stLines[i].szText)
			: FNT_fn_vDisplayStringFmt(x, y, "/o%d:%s", g_a_stLines[i].ucColor, g_a_stLines[i].szText);
		y += C_Font_xCharHeight;

	}
	//y += C_Font_xCharHeight;

	/* prompt */
	FNT_fn_vDisplayStringFmt(x, y, "/o1:]%s", g_szPrompt);

	*GLI_p_fZValueForSprite = 1.22f;

	MTH_tdxReal xCaretOffset = C_Font_xActualCharWidth * (float)(g_ulCaretPos + 1);
	g_lCaretFrame = (g_lCaretFrame + 1) % (C_CaretFrames * 2);

	if ( g_lCaretFrame < C_CaretFrames )
		FNT_fn_vDisplayString(x + xCaretOffset, y, (g_bReplaceMode ? "/o2:_" : "_"));

	/* scrollbar */
	MTH_tdxReal xScrollBarHeight = M_PercentToFontY(g_stSize.y) - (C_Font_xCharHeight * 2);
	long lScrollBarSteps = C_NbLines - C_LinesOnScreen;
	MTH_tdxReal xScrollBarPos = (1 - (float)g_ulOnScreenOffset / (float)lScrollBarSteps) * xScrollBarHeight;
	
	x = M_PercentToFontX(g_stCurrentPos.x + g_stSize.x) - C_Font_xCharWidth;
	y = M_PercentToFontY(g_stCurrentPos.y) + xScrollBarPos;

	FNT_fn_vDisplayString(x, y, "/o1:[");

	*GLI_p_fZValueForSprite = 0.998f;
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
	}
	else
	{
	    IPT_g_stInputStructure->ulNumberOfEntryElement = s_ulSaveNbEntry;

		if ( CON_bPauseGame->bValue )
			GAM_g_stEngineStructure->bEngineIsInPaused = FALSE;
	}
}

void fn_vPushHistory( char *szString )
{
	// Shift history by 1
	memmove(&g_a_szCmdHistory[1], &g_a_szCmdHistory[0], (C_NbHistory-1)*C_MaxLine);
	strcpy(g_a_szCmdHistory[0], szString);
}

void fn_vPushLine( char *szString, unsigned char ucColor, char cPrefix )
{
	// Shift lines by 1
	memmove(&g_a_stLines[1], &g_a_stLines[0], (C_NbLines-1)*sizeof(tdstLine));

    g_a_stLines[0].ucColor = ucColor;
    g_a_stLines[0].cPrefix = cPrefix;
	strcpy(g_a_stLines[0].szText, szString);

	// Scroll to newest line
	g_ulOnScreenOffset = 0;
}

void fn_vPrintEx( char const *szString, unsigned char ucColor, char cPrefix )
{
	char szBuffer[C_MaxLine];
	char *pChar = szString;
	int nChars = 0;

    do
    {
		szBuffer[nChars] = (*pChar == '\n') ? ' ' : *pChar;
		nChars++;

        if ( *pChar == 0 || *pChar == '\n' || nChars == C_MaxLine - 1 )
        {
			szBuffer[nChars] = '\0';
			nChars = 0;

			fn_vPushLine(szBuffer, ucColor, cPrefix);
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

	szString += strspn(szString, " ");
	int length = strcspn(szString, " ");

	if ( *szString == '!' )
	{
		g_bForceThisCommand = TRUE;
		szString++; length--;
	}

    if ( length <= 0 )
    {
        fn_vPrintC(2, "Unknown command");
		return;
    }

	strncpy(szCommand, szString, length);
	szCommand[length] = 0;

	for ( int i = 0; i < g_lNbCommands; i++ )
	{
        if ( _stricmp(szCommand, g_a_stCommands[i].szName) == 0 )
        {
			strcpy(szArgs, szString + length);
			int lCount = fn_lSplitArgs(szArgs, &d_szArgs);

            g_a_stCommands[i].p_stCommand(lCount, d_szArgs);
			g_lLastCommandId = i;

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
	if ( lLines > 0 ) // scrolling up
	{
		// upper bound of the buffer
		if ( g_ulOnScreenOffset + lLines > (C_NbLines - C_LinesOnScreen) )
			lLines = (C_NbLines - C_LinesOnScreen) - g_ulOnScreenOffset;

		// if the line we're trying to scroll to is empty, find next non-empty line
		while ( lLines > 0 && g_a_stLines[g_ulOnScreenOffset + lLines + (C_LinesOnScreen - 1)].szText[0] == 0 )
			lLines--;
	}
	else if ( lLines < 0 ) // scrolling down
	{
		// lower bound of the buffer
		if ( (long)g_ulOnScreenOffset + lLines < 0 )
			lLines = -(long)g_ulOnScreenOffset;
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
}

void fn_vInitConsole( void )
{
	FNT_fn_vFontInit();

	fn_vInitVars();

	fn_vPrint(g_szVersion);
	fn_vPrint("Type \"help\" for available commands");

	g_bIsInit = TRUE;
}


void MOD_GLI_vComputeTextures( void )
{
	FNT_fn_vLoadFontTexture();
	CUR_fn_vLoadCursorTexture();

	R2_GLI_vComputeTextures();
}

void MOD_AGO_vDisplayGAUGES( GLD_tdstViewportAttributes *p_stVpt )
{
	R2_AGO_vDisplayGAUGES(p_stVpt);

	CUR_fn_vInvalidateCursor();
	fn_vDrawConsole();
}

void MOD_fn_vEngine( void )
{
	GST_fn_vDoGhostMode();

	R2_fn_vEngine();
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
	else if ( uMsg == WM_MOUSEMOVE )
	{
		RECT rc;
		GetClientRect(hWnd, &rc);
		
		stPos.x = (float)LOWORD(lParam) / (float)(rc.right - rc.left) * 100.0f;
		stPos.y = (float)HIWORD(lParam) / (float)(rc.bottom - rc.top) * 100.0f;

		CUR_fn_vMoveCursor(&stPos);

#if defined(USE_WATCH)
		WAT_fn_vHitTestMove(&stPos, (wParam & MK_LBUTTON));
#endif

		return 0;
	}
	else if ( uMsg == WM_MOUSEWHEEL )
	{
		short wLines = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;

		// hit test
		if ( stPos.x > g_stCurrentPos.x && stPos.x < (g_stCurrentPos.x + g_stSize.x)
			&& stPos.y > g_stCurrentPos.y && stPos.y < (g_stCurrentPos.y + g_stSize.y) )
		{
			fn_vScrollConsole(wLines);
		}
	}
	else if ( uMsg == WM_RBUTTONDOWN )
	{
#if defined(USE_WATCH)
		WAT_fn_vHitTestClose(&stPos);
#endif
	}

	return R2_WndProc(hWnd, uMsg, wParam, lParam);
}
