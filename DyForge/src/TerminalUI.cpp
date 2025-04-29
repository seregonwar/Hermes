#include "TerminalUI.h"
#include "../communication/CommunicationManager.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <windows.h>
#include <tlhelp32.h>
#include <shellapi.h>

namespace DyForge {

TerminalUI::TerminalUI()
    : isInitialized(false)
    , selectedProcessId(0)
    , isDLLInjected(false)
    , isAnalysisRunning(false)
    , isWebServerRunning(false)
{
}

TerminalUI::~TerminalUI() {
    Cleanup();
}

bool TerminalUI::Initialize() {
    if (isInitialized) return true;

    // Inizializza la console
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE) {
        return false;
    }

    // Imposta il titolo della console
    SetConsoleTitle(L"DyForge - Terminal UI");

    // Imposta il buffer della console
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    csbi.dwSize.Y = 9999;
    SetConsoleScreenBufferSize(hConsole, csbi.dwSize);

    isInitialized = true;
    return true;
}

void TerminalUI::Cleanup() {
    if (!isInitialized) return;

    // Eject DLL se necessario
    if (isDLLInjected && selectedProcessId != 0) {
        EjectDLL(selectedProcessId);
    }

    // Stop web server se necessario
    if (isWebServerRunning) {
        StopWebServer();
    }

    // Stop analysis se necessario
    if (isAnalysisRunning) {
        StopAnalysis();
    }

    isInitialized = false;
}

void TerminalUI::ShowMainMenu() {
    while (true) {
        ClearScreen();
        PrintHeader();
        PrintStatus();

        std::cout << "\nMenu Principale:\n";
        std::cout << "1. Seleziona Processo\n";
        std::cout << "2. Comandi\n";
        std::cout << "3. Analisi\n";
        std::cout << "4. Mod\n";
        std::cout << "5. Web Server\n";
        std::cout << "0. Esci\n";
        std::cout << "\nScelta: ";

        std::string choice = GetInput("");
        
        if (choice == "0") break;
        else if (choice == "1") ShowProcessSelection();
        else if (choice == "2") ShowCommandMenu();
        else if (choice == "3") ShowAnalysisMenu();
        else if (choice == "4") ShowModMenu();
        else if (choice == "5") ShowWebServerMenu();
    }
}

void TerminalUI::ShowProcessSelection() {
    while (true) {
        ClearScreen();
        PrintHeader();
        PrintStatus();

        std::cout << "\nSelezione Processo:\n";
        PrintProcessList();

        std::cout << "\n1. Seleziona Processo\n";
        std::cout << "2. Inietta DLL\n";
        std::cout << "3. Eject DLL\n";
        std::cout << "0. Torna al Menu Principale\n";
        std::cout << "\nScelta: ";

        std::string choice = GetInput("");
        
        if (choice == "0") break;
        else if (choice == "1") {
            DWORD processId;
            if (SelectProcess(processId)) {
                selectedProcessId = processId;
                SetColor(COLOR_SUCCESS);
                std::cout << "Processo selezionato: " << processId << std::endl;
                ResetColor();
            }
        }
        else if (choice == "2") {
            if (selectedProcessId == 0) {
                SetColor(COLOR_ERROR);
                std::cout << "Nessun processo selezionato!" << std::endl;
                ResetColor();
            }
            else if (InjectDLL(selectedProcessId)) {
                isDLLInjected = true;
                SetColor(COLOR_SUCCESS);
                std::cout << "DLL iniettata con successo!" << std::endl;
                ResetColor();
            }
        }
        else if (choice == "3") {
            if (!isDLLInjected) {
                SetColor(COLOR_ERROR);
                std::cout << "DLL non iniettata!" << std::endl;
                ResetColor();
            }
            else if (EjectDLL(selectedProcessId)) {
                isDLLInjected = false;
                SetColor(COLOR_SUCCESS);
                std::cout << "DLL ejectata con successo!" << std::endl;
                ResetColor();
            }
        }

        WaitForKey();
    }
}

