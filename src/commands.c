#include "framework.h"
#include "console.h"
#include "cvars.h"
#include "ext/ghost.h"
#include "ext/freeze.h"


tdfnCommand fn_vHelpCmd;
tdfnCommand fn_vClearCmd;
tdfnCommand fn_vMapCmd;
tdfnCommand fn_vReinitCmd;
tdfnCommand fn_vMapsCmd;
tdfnCommand fn_vListObjCmd;
tdfnCommand fn_vFindCmd;
tdfnCommand fn_vGetSetPosCmd;
tdfnCommand fn_vHexViewCmd;
tdfnCommand fn_vVersionCmd;
tdfnCommand fn_vQuitCmd;
tdfnCommand fn_vTeleportCmd;
tdfnCommand fn_vTeleportToCmd;
tdfnCommand fn_vGhostCmd;
tdfnCommand fn_vCVarCmd;
tdfnCommand fn_vActorCmd;
tdfnCommand fn_vMainActorCmd;
tdfnCommand fn_vWireframeCmd;
tdfnCommand fn_vBrightnessCmd;

tdstCommand *g_a_stCommands = NULL;
int g_lNbCommands = 0;

void fn_vInitCommands( void ) {
	fn_vRegisterCommand("map", fn_vMapCmd);
	fn_vRegisterCommand("pos", fn_vGetSetPosCmd);
	fn_vRegisterCommand("tp", fn_vTeleportCmd);
	fn_vRegisterCommand("tpto", fn_vTeleportToCmd);
	fn_vRegisterCommand("noclip", fn_vGhostCmd);
	fn_vRegisterCommand("maps", fn_vMapsCmd);
	fn_vRegisterCommand("listobj", fn_vListObjCmd);
	fn_vRegisterCommand("find", fn_vFindCmd);
	fn_vRegisterCommand("actor", fn_vActorCmd);
	fn_vRegisterCommand("mainactor", fn_vMainActorCmd);
	fn_vRegisterCommand("reinit", fn_vReinitCmd);
	fn_vRegisterCommand("hexview", fn_vHexViewCmd);
#if defined(USE_WATCH)
	fn_vRegisterCommand("watch", WAT_fn_vWatchCmd);
#endif
	fn_vRegisterCommand("freeze", FRZ_fn_vFreezeCmd);
	fn_vRegisterCommand("cvar", fn_vCVarCmd);
	fn_vRegisterCommand("wireframe", fn_vWireframeCmd);
	fn_vRegisterCommand("bright", fn_vBrightnessCmd);
	fn_vRegisterCommand("clear", fn_vClearCmd);
	fn_vRegisterCommand("help", fn_vHelpCmd);
	fn_vRegisterCommand("version", fn_vVersionCmd);
	fn_vRegisterCommand("quit", fn_vQuitCmd);
}

#define C_CmdArrayChunk 16
void fn_vInternalRegisterCommand( tdstCommand *pNewCmd )
{
	static int s_ulArraySize = 0;

	if ( g_lNbCommands >= s_ulArraySize )
	{
		s_ulArraySize += C_CmdArrayChunk;
		tdstCommand *pNewArray = realloc(g_a_stCommands, s_ulArraySize * sizeof(tdstCommand));
		if ( !pNewArray )
		{
			fn_vPrintCFmt(2, "Error allocating command array while registering \"%s\"", pNewCmd->szName);
			return;
		}
		g_a_stCommands = pNewArray;
	}

	g_a_stCommands[g_lNbCommands] = *pNewCmd;
	g_lNbCommands++;
}

void fn_vRegisterCommand( char *szName, tdfnCommand *p_stCommand )
{
	if ( !szName || *szName == 0 )
	{
		fn_vPrintCFmt(2, "Cannot register command %p: name empty", p_stCommand);
		return;
	}
	if ( strlen(szName) > C_MaxCmdName )
	{
		fn_vPrintCFmt(2, "Cannot register command \"%s\": name too long (max %d chars)", szName, C_MaxCmdName);
		return;
	}

	// TODO: make sure command names are not duplicate !!

	tdstCommand stCmd = { 0 };
	stCmd.p_stCommand = p_stCommand;
	strcpy(stCmd.szName, szName);

	fn_vInternalRegisterCommand(&stCmd);
}

