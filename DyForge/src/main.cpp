#include "TerminalUI.h"
#include <iostream>
#include <Windows.h>

int main() {
    // Imposta la codifica della console
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // Crea e inizializza l'interfaccia
    DyForge::TerminalUI ui;
    if (!ui.Initialize()) {
        std::cerr << "Errore nell'inizializzazione dell'interfaccia" << std::endl;
        return 1;
    }

    // Mostra il menu principale
    ui.ShowMainMenu();

    return 0;
} 