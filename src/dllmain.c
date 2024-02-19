#include "framework.h"
#include "r2ptr.h"
#include "console.h"


void fn_vAttachHooks( void )
{
	FHK_M_lCreateHook(&GAM_fn_WndProc, MOD_WndProc);
	FHK_M_lCreateHook(&GAM_fn_vEngine, MOD_fn_vEngine);
	FHK_M_lCreateHook(&AGO_vDisplayGAUGES, MOD_AGO_vDisplayGAUGES);
}

void fn_vDetachHooks( void )
{
	FHK_M_lDestroyHook(&GAM_fn_WndProc, MOD_WndProc);
	FHK_M_lDestroyHook(&GAM_fn_vEngine, MOD_fn_vEngine);
	FHK_M_lDestroyHook(&AGO_vDisplayGAUGES, MOD_AGO_vDisplayGAUGES);
}

__declspec(dllexport)
int ModMain( BOOL bInit )
{
	if ( bInit )
	{
		fn_vAttachHooks();
		fn_vEarlyInitConsole();
	}
	else
	{
		fn_vDetachHooks();
	}

	return 0;
}