void fn_vHelpCmd( int lNbArgs, char **d_szArgs )
{
	char *szBuffer = _alloca((g_lNbCommands * C_MaxCmdName) + 32);
	if ( !szBuffer )
		return;

	strcpy(szBuffer, "Available commands:\n");
	char *pStr = szBuffer + strlen(szBuffer);
	int lBuf = 0;

	for ( int i = 0; i < g_lNbCommands; i++ )
	{
		int lCmd = strlen(g_a_stCommands[i].szName);

		if ( lBuf+lCmd+1 >= C_MaxLine )
		{
			pStr[lBuf++] = '\n';
			pStr += lBuf;
			lBuf = 0;
		}

		strcpy(pStr+lBuf, g_a_stCommands[i].szName);
		lBuf += lCmd;
		pStr[lBuf++] = ' ';
	}
	pStr[lBuf] = 0;

	fn_vPrint(szBuffer);
}

void fn_vClearCmd( int lNbArgs, char **d_szArgs )
{
	ZeroMemory(g_a_stLines, C_NbLines * sizeof(tdstLine));

	fn_vResetScroll();
}

void fn_vMapCmd( int lNbArgs, char **d_szArgs )
{
	if ( lNbArgs < 1 )
	{
		fn_vPrint("Usage: map [name]");
		fn_vPrintCFmt(0, "Current map: " M_HiLite("%s"), GAM_g_stEngineStructure->szLevelName);
		return;
	}

	char *szName = d_szArgs[0];

	for ( int i = 0; i < GAM_g_stEngineStructure->ucNumberOfLevels; i++ )
	{
		char *szMap = GAM_g_stEngineStructure->a_szLevelName[i];
		if ( _stricmp(szName, szMap) == 0 )
		{
			fn_vPrintCFmt(0, "Changing map: " M_HiLite("%s") " --> " M_HiLite("%s"), GAM_g_stEngineStructure->szLevelName, szMap);
			GAM_fn_vAskToChangeLevel(szMap, FALSE);
			return;
		}
	}

	fn_vPrintCFmt(2, "Unknown map \"%s\"", szName);
}

void fn_vMapsCmd( int lNbArgs, char **d_szArgs )
{
	char szName[32];

	//fn_vPrint("Maps:");
	for ( int i = 0; i < GAM_g_stEngineStructure->ucNumberOfLevels; i++ )
	{
		char *szMap = GAM_g_stEngineStructure->a_szLevelName[i];
		sprintf(szName, M_HiLite("%s"), szMap);
		fn_vPrintCFmt(0, "%-*s%s", 14, szName, g_a_szMapsDesc[i]);
	}
}

void fn_vReinitCmd( int lNbArgs, char **d_szArgs )
{
	fn_vPrint("Reloading current map");
	GAM_fn_vChangeEngineMode(E_EM_ModePlayerDead);
}

