#pragma once

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>
#include "../DyHexInject/DyHexInject.h"

namespace DyForge {

// Forward declarations
class ProcessManager;
class InjectionManager;
class AnalysisManager;
class ReportingManager;
class UIManager;

// Main application class
class DyForge {
public:
    static DyForge& GetInstance();
    
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
    
    // UI management
    bool ShowMainWindow();
    bool HideMainWindow();
    bool IsMainWindowVisible() const;
    
private:
    DyForge();
    ~DyForge();
    
    // Prevent copying
    DyForge(const DyForge&) = delete;
    DyForge& operator=(const DyForge&) = delete;
    
    // Internal state
    bool m_initialized;
    std::unique_ptr<ProcessManager> m_processManager;
    std::unique_ptr<InjectionManager> m_injectionManager;
    std::unique_ptr<AnalysisManager> m_analysisManager;
    std::unique_ptr<ReportingManager> m_reportingManager;
    std::unique_ptr<UIManager> m_uiManager;
    
    // Event handling
    std::unordered_map<std::string, std::vector<EventCallback>> m_eventCallbacks;
};

// Process management class
class ProcessManager {
public:
    ProcessManager();
    ~ProcessManager();
    
    bool OpenProcess(DWORD processId);
    void CloseProcess();
    bool IsProcessOpen() const;
    DWORD GetCurrentProcessId() const;
    
    // Access to process info for other managers
    const DyHexInjectProcessInfo& GetProcessInfo() const { return m_processInfo; }
    
private:
    DWORD m_processId;
    HANDLE m_processHandle;
    DyHexInjectProcessInfo m_processInfo;
};

// Injection management class
class InjectionManager {
public:
    InjectionManager(ProcessManager& processManager);
    ~InjectionManager();
    
    bool InjectDll(const std::wstring& dllPath);
    bool UnloadDll();
    bool IsDllInjected() const;
    
    // Access to communication handle for other managers
    HANDLE GetCommunicationHandle() const { return m_communicationHandle; }
    
    // Access to communication structure for other managers
    DyHexInjectCommunication* GetCommunication() { return &m_communication; }
    
    // Access to process manager for other managers
    ProcessManager& GetProcessManager() { return m_processManager; }
    
private:
    ProcessManager& m_processManager;
    bool m_dllInjected;
    HANDLE m_communicationHandle;
    DyHexInjectCommunication m_communication;
};

// Analysis management class
class AnalysisManager {
public:
    AnalysisManager(InjectionManager& injectionManager);
    ~AnalysisManager();
    
    bool StartAnalysis();
    bool StopAnalysis();
    bool IsAnalysisRunning() const;
    
    // Access to injection manager for other managers
    InjectionManager& GetInjectionManager() { return m_injectionManager; }
    
private:
    InjectionManager& m_injectionManager;
    bool m_analysisRunning;
};

// Reporting management class
class ReportingManager {
public:
    ReportingManager(AnalysisManager& analysisManager);
    ~ReportingManager();
    
    bool GenerateReport(const std::wstring& outputPath);
    bool ExportData(const std::wstring& format, const std::wstring& outputPath);
    
private:
    AnalysisManager& m_analysisManager;
};

// UI management class
class UIManager {
public:
    UIManager(DyForge& dyForge);
    ~UIManager();
    
    bool ShowMainWindow();
    bool HideMainWindow();
    bool IsMainWindowVisible() const;
    
private:
    // Window procedure and message handling
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleCommand(int id);
    LRESULT HandleNotify(LPNMHDR pnmh);
    LRESULT HandleSize(int width, int height);
    
    // Control creation and management
    void CreateControls();
    void UpdateProcessList();
    void UpdateButtonStates();
    void ShowError(const std::wstring& message);
    void ShowStatus(const std::wstring& message);
    
    // File dialogs
    std::wstring ShowOpenDllDialog();
    std::wstring ShowSaveReportDialog();
    std::wstring ShowSaveExportDialog();
    
    // Event handlers
    void OnProcessSelected();
    void OnInjectDll();
    void OnStartAnalysis();
    void OnStopAnalysis();
    void OnGenerateReport();
    void OnExportData();
    
    // Member variables
    DyForge& m_dyForge;
    bool m_windowVisible;
    HWND m_hwnd;
    HWND m_hwndProcessList;
    HWND m_hwndInjectButton;
    HWND m_hwndStartAnalysisButton;
    HWND m_hwndStopAnalysisButton;
    HWND m_hwndGenerateReportButton;
    HWND m_hwndExportDataButton;
    HWND m_hwndStatusBar;
    
    // Constants
    static const wchar_t* WINDOW_CLASS_NAME;
    static const int WINDOW_WIDTH;
    static const int WINDOW_HEIGHT;
    
    // Control IDs
    enum {
        IDC_PROCESS_LIST = 1001,
        IDC_INJECT_BUTTON = 1002,
        IDC_START_ANALYSIS_BUTTON = 1003,
        IDC_STOP_ANALYSIS_BUTTON = 1004,
        IDC_GENERATE_REPORT_BUTTON = 1005,
        IDC_EXPORT_DATA_BUTTON = 1006,
        IDC_STATUS_BAR = 1007
    };
};

} // namespace DyForge 