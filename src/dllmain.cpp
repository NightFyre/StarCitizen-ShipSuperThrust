#pragma once
#include <windows.h>
#include <iostream>
#include <TlHelp32.h>
#include <vector>
#include <detours.h>

namespace process {
    uintptr_t moduleBase;
}

namespace offsets {
    int sTHRUST = 0x42498A0;    //  patch 3.15.1 (PRE EAC)
}


namespace ship {
    bool bTHRUST = false;   //  bool for enable/disable
    float sfTHRUST;         //  Thrust Amount when Function Hooked
}

/// <summary>
/// Define Functions
/// Credit: XBOX360LSBEST
/// </summary>
HMODULE baseGame = NULL;
__int64 GetAddr(INT64 addr)
{
    if (baseGame == NULL)
        baseGame = GetModuleHandleA(NULL);
    return (__int64)baseGame + addr;
}
#define DefineFunction(functionName, returnType, params, functionAddr) returnType(*functionName)params = (decltype(functionName))GetAddr(functionAddr)

//  HOOKS
//  CREDITS
//  ACETOPY: Finding Instruction 0x42498F3 "vmovss  xmm13, dword ptr [rax]"
//  In his Cheat Table Script he places (float)-1 in RAX before that instruction is fired.
//  
//  I instead hook the whole function and use the registers to control the thrusters.
//  RAX and R8 both hold a container that controls the ships thrust
//  RAX Controls one side of the ship while R8 controls the other side of the ship
DefineFunction(ShipThrust, int, (INT64 _RCX, INT64 _RDX, INT64 _R8), offsets::sTHRUST);
int ShipThrust_hook(INT64 _RCX, INT64 _RDX, INT64 _R8)
{
    if (ship::bBOOST)
    {
        INT64 _RAX = _R8 + 0xC;
        *(int*)_R8 = ship::sfTHRUST;
        *(int*)_RAX = ship::sfTHRUST;
        return ShipThrust(_RCX, _RDX, _R8);
    }
    else
        return ShipThrust(_RCX, _RDX, _R8);
}

//  MAIN THREAD
void appmain(HMODULE hModule)
{
    //Function Hook begin
    DetourTransactionBegin();
    DetourAttach((void**)&ShipThrust, ShipThrust_hook);
    DetourTransactionCommit();

    //Gather Process Information
    process::moduleBase = (uintptr_t)GetModuleHandle(NULL);
    uintptr_t shipTHRUST = process::moduleBase + offsets::sTHRUST;

    //  DEBUG INFO
    INT32 a = *(INT32*)process::moduleBase;
    INT32 b = *(INT32*)shipTHRUST;
    printf("TARGET PROCESS INFORMATION \n");
    printf("[+] ModuleBase            => %X ", process::moduleBase, 8);
    printf("| VALUE: %X \n", a);
    printf("[+] ShipThrustAddress     => %X ", shipTHRUST, 8);
    printf("| VALUE: %X \n", b);
    
    //  DISPLAY CONTROLS
    printf("[+] PRESS [END] TO QUIT or PRESS [NUMPAD1] TO TEST HOOK THRUST \n");

    //  Start
    while (true)
    {
        //  QUIT
        if (GetAsyncKeyState(VK_END) & 1) 
        {
            printf("\n");
            printf("EXITING, PERFORMING CLEANUP");
            if (ship::bBOOST) !ship::bBOOST;
            Sleep(100);
            break;
        }

        //  ENABLE SHIP SPEED
        if (GetAsyncKeyState(VK_NUMPAD1) & 1)
        {
            ship::bBOOST = !ship::bBOOST;
            if (ship::bBOOST)
            {
                printf("HOOK ENABLED \n");
                ship::sfTHRUST = 10;
                printf("THRUST SPEED: %f \n", ship::sfTHRUST);
            }
            else
                printf("HOOK DISABLED , THRUST RESET \n");
        }

        //  LOOP FOR ADJUSTMENTS
        if (ship::bBOOST)
        {
            if (GetAsyncKeyState(VK_UP) & 1)
            {
                if (ship::sfTHRUST <= 90)
                {
                    ship::sfTHRUST = (ship::sfTHRUST + 10);
                    printf("SHIP THRUST: %f \n", ship::sfTHRUST);
                }
                else
                    printf("LIMIT EXCEEDED!! CURRENT THRUST: %f \n", ship::sfTHRUST);
                
            }

            if (GetAsyncKeyState(VK_DOWN) & 1)
            {
                if (ship::sfTHRUST >= 10)
                {
                    ship::sfTHRUST = (ship::sfTHRUST - 10);
                    printf("SHIP THRUST: %f \n", ship::sfTHRUST);
                }
                else
                    printf("LIMIT EXCEEDED!! CURRENT THRUST: %f \n", ship::sfTHRUST);
            }
        }
    }
}

//INIT
DWORD WINAPI HackThread(HMODULE hModule)
{
    //Create Console
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    SetConsoleTitle("NightFyre | DEBUG CONSOLE");
    Sleep(1000);
    appmain(hModule);

    //USER CLOSED
    DetourTransactionBegin();
    DetourDetach((void**)&ShipThrust, ShipThrust_hook);
    DetourTransactionCommit();

    Sleep(1000);
    fclose(f);
    FreeConsole();
    FreeLibraryAndExitThread(hModule, 0);
    return 0;
}

//DLL ENTRY
BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)HackThread, hModule, 0, nullptr));
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