void fn_vListObjCmd( int lNbArgs, char **d_szArgs )
{
	char const szUsage[] = "Usage: listobj [-mode]\n"
		"Optional modes: v - only visible, a - all (active+inactive)";

	ACP_tdxBool bOnlyVisible = FALSE;
	ACP_tdxBool bIncludeInactive = FALSE;

	if ( lNbArgs < 1 )
	{
		fn_vPrint(szUsage);
	}
	else if ( *d_szArgs[0] == '-' )
	{
		char *szMode = d_szArgs[0]+1;
		switch ( tolower(*szMode) )
		{
			case 'v': bOnlyVisible = TRUE; break;
			case 'a': bIncludeInactive = TRUE; break;
			default: fn_vPrintCFmt(2, "Invalid argument \"%s\"", d_szArgs[0]); return;
		}
	}
	else
	{
		fn_vPrintCFmt(2, "Invalid argument \"%s\"", d_szArgs[0]);
		return;
	}

	HIE_tdstSuperObject *pChild;
	LST_M_DynamicForEach(*GAM_g_p_stDynamicWorld, pChild)
	{
		if ( pChild->ulType == HIE_C_Type_Actor )
		{
			if( !(pChild->hLinkedObject.p_stCharacter->hStandardGame->ulCustomBits & Std_C_CustBit_OutOfVisibility) )
			{
				fn_vPrintCFmt(0, "" M_HiLite("%8p") "  V " M_HiLite("%s"), pChild, HIE_fn_szGetObjectPersonalName(pChild));
			}
			else
			{
				if ( bOnlyVisible )
					continue;
				fn_vPrintCFmt(0, "" M_HiLite("%8p") "    " M_HiLite("%s"), pChild, HIE_fn_szGetObjectPersonalName(pChild));
			}
		}
	}

	if ( !bIncludeInactive )
		return;

	LST_M_DynamicForEach(*GAM_g_p_stInactiveDynamicWorld, pChild)
	{
		if ( pChild->ulType == HIE_C_Type_Actor )
		{
			fn_vPrintCFmt(0, M_HiLite("%8p") "  I " M_HiLite("%s"), pChild, HIE_fn_szGetObjectPersonalName(pChild));
		}
	}
}

void fn_vFindCmd( int lNbArgs, char **d_szArgs )
{
	char const szUsage[] = "Usage: find [-mode] <name>\n"
		"Optional modes: m - maps, a - actors, v - cvars\n"
		"Modes can be combined like \"-xyz\".";

	char szFind[128];
	char szName[128];

	ACP_tdxBool bSearchALL = FALSE;
	ACP_tdxBool bSearchMap = FALSE;
	ACP_tdxBool bSearchAct = FALSE;
	ACP_tdxBool bSearchVar = FALSE;

	int lFoundMap = 0;
	int lFoundAct = 0;
	int lFoundVar = 0;
	
	if ( lNbArgs < 1 )
	{
		fn_vPrint(szUsage);
		return;
	}

	if ( lNbArgs > 1 && *d_szArgs[0] == '-' )
	{
		char *szMode = d_szArgs[0]+1;
		while ( *szMode )
		{
			switch ( tolower(*szMode++) )
			{
				case 'm': bSearchMap = TRUE; break;
				case 'a': bSearchAct = TRUE; break;
				case 'v': bSearchVar = TRUE; break;
				default: fn_vPrintCFmt(2, "Invalid argument \"%s\"", d_szArgs[0]); return;
			}
		}

		fn_vToLower(szFind, d_szArgs[1]);
	}
	else
	{
		bSearchALL = TRUE;
		fn_vToLower(szFind, d_szArgs[0]);
	}

	if ( bSearchALL || bSearchMap )
	{
		fn_vPrint("Maps:");
		for ( int i = 0; i < GAM_g_stEngineStructure->ucNumberOfLevels; i++ )
		{
			char *szMap = GAM_g_stEngineStructure->a_szLevelName[i];
			char *szDesc = g_a_szMapsDesc[i];

			if ( (fn_vToLower(szName, szMap), strstr(szName, szFind))
				|| (fn_vToLower(szName, szDesc), strstr(szName, szFind)) )
			{
				lFoundMap++;
				sprintf(szName, M_HiLite("%s"), szMap);
				fn_vPrintCFmt(0, "  %-*s%s", 14, szName, szDesc);
			}
		}
		if ( !lFoundMap )
			fn_vPrint("  ---- None ----");
	}

	if ( bSearchALL || bSearchAct )
	{
		fn_vPrint("Actors:");
		HIE_tdstSuperObject *pAct;
		LST_M_DynamicForEach(*GAM_g_p_stDynamicWorld, pAct)
		{
			if ( pAct->ulType == HIE_C_Type_Actor )
			{
				char *pName = HIE_fn_szGetObjectPersonalName(pAct);
				if ( !pName )
					continue;
			
				fn_vToLower(szName, pName);
				if ( strstr(szName, szFind) )
				{
					lFoundAct++;
					fn_vPrintCFmt(0, "  " M_HiLite("%8p") "    " M_HiLite("%s"), pAct, pName);
				}
			}
		}
		LST_M_DynamicForEach(*GAM_g_p_stInactiveDynamicWorld, pAct)
		{
			if ( pAct->ulType == HIE_C_Type_Actor )
			{
				char *pName = HIE_fn_szGetObjectPersonalName(pAct);
				if ( !pName )
					continue;
			
				fn_vToLower(szName, pName);
				if ( strstr(szName, szFind) )
				{
					lFoundAct++;
					fn_vPrintCFmt(0, "  " M_HiLite("%8p") "  I " M_HiLite("%s"), pAct, pName);
				}
			}
		}
		if ( !lFoundAct )
			fn_vPrint("  ---- None ----");
	}

	if ( bSearchALL || bSearchVar )
	{
		fn_vPrint("CVars:");
		for ( int i = 0; i < g_lNbCVars; i++ )
		{
			fn_vToLower(szName, g_d_pstCVars[i]->szName);
			if ( strstr(szName, szFind) )
			{
				lFoundVar++;
				fn_vPrintCFmt(0, "  " M_HiLite("%s"), g_d_pstCVars[i]->szName);
			}
		}

		if ( !lFoundVar )
			fn_vPrint("  ---- None ----");
	}

	fn_vPrintCFmt(0, "Found %d maps, %d actors, %d cvars containing \"%s\"", lFoundMap, lFoundAct, lFoundVar, szFind);
}

