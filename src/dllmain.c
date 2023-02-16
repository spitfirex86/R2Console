#include "framework.h"
#include "r2ptr.h"
#include "console.h"


WNDPROC R2_WndProc = NULL;
void (*R2_AGO_vDisplayGAUGES)( GLD_tdstViewportAttributes *p_stVpt ) = NULL;
void (*R2_fn_vEngine)( void ) = NULL;
void (*R2_GLI_vComputeTextures)( void ) = NULL;


void fn_vAttachHooks( void )
{
	R2_WndProc = GAM_fn_WndProc;
	R2_AGO_vDisplayGAUGES = AGO_vDisplayGAUGES;
	R2_fn_vEngine = GAM_fn_vEngine;
	R2_GLI_vComputeTextures = GLI_vComputeTextures;

	DetourTransactionBegin();

	DetourAttach((PVOID *)&R2_WndProc, (PVOID)MOD_WndProc);
	DetourAttach((PVOID *)&R2_fn_vEngine, (PVOID)MOD_fn_vEngine);
	DetourAttach((PVOID *)&R2_AGO_vDisplayGAUGES, (PVOID)MOD_AGO_vDisplayGAUGES);
	DetourAttach((PVOID *)&R2_GLI_vComputeTextures, (PVOID)MOD_GLI_vComputeTextures);

	DetourTransactionCommit();
}

void fn_vDetachHooks( void )
{
	DetourTransactionBegin();

	DetourDetach((PVOID *)&R2_WndProc, (PVOID)MOD_WndProc);
	DetourDetach((PVOID *)&R2_fn_vEngine, (PVOID)MOD_fn_vEngine);
	DetourDetach((PVOID *)&R2_AGO_vDisplayGAUGES, (PVOID)MOD_AGO_vDisplayGAUGES);
	DetourDetach((PVOID *)&R2_GLI_vComputeTextures, (PVOID)MOD_GLI_vComputeTextures);

	DetourTransactionCommit();
}

BOOL APIENTRY DllMain( HMODULE hModule, DWORD dwReason, LPVOID lpReserved )
{
	switch ( dwReason )
	{
		case DLL_PROCESS_ATTACH:
			fn_vAttachHooks();
			//fn_vInitConsole();
			break;

		case DLL_PROCESS_DETACH:
			fn_vDetachHooks();
			break;

		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
	}
	return TRUE;
}
