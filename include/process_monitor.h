#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <unordered_map>
#include <string>

struct ProcessInfo
{
    DWORD pid;
    std::wstring name;
};

extern std::unordered_map<DWORD, ProcessInfo> processCache;

void startProcessMonitoring();
void loadInitialProcesses();
std::wstring getProcessName(DWORD pid);
void handleProcessEnd(DWORD pid);
void handleProcessStart(DWORD pid);
void startRealtimeMonitoring();