#include "../../include/DyForge.h"
#include <stdexcept>
#include <sstream>
#include <windowsx.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <commdlg.h>
#include <tlhelp32.h>
#include <psapi.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "psapi.lib")

namespace DyForge {

// Constants
const wchar_t* UIManager::WINDOW_CLASS_NAME = L"DyForgeMainWindow";
const int UIManager::WINDOW_WIDTH = 800;
const int UIManager::WINDOW_HEIGHT = 600;

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

// Constructor
UIManager::UIManager(DyForge& dyForge)
    : m_dyForge(dyForge)
    , m_windowVisible(false)
    , m_hwnd(NULL)
    , m_hwndProcessList(NULL)
    , m_hwndInjectButton(NULL)
    , m_hwndStartAnalysisButton(NULL)
    , m_hwndStopAnalysisButton(NULL)
    , m_hwndGenerateReportButton(NULL)
    , m_hwndExportDataButton(NULL)
    , m_hwndStatusBar(NULL) {
}

// Destructor
UIManager::~UIManager() {
    if (m_windowVisible) {
        HideMainWindow();
    }
}

// UI management
bool UIManager::ShowMainWindow() {
    if (m_windowVisible) {
        return true;
    }
    
    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    
    if (!RegisterClassExW(&wc)) {
        throw std::runtime_error("Failed to register window class");
    }
    
    // Create main window
    m_hwnd = CreateWindowExW(
        0,
        WINDOW_CLASS_NAME,
        L"DyForge - Dynamic Process Sculptor",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        NULL,
        NULL,
        GetModuleHandle(NULL),
        this
    );
    
    if (!m_hwnd) {
        throw std::runtime_error("Failed to create main window");
    }
    
    // Initialize common controls
    INITCOMMONCONTROLSEX icex = {};
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icex);
    
    // Create controls
    CreateControls();
    
    // Show window
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);
    
    m_windowVisible = true;
    return true;
}

bool UIManager::HideMainWindow() {
    if (!m_windowVisible) {
        return true;
    }
    
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }
    
    m_windowVisible = false;
    return true;
}

bool UIManager::IsMainWindowVisible() const {
    return m_windowVisible;
}

// Private methods
void UIManager::CreateControls() {
    // Create process list
    m_hwndProcessList = CreateWindowExW(
        0,
        WC_LISTVIEWW,
        L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
        10,
        10,
        WINDOW_WIDTH - 20,
        200,
        m_hwnd,
        (HMENU)IDC_PROCESS_LIST,
        GetModuleHandle(NULL),
        NULL
    );
    
    // Add columns to process list
    LVCOLUMNW lvc = {};
    lvc.mask = LVCF_TEXT | LVCF_WIDTH;
    
    lvc.pszText = (LPWSTR)L"PID";
    lvc.cx = 80;
    ListView_InsertColumn(m_hwndProcessList, 0, &lvc);
    
    lvc.pszText = (LPWSTR)L"Name";
    lvc.cx = 200;
    ListView_InsertColumn(m_hwndProcessList, 1, &lvc);
    
    lvc.pszText = (LPWSTR)L"Path";
    lvc.cx = 300;
    ListView_InsertColumn(m_hwndProcessList, 2, &lvc);
    
    // Create buttons
    m_hwndInjectButton = CreateWindowExW(
        0,
        L"BUTTON",
        L"Inject DLL",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10,
        220,
        120,
        30,
        m_hwnd,
        (HMENU)IDC_INJECT_BUTTON,
        GetModuleHandle(NULL),
        NULL
    );
    
    m_hwndStartAnalysisButton = CreateWindowExW(
        0,
        L"BUTTON",
        L"Start Analysis",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        140,
        220,
        120,
        30,
        m_hwnd,
        (HMENU)IDC_START_ANALYSIS_BUTTON,
        GetModuleHandle(NULL),
        NULL
    );
    
    m_hwndStopAnalysisButton = CreateWindowExW(
        0,
        L"BUTTON",
        L"Stop Analysis",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        270,
        220,
        120,
        30,
        m_hwnd,
        (HMENU)IDC_STOP_ANALYSIS_BUTTON,
        GetModuleHandle(NULL),
        NULL
    );
    
    m_hwndGenerateReportButton = CreateWindowExW(
        0,
        L"BUTTON",
        L"Generate Report",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        400,
        220,
        120,
        30,
        m_hwnd,
        (HMENU)IDC_GENERATE_REPORT_BUTTON,
        GetModuleHandle(NULL),
        NULL
    );
    
    m_hwndExportDataButton = CreateWindowExW(
        0,
        L"BUTTON",
        L"Export Data",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        530,
        220,
        120,
        30,
        m_hwnd,
        (HMENU)IDC_EXPORT_DATA_BUTTON,
        GetModuleHandle(NULL),
        NULL
    );
    
    // Create status bar
    m_hwndStatusBar = CreateWindowExW(
        0,
        STATUSCLASSNAMEW,
        NULL,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0,
        0,
        0,
        0,
        m_hwnd,
        (HMENU)IDC_STATUS_BAR,
        GetModuleHandle(NULL),
        NULL
    );
    
    // Set status bar text
    ShowStatus(L"Ready");
    
    // Update button states
    UpdateButtonStates();
    
    // Update process list
    UpdateProcessList();
}

