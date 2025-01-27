#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <detours.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <math.h>

#include <ACP_Ray2.h>


#ifdef R2CONSOLE_EXPORTS
#define R2CON_API __declspec(dllexport)
#else
#define R2CON_API __declspec(dllimport)
#endif
