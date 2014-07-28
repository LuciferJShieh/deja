#define _WIN32_IE 0x600
#define _WIN32_WINNT 0x600
#include <stdio.h>
#include <windows.h>
#include <winbase.h>
#include <tchar.h>
#include <wininet.h>

wchar_t tzDllPath[MAX_PATH + 1];

#include <lua.hpp>
#include "Util.h"
#include "QQPatchTool.h"
#include "QQHookEngine.h"
#include "InjectDll.h"
#include "SkinsPatch.h"
#include "split.h"
#include "BossKey.h"

#include <iup.h>
#include <iuplua.h>

#define EXTERNC extern "C"
#define EXPORT EXTERNC __declspec(dllexport) void __cdecl

EXPORT vSetDdrawflag() {__asm__(".byte 0x90,0x90,0x90,0x90");}
EXPORT AlphaBlend() {__asm__(".byte 0x90,0x90,0x90,0x90");}
EXPORT DllInitialize() {__asm__(".byte 0x90,0x90,0x90,0x90");}
EXPORT GradientFill() {__asm__(".byte 0x90,0x90,0x90,0x90");}
EXPORT TransparentBlt() {__asm__(".byte 0x90,0x90,0x90,0x90");}

void WriteJMP(DWORD TargetProc, DWORD NewProc)
{
	BYTE JMP = 0xE9;
	WriteProcessMemory((void*)-1, (LPVOID)TargetProc, &JMP, sizeof(JMP), NULL);
	DWORD offset = NewProc - TargetProc - 5;
	WriteProcessMemory((void*)-1, (LPVOID)(TargetProc + 1), &offset, sizeof(offset), NULL);
}

void MSIMG32_HOOK()
{
	TCHAR szDLL[MAX_PATH+1] = {0};
	GetSystemDirectory(szDLL, MAX_PATH);
	lstrcat(szDLL, TEXT("\\msimg32.dll"));
	HINSTANCE hDll = LoadLibrary(szDLL);
	if (hDll != NULL)
	{
		WriteJMP((DWORD)AlphaBlend, (DWORD)GetProcAddress(hDll, "AlphaBlend"));
		WriteJMP((DWORD)GradientFill, (DWORD)GetProcAddress(hDll, "GradientFill"));
		WriteJMP((DWORD)vSetDdrawflag, (DWORD)GetProcAddress(hDll, "vSetDdrawflag"));
		WriteJMP((DWORD)DllInitialize, (DWORD)GetProcAddress(hDll, "DllInitialize"));
		WriteJMP((DWORD)TransparentBlt, (DWORD)GetProcAddress(hDll, "TransparentBlt"));
	}
}

wchar_t MyAbout[MAX_PATH];
void ChangeMyAbout(long offset)
{
	if(*(BYTE*)offset==0x68)
	{
		DWORD addr = *(DWORD*)(offset+1);
		wchar_t * str = (wchar_t *)addr;
		wcscpy(MyAbout,str);
		wcscat(MyAbout,L" with deja");

		//DbgPrint(L"%X %s",addr,MyAbout);

		addr = (DWORD)&MyAbout;
		WriteProcessMemory((void*)-1,(void*)(offset+1), &addr, sizeof(addr), NULL);
	}
}

typedef DWORD (WINAPI* MYMoveFileWithProgressW)(LPCWSTR lpFileName,LPCWSTR lpNewFileName,LPPROGRESS_ROUTINE lpProgressRoutine,LPVOID lpData,DWORD dwFlags);
MYMoveFileWithProgressW OldMoveFileWithProgressW = NULL;

DWORD WINAPI MyMoveFileWithProgressW(LPCWSTR lpFileName,LPCWSTR lpNewFileName,LPPROGRESS_ROUTINE lpProgressRoutine,LPVOID lpData,DWORD dwFlags)
{
	if(isEndWith(lpFileName, L"msimg32.dll")) return 0;
	if(OldMoveFileWithProgressW) return OldMoveFileWithProgressW(lpFileName,lpNewFileName,lpProgressRoutine,lpData,dwFlags);
	return 0;
}
