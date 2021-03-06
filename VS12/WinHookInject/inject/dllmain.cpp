// dllmain.cpp : Defines the entry point for the DLL application.
#include <windows.h>
#include <stdio.h>


#include "stdafx.h"

#define SIZE 6

typedef int (WINAPI *pMessageBoxA)(HWND, LPTSTR, LPTSTR, UINT);
int WINAPI MyMessageBoxA(HWND, LPTSTR, LPTSTR, UINT);

void BeginRedirect(LPVOID);

pMessageBoxA pOrigMBAddress = NULL;
BYTE oldBytes[SIZE] = {0};
BYTE JMP[SIZE] = {0};
DWORD oldProtect, myProtect = PAGE_EXECUTE_READWRITE;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	char debugBuffer[128];
    // We do not need to know about Threaded events, so disable them (reduces overhead)
    //DisableThreadLibraryCalls(hDLL);
    
    OutputDebugString("In dll");
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		OutputDebugString("Attaching");

		pOrigMBAddress = (pMessageBoxA)GetProcAddress(GetModuleHandle("user32.dll"), "MessageBoxA");

		sprintf_s(debugBuffer, 128, "pOrigMBAddress: %x", pOrigMBAddress);
		OutputDebugString(debugBuffer);

		if(pOrigMBAddress != NULL)
			BeginRedirect((LPVOID)MyMessageBoxA);    
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		OutputDebugString("Detaching");
        memcpy((void *)pOrigMBAddress, (void *)oldBytes, SIZE);
		break;
	}
	return TRUE;
}

void BeginRedirect(LPVOID newFunction)
{
    char debugBuffer[128];
    OutputDebugString("Redirecting");
    BYTE tempJMP[SIZE] = {0xE9, 0x90, 0x90, 0x90, 0x90, 0xC3};
    memcpy(JMP, tempJMP, SIZE);
    DWORD JMPSize = ((DWORD)newFunction - (DWORD)pOrigMBAddress - 5);
    VirtualProtect((LPVOID)pOrigMBAddress, SIZE, 
                    PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy((void *)oldBytes, (void *)pOrigMBAddress, SIZE);

    sprintf_s(debugBuffer, 128, "Old bytes: %x%x%x%x%x", oldBytes[0], oldBytes[1], oldBytes[2], oldBytes[3], oldBytes[4], oldBytes[5]);
    OutputDebugString(debugBuffer);

    memcpy(&JMP[1], &JMPSize, 4);

    sprintf_s(debugBuffer, 128, "JMP: %x%x%x%x%x", JMP[0], JMP[1], JMP[2], JMP[3], JMP[4], JMP[5]);
    OutputDebugString(debugBuffer);

    memcpy((void *)pOrigMBAddress, (void *)JMP, SIZE);
    VirtualProtect((LPVOID)pOrigMBAddress, SIZE, oldProtect, NULL);
}

int  WINAPI MyMessageBoxA(HWND hWnd, LPTSTR lpText, LPTSTR lpCaption, UINT uiType)
{
    OutputDebugString("In MyMessageBoxA");
    lpCaption = "Hooked!";
    // unhook
    VirtualProtect((LPVOID)pOrigMBAddress, SIZE, myProtect, NULL);
    memcpy((LPVOID)pOrigMBAddress, oldBytes, SIZE);
    int retValue = MessageBoxA(hWnd, lpText, lpCaption, uiType);
    // hook
    memcpy((LPVOID)pOrigMBAddress, JMP, SIZE);
    VirtualProtect((LPVOID)pOrigMBAddress, SIZE, oldProtect, NULL);
    return retValue;
}