void fn_vActorCmd( int lNbArgs, char **d_szArgs )
{
	HIE_tdstSuperObject *pActor;

	if ( lNbArgs < 1 )
	{
		fn_vPrint("Usage: actor <ref> [prop] [value]");
		return;
	}

	if ( !fn_bParseObjectRef(d_szArgs[0], &pActor) )
	{
		fn_vPrintCFmt(2, "Invalid object ref \"%s\"", d_szArgs[0]);
		return;
	}

	if ( lNbArgs == 3 )
	{
		// TODO: change prop
		return;
	}

	if ( lNbArgs == 2 )
	{
		// TODO: print prop
		return;
	}

	char *szName = HIE_fn_szGetObjectPersonalName(pActor);
	fn_vPrintCFmt(0, "Actor \"%s\" (%8p):", szName, pActor);

	char *szFamily = HIE_fn_szGetObjectFamilyName(pActor);
	char *szModel = HIE_fn_szGetObjectModelName(pActor);
	fn_vPrintCFmt(0, "  Family & Model: %s\\%s", szFamily, szModel);

	MTH3D_tdstVector *pPos = &pActor->p_stGlobalMatrix->stPos;
	fn_vPrintCFmt(0, "  X: %.3f  Y: %.3f  Z: %.3f", pPos->x, pPos->y, pPos->z);

	HIE_tdstEngineObject *pPerso = pActor->hLinkedObject.p_stCharacter;
	GAM_tdstStandardGame *pStdGame = pPerso->hStandardGame;
	fn_vPrintCFmt(0, "  HitPoints: %d / %d", pStdGame->ucHitPoints, pStdGame->ucHitPointsMax);
}

void fn_vMainActorCmd( int lNbArgs, char **d_szArgs )
{
	HIE_tdstSuperObject *pMain = GAM_g_stEngineStructure->g_hMainActor;
	HIE_tdstSuperObject *pNewMain;

	char *szName = HIE_fn_szGetObjectPersonalName(pMain);

	if ( lNbArgs < 1 )
	{
		fn_vPrint("Usage: mainactor [ref]");
		fn_vPrintCFmt(0, "Current main actor: \"" M_HiLite("%s") "\" (%8p)", szName, pMain);
		return;
	}

	if ( !fn_bParseObjectRef(d_szArgs[0], &pNewMain) )
	{
		fn_vPrintCFmt(2, "Invalid object ref \"%s\"", d_szArgs[0]);
		return;
	}

	GAM_g_stEngineStructure->g_hNextMainActor = pNewMain;

	char *szNewName = HIE_fn_szGetObjectPersonalName(pNewMain);
	fn_vPrintCFmt(0, "Main actor: \"" M_HiLite("%s") "\" --> \"" M_HiLite("%s") "\"", szName, szNewName);
}

