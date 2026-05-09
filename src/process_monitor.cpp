#include "process_monitor.h"

#include <krabs.hpp>
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <unordered_map>
#include <string>
#include <mutex>

#pragma comment(lib, "advapi32.lib")

std::unordered_map<DWORD, ProcessInfo> processCache;
std::mutex processCacheMutex;

void startProcessMonitoring()
{
    loadInitialProcesses();
    startRealtimeMonitoring();
}

void loadInitialProcesses()
{
    HANDLE snapshot =
        CreateToolhelp32Snapshot(
            TH32CS_SNAPPROCESS,
            0);

    if (snapshot == INVALID_HANDLE_VALUE)
    {
        std::cout << "Snapshot failed\n";
        return;
    }

    processCache.reserve(512);

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);

    if (Process32FirstW(snapshot, &pe))
    {
        do
        {
            DWORD pid = pe.th32ProcessID;

            processCache.emplace(
                pid,
                ProcessInfo{
                    pid,
                    pe.szExeFile
                });

            std::wcout
                << L" PID: "
                << pid
                << L" NAME: "
                << pe.szExeFile
                
                << L'\n';

        } while (Process32NextW(snapshot, &pe));
    }

    CloseHandle(snapshot);
}


std::wstring getProcessName(
    DWORD pid)
{
    HANDLE hProcess =
        OpenProcess(
            PROCESS_QUERY_LIMITED_INFORMATION,
            FALSE,
            pid);

    if (!hProcess)
    {
        return L"";
    }

    wchar_t path[MAX_PATH];

    DWORD size = MAX_PATH;

    std::wstring processName;

    if (QueryFullProcessImageNameW(
            hProcess,
            0,
            path,
            &size))
    {
        std::wstring fullPath =
            path;

        size_t pos =
            fullPath.find_last_of(
                L"\\/");

        if (pos != std::wstring::npos)
        {
            processName =
                fullPath.substr(
                    pos + 1);
        }
        else
        {
            processName =
                fullPath;
        }
    }

    CloseHandle(hProcess);

    return processName;
}

void handleProcessStart(DWORD pid)
{
    std::wstring processName =
        getProcessName(pid);

    {
        std::lock_guard<std::mutex>
            lock(processCacheMutex);

        processCache[pid] =
        {
            pid,
            processName
        };
    }

    std::wcout
        << L"[START] PID: "
        << pid;

    if (!processName.empty())
    {
        std::wcout
            << L" NAME: "
            << processName;
    }

    std::wcout
        << L'\n';
}

void handleProcessEnd(DWORD pid)
{
    std::wstring processName;

    {
        std::lock_guard<std::mutex>
            lock(processCacheMutex);

        auto it =
            processCache.find(pid);

        if (it != processCache.end())
        {
            processName =
                it->second.name;

            processCache.erase(it);
        }
    }

    std::wcout
        << L"[END] PID: "
        << pid;

    if (!processName.empty())
    {
        std::wcout
            << L" NAME: "
            << processName;
    }

    std::wcout
        << L'\n';
}

void startRealtimeMonitoring()
{
    krabs::kernel_trace trace(
        L"MyKernelTrace");

    krabs::kernel::process_provider
        provider;

    provider.add_on_event_callback(
        [](const EVENT_RECORD& record,
           const krabs::trace_context&
               traceContext)
        {
            const auto opcode =
                record.EventHeader
                    .EventDescriptor
                    .Opcode;

            // Process Start = 1
            // Process End   = 2
            if (opcode != 1 &&
                opcode != 2)
            {
                return;
            }

            DWORD pid = 0;

            try
            {
                krabs::schema schema(
                    record,
                    traceContext
                        .schema_locator);

                krabs::parser parser(
                    schema);

                // REAL PID from payload
                pid =
                    parser.parse<uint32_t>(
                        L"ProcessId");
            }
            catch (...)
            {
                return;
            }

            if (pid == 0)
            {
                return;
            }

            if (opcode == 1)
            {
                handleProcessStart(
                    pid);
            }
            else
            {
                handleProcessEnd(
                    pid);
            }
        });

    trace.enable(provider);

    std::wcout
        << L"Realtime monitoring started...\n";

    trace.start();
}