void UIManager::UpdateProcessList() {
    // Clear existing items
    ListView_DeleteAllItems(m_hwndProcessList);
    
    // Get process list
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        ShowError(L"Failed to create process snapshot");
        return;
    }
    
    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(pe32);
    
    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            // Add process to list
            LVITEMW lvi = {};
            lvi.mask = LVIF_TEXT;
            
            // PID
            wchar_t pidStr[16];
            swprintf_s(pidStr, L"%lu", pe32.th32ProcessID);
            lvi.iItem = ListView_GetItemCount(m_hwndProcessList);
            lvi.iSubItem = 0;
            lvi.pszText = pidStr;
            ListView_InsertItem(m_hwndProcessList, &lvi);
            
            // Name
            lvi.iSubItem = 1;
            lvi.pszText = pe32.szExeFile;
            ListView_SetItem(m_hwndProcessList, &lvi);
            
            // Path
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
            if (hProcess) {
                wchar_t path[MAX_PATH];
                if (GetModuleFileNameExW(hProcess, NULL, path, MAX_PATH)) {
                    lvi.iSubItem = 2;
                    lvi.pszText = path;
                    ListView_SetItem(m_hwndProcessList, &lvi);
                }
                CloseHandle(hProcess);
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }
    
    CloseHandle(hSnapshot);
}

void UIManager::UpdateButtonStates() {
    bool processOpen = m_dyForge.IsProcessOpen();
    bool dllInjected = m_dyForge.IsDllInjected();
    bool analysisRunning = m_dyForge.IsAnalysisRunning();
    
    EnableWindow(m_hwndInjectButton, processOpen && !dllInjected);
    EnableWindow(m_hwndStartAnalysisButton, dllInjected && !analysisRunning);
    EnableWindow(m_hwndStopAnalysisButton, analysisRunning);
    EnableWindow(m_hwndGenerateReportButton, analysisRunning);
    EnableWindow(m_hwndExportDataButton, analysisRunning);
}

void UIManager::ShowError(const std::wstring& message) {
    MessageBoxW(m_hwnd, message.c_str(), L"Error", MB_ICONERROR | MB_OK);
}

void UIManager::ShowStatus(const std::wstring& message) {
    SendMessageW(m_hwndStatusBar, SB_SETTEXT, 0, (LPARAM)message.c_str());
}

// File dialogs
std::wstring UIManager::ShowOpenDllDialog() {
    wchar_t filename[MAX_PATH] = L"";
    
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = L"DLL Files\0*.dll\0All Files\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = L"dll";
    
    if (GetOpenFileNameW(&ofn)) {
        return filename;
    }
    
    return L"";
}

std::wstring UIManager::ShowSaveReportDialog() {
    wchar_t filename[MAX_PATH] = L"";
    
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = L"Text Files\0*.txt\0All Files\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = L"txt";
    
    if (GetSaveFileNameW(&ofn)) {
        return filename;
    }
    
    return L"";
}

std::wstring UIManager::ShowSaveExportDialog() {
    wchar_t filename[MAX_PATH] = L"";
    
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = L"JSON Files\0*.json\0CSV Files\0*.csv\0All Files\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = L"json";
    
    if (GetSaveFileNameW(&ofn)) {
        return filename;
    }
    
    return L"";
}

// Event handlers
void UIManager::OnProcessSelected() {
    int selectedIndex = ListView_GetNextItem(m_hwndProcessList, -1, LVNI_SELECTED);
    if (selectedIndex >= 0) {
        wchar_t pidStr[16];
        LVITEMW lvi = {};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = selectedIndex;
        lvi.iSubItem = 0;
        lvi.pszText = pidStr;
        lvi.cchTextMax = sizeof(pidStr) / sizeof(wchar_t);
        
        if (ListView_GetItem(m_hwndProcessList, &lvi)) {
            DWORD processId = _wtoi(pidStr);
            if (m_dyForge.OpenProcess(processId)) {
                ShowStatus(L"Process opened successfully");
                UpdateButtonStates();
            }
            else {
                ShowError(L"Failed to open process");
            }
        }
    }
}

