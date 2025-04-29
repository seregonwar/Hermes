#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>

// Export macro
#ifdef DyMain_EXPORTS
#define DyMain_API __declspec(dllexport)
#else
#define DyMain_API __declspec(dllimport)
#endif

// Version info
#define DyMain_VERSION_MAJOR 1
#define DyMain_VERSION_MINOR 0

// Memory layout
struct SharedMemoryLayout {
	static const size_t HEADER_SIZE = 128;
	static const size_t COMMAND_BUFFER_SIZE = 4096;
	static const size_t STATE_BUFFER_SIZE = 8192;
	static const size_t TOTAL_SIZE = HEADER_SIZE + COMMAND_BUFFER_SIZE + STATE_BUFFER_SIZE;
};

// Function declarations
extern "C" {
	bool Initialize();
	void Cleanup();
	bool WriteCommand(const char* command, size_t length);
	bool ReadState(char* buffer, size_t bufferSize, size_t* bytesRead);
}

// Internal functions
namespace DyMain {
	// Core System
	bool StartDyMain(HMODULE hModule);
	void StopDyMain();
	
	// Injection Manager
	namespace Injection {
		bool InitializeInjection(HMODULE hModule);
		void ProtectSelf();
		void GetTargetProcessInfo();
	}
	
	// Memory Manager
	namespace Memory {
		bool ReadMemory(uintptr_t address, void* buffer, size_t size);
		bool WriteMemory(uintptr_t address, const void* buffer, size_t size);
		void* AllocateMemory(size_t size);
		void FreeMemory(void* address);
		bool AnalyzeMemoryRegions(const std::string& pattern, std::vector<uintptr_t>& matches);
	}
	
	// Hook Manager
	namespace Hook {
		bool CreateInlineHook(void* target, void* detour, void** original);
		bool RemoveHook(void* target);
		void ListActiveHooks();
	}
	
	// Mod Manager
	namespace Mod {
		bool LoadMod(const std::string& modPath);
		bool UnloadMod(const std::string& modName);
		void ListMods();
		bool GetDependencies(const std::string& modPath, std::vector<std::string>& dependencies);
	}
	
	// Deep Analyzer
	namespace Analyzer {
		void ListThreads();
		void DumpModules();
		void AnalyzeMemoryRegions();
		void DetectAntiCheatMechanisms();
	}
	
	// Web Server Integration
	namespace WebServer {
		bool StartWebServer(int port = 8080);
		void StopWebServer();
		void BroadcastEvent(const std::string& event);
	}
	
	// Utility Helpers
	namespace Utils {
		bool PatternToByteArray(const std::string& pattern, std::vector<int>& bytes);
		bool CompareMemoryPattern(const uint8_t* data, const std::vector<int>& pattern);
		bool ScanMemoryForPatterns(const std::string& pattern, std::vector<uintptr_t>& matches);
		void HexDump(const void* ptr, size_t size);
		
		namespace Logger {
			void LogInfo(const std::string& msg);
			void LogWarning(const std::string& msg);
			void LogError(const std::string& msg);
		}
	}

	// Strutture di base
	struct Command {
		enum class Type {
			START_ANALYSIS,
			STOP_ANALYSIS,
			LOAD_MOD,
			UNLOAD_MOD,
			INSTALL_HOOK,
			REMOVE_HOOK,
			SCAN_MEMORY,
			WRITE_MEMORY
		};
		
		Type type;
		std::vector<uint8_t> data;
	};

	struct State {
		bool isAnalysisRunning;
		uint32_t activeHooks;
		uint32_t loadedMods;
	};

	struct MemoryConfig {
		uintptr_t startAddress;
		size_t size;
		std::string pattern;
	};

	struct MemoryWrite {
		uintptr_t address;
		std::vector<uint8_t> data;
	};

	struct MemoryInfo {
		std::vector<uintptr_t> matches;
		size_t totalScanned;
	};

	struct HookConfig {
		void* target;
		void* detour;
		std::string name;
	};

	struct ModConfig {
		std::string path;
		std::string name;
		std::vector<std::string> dependencies;
	};

	struct AnalysisConfig {
		bool scanMemory;
		bool scanThreads;
		bool scanModules;
		bool detectAntiCheat;
	};

	struct ProcessInfo {
		DWORD processId;
		std::wstring name;
		std::wstring path;
		bool is64Bit;
	};

	struct ModuleInfo {
		HMODULE handle;
		std::wstring name;
		std::wstring path;
		uintptr_t baseAddress;
		size_t size;
	};

	struct ThreadInfo {
		DWORD threadId;
		DWORD basePriority;
		void* startAddress;
		bool isSuspended;
	};

	struct WebServerConfig {
		uint16_t port;
		bool enableWebSocket;
		bool enableSSL;
		std::string certPath;
		std::string keyPath;
	};

	// Funzioni principali
	bool StartDyMain(HMODULE hModule);
	void StopDyMain();

} // namespace DyMain

// Funzioni esportate
extern "C" {
	DyMain_API bool WriteCommandEx(const DyMain::Command* cmd);
	DyMain_API bool ReadStateEx(DyMain::State* state);
	DyMain_API bool InjectDLL(const wchar_t* dllPath);
	DyMain_API bool EjectDLL(DWORD processId);
	DyMain_API bool ScanMemory(const DyMain::MemoryConfig* config, DyMain::MemoryInfo* info);
	DyMain_API bool WriteMemory(const DyMain::MemoryWrite* write);
	DyMain_API bool InstallHook(const DyMain::HookConfig* config);
	DyMain_API bool RemoveHook(const DyMain::HookConfig* config);
	DyMain_API bool LoadMod(const DyMain::ModConfig* config);
	DyMain_API bool UnloadMod(const DyMain::ModConfig* config);
	DyMain_API bool StartAnalysis(const DyMain::AnalysisConfig* config);
	DyMain_API bool StopAnalysis();
	DyMain_API bool StartWebServer(const DyMain::WebServerConfig* config);
	DyMain_API bool StopWebServer();
	DyMain_API bool GetProcessInfo(DyMain::ProcessInfo* info);
	DyMain_API bool GetModuleInfo(DyMain::ModuleInfo* info);
	DyMain_API bool GetThreadInfo(DyMain::ThreadInfo* info);
}