void TerminalUI::ShowCommandMenu() {
    while (true) {
        ClearScreen();
        PrintHeader();
        PrintStatus();

        std::cout << "\nMenu Comandi:\n";
        std::cout << "1. Avvia Analisi\n";
        std::cout << "2. Stop Analisi\n";
        std::cout << "3. Avvia Web Server\n";
        std::cout << "4. Stop Web Server\n";
        std::cout << "0. Torna al Menu Principale\n";
        std::cout << "\nScelta: ";

        std::string choice = GetInput("");
        
        if (choice == "0") break;
        else if (choice == "1") {
            if (!isDLLInjected) {
                SetColor(COLOR_ERROR);
                std::cout << "DLL non iniettata!" << std::endl;
                ResetColor();
            }
            else if (StartAnalysis()) {
                isAnalysisRunning = true;
                SetColor(COLOR_SUCCESS);
                std::cout << "Analisi avviata con successo!" << std::endl;
                ResetColor();
            }
        }
        else if (choice == "2") {
            if (!isAnalysisRunning) {
                SetColor(COLOR_ERROR);
                std::cout << "Analisi non in esecuzione!" << std::endl;
                ResetColor();
            }
            else if (StopAnalysis()) {
                isAnalysisRunning = false;
                SetColor(COLOR_SUCCESS);
                std::cout << "Analisi fermata con successo!" << std::endl;
                ResetColor();
            }
        }
        else if (choice == "3") {
            if (!isDLLInjected) {
                SetColor(COLOR_ERROR);
                std::cout << "DLL non iniettata!" << std::endl;
                ResetColor();
            }
            else if (StartWebServer()) {
                isWebServerRunning = true;
                SetColor(COLOR_SUCCESS);
                std::cout << "Web Server avviato con successo!" << std::endl;
                ResetColor();
            }
        }
        else if (choice == "4") {
            if (!isWebServerRunning) {
                SetColor(COLOR_ERROR);
                std::cout << "Web Server non in esecuzione!" << std::endl;
                ResetColor();
            }
            else if (StopWebServer()) {
                isWebServerRunning = false;
                SetColor(COLOR_SUCCESS);
                std::cout << "Web Server fermato con successo!" << std::endl;
                ResetColor();
            }
        }

        WaitForKey();
    }
}

void TerminalUI::ShowAnalysisMenu() {
    while (true) {
        ClearScreen();
        PrintHeader();
        PrintStatus();

        std::cout << "\nMenu Analisi:\n";
        std::cout << "1. Avvia Analisi\n";
        std::cout << "2. Stop Analisi\n";
        std::cout << "3. Mostra Risultati\n";
        std::cout << "0. Torna al Menu Principale\n";
        std::cout << "\nScelta: ";

        std::string choice = GetInput("");
        
        if (choice == "0") break;
        else if (choice == "1") {
            if (StartAnalysis()) {
                SetColor(COLOR_SUCCESS);
                std::cout << "Analisi avviata con successo!" << std::endl;
                ResetColor();
            }
        }
        else if (choice == "2") {
            if (StopAnalysis()) {
                SetColor(COLOR_SUCCESS);
                std::cout << "Analisi fermata con successo!" << std::endl;
                ResetColor();
            }
        }
        else if (choice == "3") {
            std::string results;
            if (m_commManager.GetAnalysisResults(results)) {
                std::cout << "\nRisultati Analisi:\n" << results << std::endl;
            }
        }

        WaitForKey();
    }
}

void TerminalUI::ShowModMenu() {
    while (true) {
        ClearScreen();
        PrintHeader();
        PrintStatus();

        std::cout << "\nMenu Mod:\n";
        std::cout << "1. Carica Mod\n";
        std::cout << "2. Scarica Mod\n";
        std::cout << "3. Lista Mod\n";
        std::cout << "0. Torna al Menu Principale\n";
        std::cout << "\nScelta: ";

        std::string choice = GetInput("");
        
        if (choice == "0") break;
        else if (choice == "1") {
            std::string modPath = GetInput("Inserisci il percorso della mod: ");
            if (LoadMod(modPath)) {
                SetColor(COLOR_SUCCESS);
                std::cout << "Mod caricata con successo!" << std::endl;
                ResetColor();
            }
        }
        else if (choice == "2") {
            std::string modName = GetInput("Inserisci il nome della mod: ");
            if (UnloadMod(modName)) {
                SetColor(COLOR_SUCCESS);
                std::cout << "Mod scaricata con successo!" << std::endl;
                ResetColor();
            }
        }
        else if (choice == "3") {
            std::string modList;
            if (m_commManager.GetModInfo(modList)) {
                std::cout << "\nMod Caricate:\n" << modList << std::endl;
            }
        }

        WaitForKey();
    }
}

