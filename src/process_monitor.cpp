#include "process_monitor.h"

#include <krabs.hpp>
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>

std::unordered_map<DWORD, ProcessInfo> processCache;

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
                << L"Loaded: "
                << pe.szExeFile
                << L" PID: "
                << pid
                << L'\n';

        } while (Process32NextW(snapshot, &pe));
    }

    CloseHandle(snapshot);
}

void startRealtimeMonitoring()
{
    // Kernel trace session
    krabs::kernel_trace trace(L"MyTrace");

    // Process provider
    krabs::kernel::process_provider provider;

    // Callback for every process event
    provider.add_on_event_callback(
        [](const EVENT_RECORD& record,
           const krabs::trace_context& traceContext)
        {
            try
            {
                krabs::schema schema(
                    record,
                    traceContext.schema_locator);

                krabs::parser parser(schema);

                // Process ID
                DWORD pid =
                    parser.parse<uint32_t>(L"ProcessId");

                // Process name
                auto processName =
                    parser.parse<std::wstring>(
                        L"ImageFileName");

                std::wcout
                    << "PID: "
                    << pid
                    << " | "
                    << processName
                    << '\n';
            }
            catch (...)
            {
                // Ignore parsing failures
            }
        });

    trace.enable(provider);

    std::cout << "Realtime monitoring started...\n";

    // Blocking call
    trace.start();
}