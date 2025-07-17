#define UNICODE
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>

DWORD GetNotepadPID() {
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Process32First(hSnapshot, &pe32)) {
        do {
            if (_wcsicmp(pe32.szExeFile, L"notepad.exe") == 0) {
                CloseHandle(hSnapshot);
                return pe32.th32ProcessID;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }
    CloseHandle(hSnapshot);
    return 0;
}

bool InjectDLL(DWORD pid, const char* dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) {
        std::cout << "Failed to open process. Error: " << GetLastError() << std::endl;
        return false;
    }

    LPVOID pRemoteMem = VirtualAllocEx(hProcess, NULL, strlen(dllPath) + 1, MEM_COMMIT, PAGE_READWRITE);
    if (!pRemoteMem) {
        std::cout << "Failed to allocate memory. Error: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    if (!WriteProcessMemory(hProcess, pRemoteMem, dllPath, strlen(dllPath) + 1, NULL)) {
        std::cout << "Failed to write memory. Error: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pRemoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    HMODULE hKernel32 = GetModuleHandle(L"kernel32.dll");
    LPTHREAD_START_ROUTINE pLoadLibrary = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryA");

    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, pLoadLibrary, pRemoteMem, 0, NULL);
    if (!hThread) {
        std::cout << "Failed to create remote thread. Error: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pRemoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return false;
    }

    WaitForSingleObject(hThread, INFINITE);

    // Get LoadLibrary return value, check if DLL loaded successfully
    DWORD exitCode;
    GetExitCodeThread(hThread, &exitCode);
    if (exitCode == 0) {
        std::cout << "DLL failed to load in target process." << std::endl;
        VirtualFreeEx(hProcess, pRemoteMem, 0, MEM_RELEASE);
        CloseHandle(hThread);
        CloseHandle(hProcess);
        return false;
    }

    VirtualFreeEx(hProcess, pRemoteMem, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);
    return true;
}

int main() {
    DWORD pid = GetNotepadPID();
    if (!pid) {
        std::cout << "Not found notepad.exe process!" << std::endl;
        std::cout << "Please start notepad.exe first." << std::endl;
        return 1;
    }

    std::cout << "Found notepad.exe with PID: " << pid << std::endl;

    // Get current program directory and build absolute path for DLL
    char currentDir[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, currentDir);
    
    char dllPath[MAX_PATH];
    sprintf_s(dllPath, "%s\\HookNotepad.dll", currentDir);
    
    std::cout << "DLL Path: " << dllPath << std::endl;

    // Check if DLL file exists
    DWORD fileAttr = GetFileAttributesA(dllPath);
    if (fileAttr == INVALID_FILE_ATTRIBUTES) {
        std::cout << "DLL file not found: " << dllPath << std::endl;
        return 1;
    }

    if (InjectDLL(pid, dllPath)) {
        std::cout << "DLL inject success!" << std::endl;
    } else {
        std::cout << "DLL inject failed!" << std::endl;
    }

    return 0;
}