void TerminalUI::ShowWebServerMenu() {
    while (true) {
        ClearScreen();
        PrintHeader();
        PrintStatus();

        std::cout << "\nMenu Web Server:\n";
        std::cout << "1. Avvia Web Server\n";
        std::cout << "2. Stop Web Server\n";
        std::cout << "3. Apri Dashboard\n";
        std::cout << "0. Torna al Menu Principale\n";
        std::cout << "\nScelta: ";

        std::string choice = GetInput("");
        
        if (choice == "0") break;
        else if (choice == "1") {
            if (!isDLLInjected) {
                SetColor(COLOR_ERROR);
                std::cout << "DLL non iniettata!" << std::endl;
                ResetColor();
            }
            else if (StartWebServer()) {
                isWebServerRunning = true;
                SetColor(COLOR_SUCCESS);
                std::cout << "Web Server avviato con successo!" << std::endl;
                ResetColor();
            }
        }
        else if (choice == "2") {
            if (!isWebServerRunning) {
                SetColor(COLOR_ERROR);
                std::cout << "Web Server non in esecuzione!" << std::endl;
                ResetColor();
            }
            else if (StopWebServer()) {
                isWebServerRunning = false;
                SetColor(COLOR_SUCCESS);
                std::cout << "Web Server fermato con successo!" << std::endl;
                ResetColor();
            }
        }
        else if (choice == "3") {
            if (!isWebServerRunning) {
                SetColor(COLOR_ERROR);
                std::cout << "Web Server non in esecuzione!" << std::endl;
                ResetColor();
            }
            else {
                ShellExecuteA(NULL, "open", "http://localhost:8080", NULL, NULL, SW_SHOWNORMAL);
            }
        }

        WaitForKey();
    }
}

bool TerminalUI::SelectProcess(DWORD& processId) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnapshot, &pe32)) {
        CloseHandle(hSnapshot);
        return false;
    }

    std::vector<std::pair<DWORD, std::wstring>> processes;
    do {
        processes.push_back({pe32.th32ProcessID, pe32.szExeFile});
    } while (Process32Next(hSnapshot, &pe32));

    CloseHandle(hSnapshot);

    // Mostra la lista dei processi
    std::cout << "\nProcessi disponibili:\n";
    for (size_t i = 0; i < processes.size(); i++) {
        std::wcout << std::setw(3) << i + 1 << ". " << processes[i].second << " (PID: " << processes[i].first << ")\n";
    }

    // Chiedi all'utente di selezionare un processo
    std::string input = GetInput("\nSeleziona un processo (numero): ");
    try {
        size_t index = std::stoul(input) - 1;
        if (index < processes.size()) {
            processId = processes[index].first;
            return true;
        }
    }
    catch (...) {
        // Ignora errori di conversione
    }

    return false;
}

bool TerminalUI::InjectDLL(DWORD processId) {
    if (!m_commManager.InjectDLL(processId)) {
        SetColor(COLOR_ERROR);
        std::cout << "Errore durante l'iniezione della DLL: " << m_commManager.GetLastError() << std::endl;
        ResetColor();
        return false;
    }
    isDLLInjected = true;
    return true;
}

bool TerminalUI::EjectDLL(DWORD processId) {
    if (!m_commManager.EjectDLL(processId)) {
        SetColor(COLOR_ERROR);
        std::cout << "Errore durante l'eiezione della DLL: " << m_commManager.GetLastError() << std::endl;
        ResetColor();
        return false;
    }
    isDLLInjected = false;
    return true;
}

bool TerminalUI::StartAnalysis() {
    if (!isDLLInjected) {
        SetColor(COLOR_ERROR);
        std::cout << "DLL non iniettata!" << std::endl;
        ResetColor();
        return false;
    }

    if (!m_commManager.StartAnalysis()) {
        SetColor(COLOR_ERROR);
        std::cout << "Errore durante l'avvio dell'analisi: " << m_commManager.GetLastError() << std::endl;
        ResetColor();
        return false;
    }
    isAnalysisRunning = true;
    return true;
}

bool TerminalUI::StopAnalysis() {
    if (!isAnalysisRunning) {
        SetColor(COLOR_ERROR);
        std::cout << "Analisi non in esecuzione!" << std::endl;
        ResetColor();
        return false;
    }

    if (!m_commManager.StopAnalysis()) {
        SetColor(COLOR_ERROR);
        std::cout << "Errore durante l'arresto dell'analisi: " << m_commManager.GetLastError() << std::endl;
        ResetColor();
        return false;
    }
    isAnalysisRunning = false;
    return true;
}

