#include "framework.h"
#include "r2ptr.h"
#include "console.h"


void fn_vAttachHooks( void )
{
	FHK_fn_lCreateHook((void **)&GAM_fn_WndProc, (void *)MOD_WndProc);
	FHK_fn_lCreateHook((void **)&GAM_fn_vEngine, (void *)MOD_fn_vEngine);
	FHK_fn_lCreateHook((void **)&AGO_vDisplayGAUGES, (void *)MOD_AGO_vDisplayGAUGES);
	FHK_fn_lCreateHook((void **)&GLI_vComputeTextures, (void *)MOD_GLI_vComputeTextures);
}

void fn_vDetachHooks( void )
{
	FHK_fn_lDestroyHook((void **)&GAM_fn_WndProc, (void *)MOD_WndProc);
	FHK_fn_lDestroyHook((void **)&GAM_fn_vEngine, (void *)MOD_fn_vEngine);
	FHK_fn_lDestroyHook((void **)&AGO_vDisplayGAUGES, (void *)MOD_AGO_vDisplayGAUGES);
	FHK_fn_lDestroyHook((void **)&GLI_vComputeTextures, (void *)MOD_GLI_vComputeTextures);
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
