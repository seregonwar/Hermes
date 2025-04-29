#include <iostream>
#include <string>
#include <windows.h>
#include "DyMain.h"
#include "DyHexInject.h"

int main(int argc, char* argv[]) {
    std::cout << "DyForge v1.0.0 - Custom Module Loader" << std::endl;
    std::cout << "--------------------------------" << std::endl;

    // Inizializza DyMain
    if (!Initialize()) {
        std::cerr << "Errore durante l'inizializzazione di DyMain" << std::endl;
        return 1;
    }

    std::cout << "DyMain inizializzato correttamente" << std::endl;

    // Menu principale
    while (true) {
        std::cout << "\nMenu:" << std::endl;
        std::cout << "1. Carica modulo" << std::endl;
        std::cout << "2. Scarica modulo" << std::endl;
        std::cout << "3. Lista moduli" << std::endl;
        std::cout << "4. Esci" << std::endl;
        std::cout << "\nScelta: ";

        int scelta;
        std::cin >> scelta;
        std::cin.ignore();

        switch (scelta) {
            case 1: {
                std::cout << "Inserisci il percorso del modulo: ";
                std::string path;
                std::getline(std::cin, path);
                
                if (WriteCommand(("LOAD_MOD " + path).c_str(), path.length() + 9)) {
                    std::cout << "Comando inviato correttamente" << std::endl;
                } else {
                    std::cerr << "Errore nell'invio del comando" << std::endl;
                }
                break;
            }
            case 2: {
                std::cout << "Inserisci il nome del modulo: ";
                std::string name;
                std::getline(std::cin, name);
                
                if (WriteCommand(("UNLOAD_MOD " + name).c_str(), name.length() + 11)) {
                    std::cout << "Comando inviato correttamente" << std::endl;
                } else {
                    std::cerr << "Errore nell'invio del comando" << std::endl;
                }
                break;
            }
            case 3: {
                if (WriteCommand("LIST_MODS", 9)) {
                    // Leggi lo stato
                    char buffer[4096];
                    size_t bytesRead;
                    if (ReadState(buffer, sizeof(buffer), &bytesRead)) {
                        std::cout << "Lista moduli:" << std::endl;
                        std::cout << std::string(buffer, bytesRead) << std::endl;
                    } else {
                        std::cerr << "Errore nella lettura dello stato" << std::endl;
                    }
                } else {
                    std::cerr << "Errore nell'invio del comando" << std::endl;
                }
                break;
            }
            case 4: {
                // Cleanup
                Cleanup();
                return 0;
            }
            default: {
                std::cout << "Scelta non valida" << std::endl;
                break;
            }
        }
    }

    return 0;
} 