void fn_vGetSetPosCmd( int lNbArgs, char **d_szArgs )
{
	HIE_tdstSuperObject *pSpo;
	MTH3D_tdstVector stOldPos;
	MTH3D_tdstVector stNewPos;

	if ( lNbArgs < 1 )
	{
		fn_vPrint("Usage: pos <ref> [X] [Y] [Z]");
		return;
	}

	if ( !fn_bParseObjectRef(d_szArgs[0], &pSpo) )
	{
		fn_vPrintCFmt(2, "Invalid object ref \"%s\"", d_szArgs[0]);
		return;
	}

	char *szName = HIE_fn_szGetObjectPersonalName(pSpo);
	MTH3D_tdstVector *pPos = &pSpo->p_stGlobalMatrix->stPos;

	g_pvLastCommandData = pSpo;

	// just print the position
	if ( lNbArgs < 4 )
	{
		fn_vPrintCFmt(0, "" M_HiLite("%s") " :  X: %.3f  Y: %.3f  Z: %.3f", szName, pPos->x, pPos->y, pPos->z);
		return;
	}

	// parse the remaining args and update position
	d_szArgs++;
	stOldPos = stNewPos = *pPos;

	int lParsed = fn_lParseCoordinates(3, d_szArgs, (MTH_tdxReal*)&stNewPos);
	if ( lParsed < 3 )
	{
		fn_vPrintCFmt(2, "Invalid argument \"%s\"", d_szArgs[lParsed]);
		return;
	}

	// move the object
	pSpo->p_stLocalMatrix->stPos = pSpo->p_stGlobalMatrix->stPos = stNewPos;
	pSpo->hLinkedObject.p_stCharacter->hDynam->p_stDynamics->stDynamicsBase.ulEndFlags |= 0x00000080;

	fn_vPrintCFmt(0, "Changing position of \"" M_HiLite("%s") "\":", szName);
	fn_vPrintCFmt(0, "  Old pos:  X: %.3f  Y: %.3f  Z: %.3f", stOldPos.x, stOldPos.y, stOldPos.z);
	fn_vPrintCFmt(0, "  New pos:  X: %.3f  Y: %.3f  Z: %.3f", pPos->x, pPos->y, pPos->z);
}

unsigned char g_ucGhostModeCameraWorkaround = 0;

void fn_vTeleportCmd( int lNbArgs, char **d_szArgs )
{
	MTH3D_tdstVector stOldPos;
	MTH3D_tdstVector stNewPos;

	if ( lNbArgs < 3 )
	{
		fn_vPrint("Usage: tp <X> <Y> <Z>");
		return;
	}

	HIE_tdstSuperObject *pMain = GAM_g_stEngineStructure->g_hMainActor;
	if ( !pMain )
	{
		fn_vPrintC(2, "Something went wrong, pointer to main actor is invalid!");
		return;
	}

	char *szName = HIE_fn_szGetObjectPersonalName(pMain);
	MTH3D_tdstVector *pPos = &pMain->p_stGlobalMatrix->stPos;

	stOldPos = stNewPos = *pPos;

	int lParsed = fn_lParseCoordinates(3, d_szArgs, (MTH_tdxReal*)&stNewPos);
	if ( lParsed < 3 )
	{
		fn_vPrintCFmt(2, "Invalid argument \"%s\"", d_szArgs[lParsed]);
		return;
	}

	pMain->p_stLocalMatrix->stPos = pMain->p_stGlobalMatrix->stPos = stNewPos;
	pMain->hLinkedObject.p_stCharacter->hDynam->p_stDynamics->stDynamicsBase.ulEndFlags |= 0x00000080;

	g_ucGhostModeCameraWorkaround = 2;

	fn_vPrintCFmt(0, "Teleporting \"" M_HiLite("%s") "\":", szName);
	fn_vPrintCFmt(0, "  Old pos:  X: %.3f  Y: %.3f  Z: %.3f", stOldPos.x, stOldPos.y, stOldPos.z);
	fn_vPrintCFmt(0, "  New pos:  X: %.3f  Y: %.3f  Z: %.3f", pPos->x, pPos->y, pPos->z);
}

