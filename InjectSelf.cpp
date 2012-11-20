#include <stddef.h>
#include <windows.h>

#define PTR_APPEND_STR(data) { memmove(ptr, data, sizeof(data) - 1); ptr += sizeof(data) - 1; }
#define PTR_APPEND_5BY(instr, dw) { *ptr++ = (instr); *((DWORD*)ptr) = (dw); ptr += 4; }

//DWORD __declspec(dllexport) WINAPI trampoline(LPVOID lpThreadParameter)
//{
//    HMODULE hMod = LoadLibraryW(L"ololo.dll");
//    DWORD result = ((PTHREAD_START_ROUTINE)((DWORD)hMod + (DWORD)0x123456))((LPVOID)"payload");
//    FreeLibrary(hMod);
//    return result;
//}

void WINAPI InjectSelf(DWORD pid, PTHREAD_START_ROUTINE callback, LPVOID payload, int payloadSize, LPDWORD lpExitCode)
{
    HANDLE hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE, false, pid);
    HMODULE hMod;
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)callback, &hMod);

    struct _mLocal {
        unsigned char code[256];
        wchar_t fileName[MAX_PATH];
        char payloadData[1];
    } mLocal;

    int allocSize = sizeof(mLocal) - sizeof(mLocal.payloadData) + payloadSize;
    DWORD pRemote = (DWORD)VirtualAllocEx(hProcess, NULL, allocSize,
        MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

    unsigned char* ptr =  mLocal.code;
//55             PUSH EBP
//8BEC           MOV EBP,ESP
//83EC 08        SUB ESP,8
    PTR_APPEND_STR("\x55\x8B\xEC\x83\xEC\x08");
//68 ????????    PUSH ?   ; FileName
    PTR_APPEND_5BY(0x68, pRemote + offsetof(_mLocal, fileName));
//E8 ????????    CALL ?   ; LoadLibraryW
    HMODULE kernel32 = GetModuleHandleW(L"kernel32");
    DWORD offs = (DWORD)pRemote + (ptr - mLocal.code) + 5;
    PTR_APPEND_5BY(0xE8, (DWORD)GetProcAddress(kernel32, "LoadLibraryW") - offs);
//8945 FC        MOV DWORD PTR SS:[EBP-4],EAX
    PTR_APPEND_STR("\x89\x45\xFC");
//68 ????????    PUSH ?   ; payload
    PTR_APPEND_5BY(0x68, pRemote + offsetof(_mLocal, payloadData));
//8B45 FC        MOV EAX,DWORD PTR SS:[EBP-4]
    PTR_APPEND_STR("\x8B\x45\xFC");
//05 ????????    ADD EAX,?; callback offset
    PTR_APPEND_5BY(0x05, (DWORD)callback - (DWORD)hMod);
//FFD0           CALL EAX
//8945 F8        MOV DWORD PTR SS:[EBP-8],EAX
//8B4D FC        MOV ECX,DWORD PTR SS:[EBP-4]
//51             PUSH ECX ; hLibModule
    PTR_APPEND_STR("\xFF\xD0\x89\x45\xF8\x8B\x4D\xFC\x51");
//E8 ????????    CALL ?   ; FreeLibrary
    offs = (DWORD)pRemote + (ptr - mLocal.code) + 5;
    PTR_APPEND_5BY(0xE8, (DWORD)GetProcAddress(kernel32, "FreeLibrary") - offs);
//8B45 F8        MOV EAX,DWORD PTR SS:[EBP-8]
//8BE5           MOV ESP,EBP
//5D             POP EBP
//C2 0400        RETN 4
    PTR_APPEND_STR("\x8B\x45\xF8\x8B\xE5\x5D\xC2\x04\x00");

    GetModuleFileNameW(hMod, mLocal.fileName, MAX_PATH);

    WriteProcessMemory(hProcess, (void*)pRemote, &mLocal, sizeof(mLocal), NULL);
    WriteProcessMemory(hProcess, (void*)(pRemote + offsetof(_mLocal, payloadData)), payload, payloadSize, NULL);

    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (PTHREAD_START_ROUTINE)pRemote, 0, 0, NULL);
    WaitForSingleObject(hThread, INFINITE);
    if (lpExitCode)
        GetExitCodeThread(hThread, lpExitCode);
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, (void*)pRemote, allocSize, MEM_RELEASE);
    CloseHandle(hProcess);
}
