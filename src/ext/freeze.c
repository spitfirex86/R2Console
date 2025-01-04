#include "../framework.h"
#include "freeze.h"


/* freeze the engine */
BOOL g_bFreezeEngine = FALSE;
/* (0) = frozen, (-1) = running, (n>0) = run for n frames */
int g_lFreezeRequest = -1;
int g_lSaveCurrentFrame = 0;


void FRZ_fn_vDoEngineFreeze( void )
{
	if ( g_bFreezeEngine ) /* engine is already frozen... */
	{
		if ( g_lFreezeRequest != 0 ) /* ...and unfreeze was requested */
		{
			g_bFreezeEngine = FALSE;
			GAM_g_stEngineStructure->bEngineIsInPaused = FALSE;
			*HIE_gs_lCurrentFrame = g_lSaveCurrentFrame;
			GAM_fn_vLoadEngineClock();
			//Sleep(GAM_g_stEngineStructure->stEngineTimer.ulUsefulDeltaTime);
			//fn_vActualizeEngineClock();
		}
	}
	else /* engine is running... */
	{
		if ( g_lFreezeRequest == 0 ) /* ...and freeze was requested */
		{
			g_bFreezeEngine = TRUE;
			GAM_g_stEngineStructure->bEngineIsInPaused = TRUE;
			g_lSaveCurrentFrame = *HIE_gs_lCurrentFrame;
			GAM_fn_vSaveEngineClock();
		}
	}

	/* decrease frame counter if unfreeze is temporary */
	( g_lFreezeRequest > 0 ) ? --g_lFreezeRequest : g_lFreezeRequest;
}

void FRZ_fn_vFreezeCmd( int lNbArgs, char **d_szArgs )
{
	int lNbFrames = 0;

	if ( lNbArgs >= 1 && !fn_bParseInt(d_szArgs[0], &lNbFrames) )
	{
		fn_vPrint("Usage: freeze [n]");
		fn_vPrintCFmt(0, "Engine is currently %s", (g_bFreezeEngine) ? "frozen" : "running");
		return;
	}

	if ( g_bFreezeEngine )
	{
		if ( lNbFrames > 0 )
		{
			g_lFreezeRequest = lNbFrames;
			fn_vPrintCFmt(0, "Unfreezing for %d frames", lNbFrames);
		}
		else
		{
			g_lFreezeRequest = -1;
			fn_vPrint("Engine running");
		}
	}
	else
	{
		if ( lNbFrames > 0 )
		{
			g_lFreezeRequest = lNbFrames;
			fn_vPrintCFmt(0, "Freezing after %d frames", lNbFrames);
		}
		else
		{
			g_lFreezeRequest = 0;
			fn_vPrint("Engine frozen");
		}
	}
}
