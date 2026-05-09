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

enum class ProcessOpcode
{
    Start = 1,
    End = 2
};

extern std::unordered_map<DWORD, ProcessInfo> processCache;

void startProcessMonitoring();
void loadInitialProcesses();
void startRealtimeMonitoring();