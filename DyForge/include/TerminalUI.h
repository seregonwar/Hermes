#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <Windows.h>
#include "../../DyMain/include/DyMain.h"

namespace DyForge {

class TerminalUI {
public:
    TerminalUI();
    ~TerminalUI();

    // Core functionality
    bool Initialize();
    void Cleanup();
    bool IsInitialized() const;

    // Process management
    bool OpenProcess(DWORD processId);
    bool CloseProcess();
    bool IsProcessOpen() const;
    DWORD GetCurrentProcessId() const;

    // Injection management
    bool InjectDll(const std::wstring& dllPath);
    bool UnloadDll();
    bool IsDllInjected() const;

    // Analysis management
    bool StartAnalysis();
    bool StopAnalysis();
    bool IsAnalysisRunning() const;

    // Reporting
    bool GenerateReport(const std::wstring& outputPath);
    bool ExportData(const std::wstring& format, const std::wstring& outputPath);

    // Event handling
    using EventCallback = std::function<void(const std::string&)>;
    void RegisterEventCallback(const std::string& eventName, EventCallback callback);
    void UnregisterEventCallback(const std::string& eventName);

    // Menu principale
    void ShowMainMenu();
    void ShowProcessSelection();
    void ShowCommandMenu();
    void ShowAnalysisMenu();
    void ShowModMenu();
    void ShowWebServerMenu();

    // Gestione processi
    bool SelectProcess(DWORD& processId);
    bool InjectDLL(DWORD processId);
    bool EjectDLL(DWORD processId);

    // Comandi
    bool StartWebServer();
    bool StopWebServer();
    bool LoadMod(const std::string& modPath);
    bool UnloadMod(const std::string& modName);

private:
    // Prevent copying
    TerminalUI(const TerminalUI&) = delete;
    TerminalUI& operator=(const TerminalUI&) = delete;

    // Internal state
    bool m_initialized;
    HANDLE m_stdin;
    HANDLE m_stdout;
    HANDLE m_stderr;

    // Event handling
    std::unordered_map<std::string, std::vector<EventCallback>> m_eventCallbacks;

    // Stato interno
    DWORD selectedProcessId;
    bool isDLLInjected;
    bool isAnalysisRunning;
    bool isWebServerRunning;

    // Funzioni di utilità
    void ClearScreen();
    void PrintHeader();
    void PrintProcessList();
    void PrintStatus();
    void WaitForKey();
    std::string GetInput(const std::string& prompt);
    bool ConfirmAction(const std::string& message);

    // Gestione colori
    void SetColor(int color);
    void ResetColor();

    // Costanti
    static const int COLOR_DEFAULT = 7;
    static const int COLOR_HEADER = 11;
    static const int COLOR_SUCCESS = 10;
    static const int COLOR_ERROR = 12;
    static const int COLOR_WARNING = 14;
    static const int COLOR_INFO = 9;
};

} // namespace DyForge 