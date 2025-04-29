# DyForge: Dynamic Process Sculptor

DyForge è un'applicazione Windows che gestisce l'iniezione e l'analisi di processi tramite `DyMain.dll`. Fornisce un'interfaccia grafica per:

- Iniettare `DyMain.dll` in processi target
- Avviare/fermare l'analisi del processo
- Generare report dettagliati
- Esportare dati in vari formati

## Caratteristiche

- Interfaccia grafica moderna e intuitiva
- Gestione sicura dell'iniezione DLL
- Analisi in tempo reale del processo
- Generazione di report dettagliati
- Esportazione dati in JSON e CSV
- Logging completo delle operazioni

## Requisiti

- Windows 7 o superiore
- Visual Studio 2019 o superiore
- Windows SDK 10.0 o superiore
- DyHexInject (incluso)
- DyMain.dll (da compilare separatamente)

## Compilazione

1. Compilare `DyHexInject`:
   ```
   cd ../DyHexInject
   build.bat
   ```

2. Compilare `DyForge`:
   ```
   cd ../DyForge
   build.bat
   ```

## Utilizzo

1. Avviare `DyForge.exe`
2. Selezionare il processo target dalla lista
3. Cliccare su "Inject DLL" per iniettare `DyMain.dll`
4. Utilizzare i pulsanti per:
   - Avviare l'analisi
   - Fermare l'analisi
   - Generare report
   - Esportare dati

## Struttura del Progetto

```
DyForge/
├── src/
│   ├── core/           # Core functionality
│   ├── injection/      # DLL injection
│   ├── analysis/       # Process analysis
│   ├── reporting/      # Report generation
│   └── ui/            # User interface
├── include/           # Header files
├── tests/            # Unit tests
└── bin/              # Output directory
```

## Integrazione con DyMain.dll

DyForge comunica con `DyMain.dll` tramite un canale di comunicazione sicuro. I comandi supportati sono:

- `START_ANALYSIS` - Avvia l'analisi
- `STOP_ANALYSIS` - Ferma l'analisi
- `GET_ANALYSIS_DATA` - Ottiene i dati dell'analisi

## Note di Sicurezza

- L'iniezione DLL richiede privilegi elevati
- Utilizzare sempre l'opzione "Hide Injection" in produzione
- Non esporre l'API direttamente all'utente finale
- Gestire sempre gli errori e pulire le risorse

## Licenza

Questo progetto è rilasciato sotto la licenza MIT.