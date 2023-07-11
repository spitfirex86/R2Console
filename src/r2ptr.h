#pragma once

#include "framework.h"


LRESULT CALLBACK MOD_WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
void MOD_AGO_vDisplayGAUGES( GLD_tdstViewportAttributes *p_stVpt );
void MOD_fn_vEngine( void );
