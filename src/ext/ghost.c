#include "../framework.h"
#include "ghost.h"


BOOL (*PLA_fn_vInitNewState)( HIE_tdstSuperObject *p_stSuperObject, void *h_State ) = OFFSET(0x40FE70);
void (*GAM_fn_vSectInfoTestChangeSectorForCharacter)( HIE_tdstSuperObject *hCharacter ) = OFFSET(0x4128D0);


MTH_tdxReal GST_g_xSpeed = 1.0f;
MTH_tdxReal GST_g_xAlpha = 0.10f;

MTH_tdxReal GST_g_xAddAngle = 0;
MTH3D_tdstVector GST_g_stAdd = { 0 };


BOOL GST_fn_bToggleGhostMode( void )
{
	*GAM_g_ucIsEdInGhostMode = !*GAM_g_ucIsEdInGhostMode;
	
	if ( *GAM_g_ucIsEdInGhostMode )
		GAM_g_stEngineStructure->g_hMainActor->lDrawModeMask &= ~GLI_C_Mat_lIsNotWired;
	else
		GAM_g_stEngineStructure->g_hMainActor->lDrawModeMask |= GLI_C_Mat_lIsNotWired;

	GST_g_stAdd.x = 0;
	GST_g_stAdd.y = 0;
	GST_g_stAdd.z = 0;
	GST_g_xAddAngle = 0;

	return *GAM_g_ucIsEdInGhostMode;
}

void GST_fn_vUpdateSuperObj( HIE_tdstSuperObject *pCharacter )
{
	if ( !*GAM_g_ucIsEdInGhostMode )
		return;

	MTH_tdxReal xAngle = DNM_fn_xComputeAngleOfPerso(pCharacter);
	
	MTH3D_tdstVector stV = pCharacter->p_stLocalMatrix->stPos;
	MTH3D_tdstVector stCumul = GST_g_stAdd;
	
	POS_fn_vTurnMatrixZ(pCharacter->p_stLocalMatrix, GST_g_xAddAngle);
	POS_fn_vMulMatrixVector(&stCumul, pCharacter->p_stLocalMatrix, &stCumul);
	MTH3D_M_vAddVector(&stV, &stV, &stCumul);
	pCharacter->p_stLocalMatrix->stPos = stV;
	pCharacter->p_stGlobalMatrix->stPos = stV;

	pCharacter->hLinkedObject.p_stCharacter->hDynam->p_stDynamics->stDynamicsBase.ulEndFlags |= 0x00000080;
}

void GST_fn_vDoGhostMode( void )
{
	if ( !*GAM_g_ucIsEdInGhostMode )
		return;

	HIE_tdstSuperObject *hMainActor = GAM_g_stEngineStructure->g_hMainActor;

	AI_fn_vReinitAI(hMainActor->hLinkedObject.p_stCharacter->hBrain->p_stMind);
	PLA_fn_vInitNewState(hMainActor, *(void **)hMainActor->hLinkedObject.p_stCharacter->h3dData);

	MTH_tdxReal xXAxis = IPT_g_stInputStructure->d_stEntryElementArray[IPT_E_Entry_Action_Pad_AxeX].xAnalogicValue;
	MTH_tdxReal xYAxis = IPT_g_stInputStructure->d_stEntryElementArray[IPT_E_Entry_Action_Pad_AxeY].xAnalogicValue;

	if ( xXAxis > 10 || xXAxis < -10 )
		GST_g_stAdd.x -= xXAxis / 100 * GST_g_xSpeed;

	if ( xYAxis > 10 || xYAxis < -10 )
		GST_g_stAdd.y += xYAxis / 100 * GST_g_xSpeed;

	if ( IPT_g_stInputStructure->d_stEntryElementArray[IPT_E_Entry_Action_Camera_TourneGauche].lState > 0 )
		GST_g_xAddAngle += GST_g_xAlpha;

	if ( IPT_g_stInputStructure->d_stEntryElementArray[IPT_E_Entry_Action_Camera_TourneDroite].lState > 0 )
		GST_g_xAddAngle -= GST_g_xAlpha;

	if ( IPT_g_stInputStructure->d_stEntryElementArray[IPT_E_Entry_Action_Nage_Remonter].lState > 0 )
		GST_g_stAdd.z += GST_g_xSpeed;

	if ( IPT_g_stInputStructure->d_stEntryElementArray[IPT_E_Entry_Action_Nage_Plonger].lState > 0 )
		GST_g_stAdd.z -= GST_g_xSpeed;

	if ( IPT_g_stInputStructure->d_stEntryElementArray[IPT_E_Entry_Action_Clavier_Haut].lState > 0 )
		GST_g_stAdd.y -= GST_g_xSpeed;

	if ( IPT_g_stInputStructure->d_stEntryElementArray[IPT_E_Entry_Action_Clavier_Bas].lState > 0 )
		GST_g_stAdd.y += GST_g_xSpeed;

	if ( IPT_g_stInputStructure->d_stEntryElementArray[IPT_E_Entry_Action_Clavier_Gauche].lState > 0 )
		GST_g_xAddAngle += GST_g_xAlpha;

	if ( IPT_g_stInputStructure->d_stEntryElementArray[IPT_E_Entry_Action_Clavier_Droite].lState > 0 )
		GST_g_xAddAngle -= GST_g_xAlpha;

	GST_fn_vUpdateSuperObj(hMainActor);
	//GST_fn_vUpdateSuperObj(p_stCamera);
	//p_stCamera->p_stLocalMatrix->stPos = hMainActor->p_stLocalMatrix->stPos;

	GAM_fn_vSectInfoTestChangeSectorForCharacter(hMainActor);
	//GAM_fn_vSectInfoTestChangeSectorForCharacter(p_stCamera);

	GST_g_stAdd.x = 0;
	GST_g_stAdd.y = 0;
	GST_g_stAdd.z = 0;
	GST_g_xAddAngle = 0;
}