void fn_vTeleportToCmd( int lNbArgs, char **d_szArgs )
{
	MTH3D_tdstVector stOldPos;
	HIE_tdstSuperObject *pDest;

	if ( lNbArgs < 1 )
	{
		fn_vPrint("Usage: tpto <ref>");
		return;
	}

	HIE_tdstSuperObject *pMain = GAM_g_stEngineStructure->g_hMainActor;
	if ( !pMain )
	{
		fn_vPrintC(2, "Something went wrong, pointer to main actor is invalid!");
		return;
	}

	if ( !fn_bParseObjectRef(d_szArgs[0], &pDest) )
	{
		fn_vPrintCFmt(2, "Invalid object ref \"%s\"", d_szArgs[0]);
		return;
	}

	char *szName = HIE_fn_szGetObjectPersonalName(pMain);
	char *szDestName = HIE_fn_szGetObjectPersonalName(pDest);

	MTH3D_tdstVector *pPos = &pMain->p_stGlobalMatrix->stPos;
	stOldPos = *pPos;

	pMain->p_stLocalMatrix->stPos = pMain->p_stGlobalMatrix->stPos = pDest->p_stGlobalMatrix->stPos;
	pMain->hLinkedObject.p_stCharacter->hDynam->p_stDynamics->stDynamicsBase.ulEndFlags |= 0x00000080;

	g_ucGhostModeCameraWorkaround = 2;

	fn_vPrintCFmt(0, "Teleporting \"" M_HiLite("%s") "\" to \"" M_HiLite("%s") "\":", szName, szDestName);
	fn_vPrintCFmt(0, "  Old pos:  X: %.3f  Y: %.3f  Z: %.3f", stOldPos.x, stOldPos.y, stOldPos.z);
	fn_vPrintCFmt(0, "  New pos:  X: %.3f  Y: %.3f  Z: %.3f", pPos->x, pPos->y, pPos->z);
}

void fn_vGhostCmd( int lNbArgs, char **d_szArgs )
{
	GST_fn_bToggleGhostMode()
		? fn_vPrint("Noclip ON")
		: fn_vPrint("Noclip OFF");
}

#define M_HexViewChar(x) (isprint((unsigned int)(x))?(x):'.')

void fn_vHexViewCmd( int lNbArgs, char **d_szArgs )
{
	unsigned char *ptr;

	if ( lNbArgs < 1 )
	{
		fn_vPrint("Usage: hexview <ptr>");
		return;
	}
	
	if ( !fn_bParsePtr(d_szArgs[0], &ptr) )
	{
		fn_vPrintCFmt(2, "Invalid ptr \"%s\"", d_szArgs[0]);
		return;
	}

	g_pvLastCommandData = ptr;

	for ( int lOffset = 0; lOffset < 4*4; lOffset += 4 )
	{
		unsigned char *optr = ptr + lOffset;
		fn_vPrintCFmt(0, "%8p : %02.2x %02.2x %02.2x %02.2x : %c%c%c%c",
					  optr,
					  optr[0], optr[1], optr[2], optr[3],
					  M_HexViewChar(optr[0]), M_HexViewChar(optr[1]),
					  M_HexViewChar(optr[2]), M_HexViewChar(optr[3])
		);
	}
}

