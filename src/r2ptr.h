#pragma once

#include "framework.h"


extern WNDPROC R2_WndProc;
extern void (*R2_AGO_vDisplayGAUGES)( GLD_tdstViewportAttributes *p_stVpt );
extern void (*R2_fn_vEngine)( void );
extern void (*R2_GLI_vComputeTextures)( void );

extern DWORD *R2_ulNumberOfEntryElement;


LRESULT CALLBACK MOD_WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
void MOD_AGO_vDisplayGAUGES( GLD_tdstViewportAttributes *p_stVpt );
void MOD_fn_vEngine( void );
void MOD_GLI_vComputeTextures( void );