void UIManager::OnInjectDll() {
    std::wstring dllPath = ShowOpenDllDialog();
    if (!dllPath.empty()) {
        if (m_dyForge.InjectDll(dllPath)) {
            ShowStatus(L"DLL injected successfully");
            UpdateButtonStates();
        }
        else {
            ShowError(L"Failed to inject DLL");
        }
    }
}

void UIManager::OnStartAnalysis() {
    if (m_dyForge.StartAnalysis()) {
        ShowStatus(L"Analysis started");
        UpdateButtonStates();
    }
    else {
        ShowError(L"Failed to start analysis");
    }
}

void UIManager::OnStopAnalysis() {
    if (m_dyForge.StopAnalysis()) {
        ShowStatus(L"Analysis stopped");
        UpdateButtonStates();
    }
    else {
        ShowError(L"Failed to stop analysis");
    }
}

void UIManager::OnGenerateReport() {
    std::wstring outputPath = ShowSaveReportDialog();
    if (!outputPath.empty()) {
        if (m_dyForge.GenerateReport(outputPath)) {
            ShowStatus(L"Report generated successfully");
        }
        else {
            ShowError(L"Failed to generate report");
        }
    }
}

void UIManager::OnExportData() {
    std::wstring outputPath = ShowSaveExportDialog();
    if (!outputPath.empty()) {
        std::wstring format = L"json";
        if (outputPath.length() >= 4 && outputPath.substr(outputPath.length() - 4) == L".csv") {
            format = L"csv";
        }
        
        if (m_dyForge.ExportData(format, outputPath)) {
            ShowStatus(L"Data exported successfully");
        }
        else {
            ShowError(L"Failed to export data");
        }
    }
}

// Window procedure
LRESULT CALLBACK UIManager::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    UIManager* uiManager = NULL;
    
    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        uiManager = (UIManager*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)uiManager);
    }
    else {
        uiManager = (UIManager*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    
    if (uiManager) {
        return uiManager->HandleMessage(hwnd, uMsg, wParam, lParam);
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT UIManager::HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_COMMAND:
            return HandleCommand(LOWORD(wParam));
            
        case WM_NOTIFY:
            return HandleNotify((LPNMHDR)lParam);
            
        case WM_SIZE:
            return HandleSize(LOWORD(lParam), HIWORD(lParam));
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT UIManager::HandleCommand(int id) {
    switch (id) {
        case IDC_INJECT_BUTTON:
            OnInjectDll();
            return 0;
            
        case IDC_START_ANALYSIS_BUTTON:
            OnStartAnalysis();
            return 0;
            
        case IDC_STOP_ANALYSIS_BUTTON:
            OnStopAnalysis();
            return 0;
            
        case IDC_GENERATE_REPORT_BUTTON:
            OnGenerateReport();
            return 0;
            
        case IDC_EXPORT_DATA_BUTTON:
            OnExportData();
            return 0;
    }
    
    return 0;
}

LRESULT UIManager::HandleNotify(LPNMHDR pnmh) {
    switch (pnmh->code) {
        case LVN_ITEMCHANGED:
            OnProcessSelected();
            return 0;
    }
    
    return 0;
}

LRESULT UIManager::HandleSize(int width, int height) {
    // Resize status bar
    SendMessage(m_hwndStatusBar, WM_SIZE, 0, 0);
    
    // Resize process list
    SetWindowPos(
        m_hwndProcessList,
        NULL,
        10,
        10,
        width - 20,
        200,
        SWP_NOZORDER
    );
    
    // Reposition buttons
    SetWindowPos(
        m_hwndInjectButton,
        NULL,
        10,
        220,
        120,
        30,
        SWP_NOZORDER
    );
    
    SetWindowPos(
        m_hwndStartAnalysisButton,
        NULL,
        140,
        220,
        120,
        30,
        SWP_NOZORDER
    );
    
    SetWindowPos(
        m_hwndStopAnalysisButton,
        NULL,
        270,
        220,
        120,
        30,
        SWP_NOZORDER
    );
    
    SetWindowPos(
        m_hwndGenerateReportButton,
        NULL,
        400,
        220,
        120,
        30,
        SWP_NOZORDER
    );
    
    SetWindowPos(
        m_hwndExportDataButton,
        NULL,
        530,
        220,
        120,
        30,
        SWP_NOZORDER
    );
    
    return 0;
}

} // namespace DyForge 