void fn_vCVarCmd( int lNbArgs, char **d_szArgs )
{
	if ( lNbArgs < 1 )
	{
		fn_vPrint("Usage: cvar <name> [value]");
		fn_vPrint("Available CVars:");

		for ( int i = 0; i < g_lNbCVars; i++ )
		{
			tdstCVar *pVar = g_d_pstCVars[i];
			switch ( pVar->eType )
			{
			case E_CVarBool:
				fn_vPrintCFmt(0, "" M_HiLite("%s") " = %s", pVar->szName, (pVar->bValue ? "TRUE" : "FALSE"));
				break;
			case E_CVarInt:
				fn_vPrintCFmt(0, "" M_HiLite("%s") " = %d", pVar->szName, pVar->lValue);
				break;
			case E_CVarReal:
				fn_vPrintCFmt(0, "" M_HiLite("%s") " = %f", pVar->szName, pVar->xValue);
				break;
			}
		}

		return;
	}

	tdstCVar *pVar = fn_p_stGetCVar(d_szArgs[0]);
	if ( !pVar )
	{
		fn_vPrintCFmt(2, "Unknown cvar \"%s\"", d_szArgs[0]);
		return;
	}

	if ( lNbArgs < 2 )
	{
		switch ( pVar->eType )
		{
		case E_CVarBool:
			fn_vPrintCFmt(0, "" M_HiLite("%s") " = %s", pVar->szName, (pVar->bValue ? "TRUE" : "FALSE"));
			break;
		case E_CVarInt:
			fn_vPrintCFmt(0, "" M_HiLite("%s") " = %d", pVar->szName, pVar->lValue);
			break;
		case E_CVarReal:
			fn_vPrintCFmt(0, "" M_HiLite("%s") " = %f", pVar->szName, pVar->xValue);
			break;
		}
		return;
	}

	tdstCVar stOldVar = *pVar;
	BOOL bResult = FALSE;

	switch ( pVar->eType )
	{
	case E_CVarBool:
		bResult = fn_bParseBool(d_szArgs[1], &pVar->bValue);
		break;
	case E_CVarInt:
		bResult = fn_bParseInt(d_szArgs[1], &pVar->lValue);
		break;
	case E_CVarReal:
		bResult = fn_bParseReal(d_szArgs[1], &pVar->xValue);
		break;
	}

	if ( !bResult )
	{
		fn_vPrintCFmt(2, "Invalid argument \"%s\"", d_szArgs[1]);
		return;
	}

	switch ( pVar->eType )
	{
	case E_CVarBool:
		fn_vPrintCFmt(0, "  " M_HiLite("%s") ": \"%s\" --> \"%s\"", pVar->szName, (stOldVar.bValue ? "TRUE" : "FALSE"), (pVar->bValue ? "TRUE" : "FALSE"));
		break;
	case E_CVarInt:
		fn_vPrintCFmt(0, "  " M_HiLite("%s") ": \"%d\" --> \"%d\"", pVar->szName, stOldVar.lValue, pVar->lValue);
		break;
	case E_CVarReal:
		fn_vPrintCFmt(0, "  " M_HiLite("%s") ": \"%f\" --> \"%f\"", pVar->szName, stOldVar.lValue, pVar->xValue);
		break;
	}
}

void fn_vWireframeCmd( int lNbArgs, char **d_szArgs )
{
	if ( GAM_g_stEngineStructure->ulDisplayMode & GLI_C_Mat_lIsNotWired )
	{
		GAM_g_stEngineStructure->ulDisplayMode &= ~GLI_C_Mat_lIsNotWired;
		fn_vPrint("Wireframe ON");
	}
	else
	{
		GAM_g_stEngineStructure->ulDisplayMode |= GLI_C_Mat_lIsNotWired;
		fn_vPrint("Wireframe OFF");
	}
}

void fn_vBrightnessCmd( int lNbArgs, char **d_szArgs )
{
	long lCurrent = (long)(*GLI_g_xBrightness * 100.0f);

	if ( lNbArgs < 1 )
	{
		fn_vPrint("Usage: bright [value]");
		fn_vPrintCFmt(0, "Current brightness: %d", lCurrent);
		return;
	}

	long lVal;
	if ( !fn_bParseInt(d_szArgs[0], &lVal) )
	{
		fn_vPrintCFmt(2, "Invalid value \"%s\"", d_szArgs[0]);
		return;
	}

	*GLI_g_xBrightness = lVal * 0.01f;

	fn_vPrintCFmt(0, "Brightness: %d --> %d", lCurrent, lVal);
}

void fn_vVersionCmd( int lNbArgs, char **d_szArgs )
{
	fn_vPrint(g_szVersion);
}

void fn_vQuitCmd( int lNbArgs, char **d_szArgs )
{
	fn_vPrint("Goodbye!");
	SendMessage(GAM_fn_hGetWindowHandle(), WM_CLOSE, 0, 0);
}