bool TerminalUI::StartWebServer() {
    if (!isDLLInjected) {
        SetColor(COLOR_ERROR);
        std::cout << "DLL non iniettata!" << std::endl;
        ResetColor();
        return false;
    }

    if (!m_commManager.StartWebServer()) {
        SetColor(COLOR_ERROR);
        std::cout << "Errore durante l'avvio del web server: " << m_commManager.GetLastError() << std::endl;
        ResetColor();
        return false;
    }
    return true;
}

bool TerminalUI::StopWebServer() {
    if (!m_commManager.StopWebServer()) {
        SetColor(COLOR_ERROR);
        std::cout << "Errore durante l'arresto del web server: " << m_commManager.GetLastError() << std::endl;
        ResetColor();
        return false;
    }
    return true;
}

bool TerminalUI::LoadMod(const std::string& modPath) {
    if (!isDLLInjected) {
        SetColor(COLOR_ERROR);
        std::cout << "DLL non iniettata!" << std::endl;
        ResetColor();
        return false;
    }

    if (!std::filesystem::exists(modPath)) {
        SetColor(COLOR_ERROR);
        std::cout << "File mod non trovato!" << std::endl;
        ResetColor();
        return false;
    }

    if (!m_commManager.LoadMod(modPath)) {
        SetColor(COLOR_ERROR);
        std::cout << "Errore durante il caricamento della mod: " << m_commManager.GetLastError() << std::endl;
        ResetColor();
        return false;
    }
    return true;
}

bool TerminalUI::UnloadMod(const std::string& modName) {
    if (!isDLLInjected) {
        SetColor(COLOR_ERROR);
        std::cout << "DLL non iniettata!" << std::endl;
        ResetColor();
        return false;
    }

    if (!m_commManager.UnloadMod(modName)) {
        SetColor(COLOR_ERROR);
        std::cout << "Errore durante lo scaricamento della mod: " << m_commManager.GetLastError() << std::endl;
        ResetColor();
        return false;
    }
    return true;
}

void TerminalUI::ClearScreen() {
    system("cls");
}

void TerminalUI::PrintHeader() {
    SetColor(COLOR_HEADER);
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                     DyForge Terminal UI                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    ResetColor();
}

void TerminalUI::PrintProcessList() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnapshot, &pe32)) {
        CloseHandle(hSnapshot);
        return;
    }

    std::cout << std::setw(5) << "PID" << " | " << std::setw(40) << "Nome Processo" << " | " << "Stato\n";
    std::cout << std::string(60, '-') << "\n";

    do {
        std::wcout << std::setw(5) << pe32.th32ProcessID << " | " 
                   << std::setw(40) << pe32.szExeFile << " | ";
        
        if (pe32.th32ProcessID == selectedProcessId) {
            SetColor(COLOR_SUCCESS);
            std::cout << "Selezionato";
            ResetColor();
        }
        else {
            std::cout << "-";
        }
        std::cout << "\n";
    } while (Process32Next(hSnapshot, &pe32));

    CloseHandle(hSnapshot);
}

void TerminalUI::PrintStatus() {
    std::cout << "\nStato:\n";
    std::cout << "Processo: " << (selectedProcessId ? std::to_string(selectedProcessId) : "Nessuno") << "\n";
    std::cout << "DLL: " << (isDLLInjected ? "Iniettata" : "Non iniettata") << "\n";
    std::cout << "Analisi: " << (isAnalysisRunning ? "In esecuzione" : "Fermata") << "\n";
    std::cout << "Web Server: " << (isWebServerRunning ? "In esecuzione" : "Fermato") << "\n";
}

void TerminalUI::WaitForKey() {
    std::cout << "\nPremi un tasto per continuare...";
    _getch();
}

std::string TerminalUI::GetInput(const std::string& prompt) {
    std::string input;
    std::cout << prompt;
    std::getline(std::cin, input);
    return input;
}

bool TerminalUI::ConfirmAction(const std::string& message) {
    std::cout << message << " (s/n): ";
    std::string input = GetInput("");
    return input == "s" || input == "S";
}

void TerminalUI::SetColor(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void TerminalUI::ResetColor() {
    SetColor(COLOR_DEFAULT);
}

} // namespace DyForge 