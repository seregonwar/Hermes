#include "DyMain.h"
#include "WebServer/WebServer.h"
#include "SharedMemoryLayout.h"
#include <windows.h>
#include <iostream>
#include <string>
#include <memory>
#include <mutex>
#include <aclapi.h>
#include <sddl.h>
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <tlhelp32.h>
#include <psapi.h>
#include <detours.h>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <algorithm>
#include <mongoose.h>

// Definizioni Mongoose
#define MG_EV_HTTP_MSG 1
#define MG_WS_OP_TEXT 1

// Funzioni Mongoose
static inline struct mg_str mg_http_get_header_value(const struct mg_http_message* hm, const char* name) {
	struct mg_str result = {NULL, 0};
	// Implementazione robusta per ottenere l'header
	for (int i = 0; i < MG_MAX_HTTP_HEADERS; i++) {
		if (hm->headers[i].name.len == 0) break;  // Fine degli headers
		if (mg_strcmp(hm->headers[i].name, mg_str(name)) == 0) {
			result = hm->headers[i].value;
			break;
		}
	}
	return result;
}

static inline int mg_http_get_var_value(const struct mg_http_message* hm, const char* name, char* buf, size_t len) {
	struct mg_str v = mg_http_get_header_value(hm, name);
	if (v.len > 0) {
		if (v.len >= len) return -1;
		memcpy(buf, v.buf, v.len);
		buf[v.len] = '\0';
		return (int)v.len;
	}
	return -1;
}

static inline int mg_websocket_send(struct mg_connection* c, const void* data, size_t len, int op) {
	// Implementazione robusta per inviare un messaggio WebSocket
	size_t i, j;
	size_t header_len = 2;
	size_t frame_len = len;
	uint8_t header[10];  // WebSocket frame header

	header[0] = (uint8_t)(op | 0x80);  // FIN + opcode
	
	if (len < 126) {
		header[1] = (uint8_t)len;
	} else if (len < 65536) {
		header[1] = 126;
		header[2] = (uint8_t)(len >> 8);
		header[3] = (uint8_t)(len & 0xff);
		header_len = 4;
	} else {
		header[1] = 127;
		for (i = 0; i < 8; i++) {
			header[2 + i] = (uint8_t)((len >> ((7 - i) * 8)) & 0xff);
		}
		header_len = 10;
	}

	// Invia l'header
	for (i = 0; i < header_len; i++) {
		if (mg_io_send(c, &header[i], 1) != 1) return -1;
	}

	// Invia i dati
	const uint8_t* p = (const uint8_t*)data;
	for (j = 0; j < frame_len; j++) {
		if (mg_io_send(c, &p[j], 1) != 1) return -1;
	}

	return (int)(header_len + frame_len);
}

// PEB structures
typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _PEB_LDR_DATA {
	ULONG Length;
	BYTE Initialized;
	PVOID SsHandle;
	LIST_ENTRY InLoadOrderModuleList;
	LIST_ENTRY InMemoryOrderModuleList;
	LIST_ENTRY InInitializationOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _LDR_DATA_TABLE_ENTRY {
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InMemoryOrderLinks;
	LIST_ENTRY InInitializationOrderLinks;
	PVOID DllBase;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

typedef struct _PEB {
	BYTE Reserved1[2];
	BYTE BeingDebugged;
	BYTE Reserved2[1];
	PVOID Reserved3[2];
	PPEB_LDR_DATA Ldr;
	// ... other fields omitted for brevity
} PEB, *PPEB;

// Global variables
namespace {
	HANDLE g_SharedMemory = NULL;
	void* g_MappedMemory = NULL;
	std::mutex g_Mutex;
	bool g_Initialized = false;
	HANDLE g_CommandThread = NULL;
}

// Global state
static HMODULE g_hModule = NULL;
static bool g_initialized = false;
static HANDLE g_sharedMemory = NULL;
static void* g_sharedMemoryPtr = NULL;
static std::ofstream g_logFile;

// Implementation of exported functions
extern "C" {
	bool Initialize() {
		try {
			std::lock_guard<std::mutex> lock(g_Mutex);
			
			if (g_Initialized) {
				std::cout << "Already initialized" << std::endl;
				return true;
			}

			DWORD error = 0;
			std::cout << "Initializing shared memory..." << std::endl;
			if (!DyMain::SharedMemory::CreateSharedMemory()) {
				error = GetLastError();
				std::cout << "Failed to create shared memory. Error: " << error << std::endl;
				return false;
			}

			std::cout << "Starting command thread..." << std::endl;
			g_CommandThread = CreateThread(NULL, 0, 
				(LPTHREAD_START_ROUTINE)DyMain::SharedMemory::ProcessCommands, 
				NULL, 0, NULL);

			if (!g_CommandThread) {
				error = GetLastError();
				std::cout << "Failed to create command thread. Error: " << error << std::endl;
				DyMain::SharedMemory::CloseSharedMemory();
				return false;
			}

			std::cout << "Injecting hooks..." << std::endl;
			if (!DyMain::SharedMemory::InjectHooks()) {
				std::cout << "Failed to inject hooks" << std::endl;
				DyMain::SharedMemory::CloseSharedMemory();
				return false;
			}

			g_Initialized = true;
			std::cout << "Initialization completed successfully" << std::endl;
			return true;
		}
		catch (const std::exception& e) {
			std::cout << "Exception during initialization: " << e.what() << std::endl;
			return false;
		}
	}

	void Cleanup() {
		std::lock_guard<std::mutex> lock(g_Mutex);
		
		if (!g_Initialized) {
			return;
		}

		DyMain::SharedMemory::RemoveHooks();
		
		if (g_CommandThread) {
			TerminateThread(g_CommandThread, 0);
			CloseHandle(g_CommandThread);
			g_CommandThread = NULL;
		}

		DyMain::SharedMemory::CloseSharedMemory();
		g_Initialized = false;
	}

	bool WriteCommand(const char* command, size_t length) {
		if (!g_Initialized || !g_MappedMemory || !command || length == 0) {
			return false;
		}

		std::lock_guard<std::mutex> lock(g_Mutex);
		
		if (length > DyMain::SharedMemory::COMMAND_BUFFER_SIZE) {
			return false;
		}

		char* cmdBuffer = static_cast<char*>(g_MappedMemory) + DyMain::SharedMemory::HEADER_SIZE;
		memcpy(cmdBuffer, command, length);
		cmdBuffer[length] = '\0';
		return true;
	}

	bool ReadState(char* buffer, size_t bufferSize, size_t* bytesRead) {
		if (!g_Initialized || !g_MappedMemory || !buffer || !bytesRead) {
			return false;
		}

		std::lock_guard<std::mutex> lock(g_Mutex);
		
		char* stateBuffer = static_cast<char*>(g_MappedMemory) + 
			DyMain::SharedMemory::HEADER_SIZE + 
			DyMain::SharedMemory::COMMAND_BUFFER_SIZE;

		size_t stateLength = strlen(stateBuffer);
		if (stateLength == 0) {
			*bytesRead = 0;
			return true;
		}

		if (bufferSize < stateLength + 1) {
			return false;
		}

		memcpy(buffer, stateBuffer, stateLength);
		buffer[stateLength] = '\0';
		*bytesRead = stateLength;
		return true;
	}
}

// Implementation of internal functions
namespace DyMain::SharedMemory {
	bool CreateSharedMemory() {
		// Create security attributes with explicit permissions for all users
		SECURITY_ATTRIBUTES sa;
		SECURITY_DESCRIPTOR sd;
		PSECURITY_DESCRIPTOR pSD = NULL;
		PACL pACL = NULL;
		EXPLICIT_ACCESS ea;
		PSID pEveryoneSID = NULL;
		SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
		
		// Create a well-known SID for the Everyone group
		if(!AllocateAndInitializeSid(&SIDAuthWorld, 1,
									SECURITY_WORLD_RID,
									0, 0, 0, 0, 0, 0, 0,
									&pEveryoneSID)) {
			return false;
		}
		
		// Initialize an EXPLICIT_ACCESS structure for an ACE
		ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
		ea.grfAccessPermissions = GENERIC_ALL;
		ea.grfAccessMode = SET_ACCESS;
		ea.grfInheritance = NO_INHERITANCE;
		ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
		ea.Trustee.ptstrName = (LPTSTR)pEveryoneSID;
		
		// Create a new ACL that contains the new ACEs
		DWORD dwRes = SetEntriesInAcl(1, &ea, NULL, &pACL);
		if (ERROR_SUCCESS != dwRes) {
			FreeSid(pEveryoneSID);
			return false;
		}
		
		// Initialize a security descriptor
		pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
		if (NULL == pSD) {
			FreeSid(pEveryoneSID);
			LocalFree(pACL);
			return false;
		}
		
		if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION)) {
			FreeSid(pEveryoneSID);
			LocalFree(pACL);
			LocalFree(pSD);
			return false;
		}
		
		// Add the ACL to the security descriptor
		if (!SetSecurityDescriptorDacl(pSD, TRUE, pACL, FALSE)) {
			FreeSid(pEveryoneSID);
			LocalFree(pACL);
			LocalFree(pSD);
			return false;
		}
		
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = pSD;
		sa.bInheritHandle = FALSE;
		
		// Create the shared memory with the security attributes
		g_SharedMemory = CreateFileMappingA(
			INVALID_HANDLE_VALUE,
			&sa,
			PAGE_READWRITE,
			0,
			SharedMemoryLayout::TOTAL_SIZE,
			"GTAVCoopSharedMem"
		);
		
		// Cleanup security descriptor resources
		FreeSid(pEveryoneSID);
		LocalFree(pACL);
		LocalFree(pSD);
		
		if (!g_SharedMemory) {
			DWORD error = GetLastError();
			std::cout << "Failed to create shared memory mapping. Error: " << error << std::endl;
			return false;
		}
		
		g_MappedMemory = MapViewOfFile(
			g_SharedMemory,
			FILE_MAP_ALL_ACCESS,
			0,
			0,
			SharedMemoryLayout::TOTAL_SIZE
		);
		
		if (!g_MappedMemory) {
			DWORD error = GetLastError();
			std::cout << "Failed to map view of file. Error: " << error << std::endl;
			CloseHandle(g_SharedMemory);
			g_SharedMemory = NULL;
			return false;
		}
		
		// Initialize memory
		memset(g_MappedMemory, 0, SharedMemoryLayout::TOTAL_SIZE);
		return true;
	}

	void CloseSharedMemory() {
		if (g_MappedMemory) {
			UnmapViewOfFile(g_MappedMemory);
			g_MappedMemory = NULL;
		}

		if (g_SharedMemory) {
			CloseHandle(g_SharedMemory);
			g_SharedMemory = NULL;
		}
	}

	DWORD WINAPI ProcessCommands(LPVOID lpParam) {
		while (true) {
			if (!g_MappedMemory) {
				Sleep(100);
				continue;
			}

			char* cmdBuffer = static_cast<char*>(g_MappedMemory) + SharedMemoryLayout::HEADER_SIZE;
			if (cmdBuffer[0] != '\0') {
				std::string command(cmdBuffer);
				// Processa il comando qui
				cmdBuffer[0] = '\0'; // Reset del buffer
			}
			Sleep(10);
		}
		return 0;
	}

	bool InjectHooks() {
		// Implementazione base degli hook
		return true;
	}

	void RemoveHooks() {
		// Implementazione base della rimozione degli hook
	}
}

// Core System
bool DyMain::StartDyMain(HMODULE hModule) {
	if (g_initialized) {
		return true;
	}
	
	try {
		g_hModule = hModule;
		
		// Initialize logging
		g_logFile.open("DyMain.log", std::ios::app);
		if (!g_logFile.is_open()) {
			throw std::runtime_error("Failed to open log file");
		}
		
		// Initialize shared memory
		g_sharedMemory = CreateFileMappingA(
			INVALID_HANDLE_VALUE,
			NULL,
			PAGE_READWRITE,
			0,
			SharedMemoryLayout::TOTAL_SIZE,
			"DyMainSharedMemory"
		);
		
		if (!g_sharedMemory) {
			throw std::runtime_error("Failed to create shared memory");
		}
		
		g_sharedMemoryPtr = MapViewOfFile(
			g_sharedMemory,
			FILE_MAP_ALL_ACCESS,
			0,
			0,
			SharedMemoryLayout::TOTAL_SIZE
		);
		
		if (!g_sharedMemoryPtr) {
			CloseHandle(g_sharedMemory);
			throw std::runtime_error("Failed to map shared memory");
		}
		
		// Initialize injection
		if (!Injection::InitializeInjection(hModule)) {
			throw std::runtime_error("Failed to initialize injection");
		}
		
		// Start web server
		if (!WebServer::StartWebServer(8080)) {
			throw std::runtime_error("Failed to start web server");
		}
		
		g_initialized = true;
		Utils::Logger::LogInfo("DyMain started successfully");
		return true;
	}
	catch (const std::exception& e) {
		Utils::Logger::LogError(std::string("Failed to start DyMain: ") + e.what());
		return false;
	}
}

void DyMain::StopDyMain() {
	if (!g_initialized) {
		return;
	}
	
	// Stop web server
	WebServer::StopWebServer();
	
	// Cleanup shared memory
	if (g_sharedMemoryPtr) {
		UnmapViewOfFile(g_sharedMemoryPtr);
		g_sharedMemoryPtr = NULL;
	}
	
	if (g_sharedMemory) {
		CloseHandle(g_sharedMemory);
		g_sharedMemory = NULL;
	}
	
	// Close log file
	if (g_logFile.is_open()) {
		g_logFile.close();
	}
	
	g_initialized = false;
	Utils::Logger::LogInfo("DyMain stopped");
}

// Injection Manager
namespace DyMain::Injection {
	bool InitializeInjection(HMODULE hModule) {
		try {
			// Get process information
			GetTargetProcessInfo();
			
			// Protect DLL from detection
			ProtectSelf();
			
			Utils::Logger::LogInfo("Injection initialized successfully");
			return true;
		}
		catch (const std::exception& e) {
			Utils::Logger::LogError(std::string("Failed to initialize injection: ") + e.what());
			return false;
		}
	}
	
	void ProtectSelf() {
		try {
			// Get module handle
			HMODULE hModule = GetModuleHandle(NULL);
			if (hModule == NULL) {
				throw std::runtime_error("Failed to get module handle");
			}
			
			// Get PEB
			PEB* pPeb = (PEB*)__readgsqword(0x60);
			if (pPeb == NULL) {
				throw std::runtime_error("Failed to get PEB");
			}
			
			// Get LDR
			PEB_LDR_DATA* pLdr = pPeb->Ldr;
			if (pLdr == NULL) {
				throw std::runtime_error("Failed to get LDR");
			}
			
			// Find our module in the LDR
			LIST_ENTRY* pEntry = &pLdr->InLoadOrderModuleList;
			LIST_ENTRY* pCurrent = pEntry->Flink;
			
			while (pCurrent != pEntry) {
				LDR_DATA_TABLE_ENTRY* pLdrEntry = CONTAINING_RECORD(pCurrent, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
				
				if (pLdrEntry->DllBase == hModule) {
					// Remove from InLoadOrderLinks
					pCurrent->Blink->Flink = pCurrent->Flink;
					pCurrent->Flink->Blink = pCurrent->Blink;
					
					// Remove from InMemoryOrderLinks
					pLdrEntry->InMemoryOrderLinks.Blink->Flink = pLdrEntry->InMemoryOrderLinks.Flink;
					pLdrEntry->InMemoryOrderLinks.Flink->Blink = pLdrEntry->InMemoryOrderLinks.Blink;
					
					// Remove from InInitializationOrderLinks
					pLdrEntry->InInitializationOrderLinks.Blink->Flink = pLdrEntry->InInitializationOrderLinks.Flink;
					pLdrEntry->InInitializationOrderLinks.Flink->Blink = pLdrEntry->InInitializationOrderLinks.Blink;
					
					break;
				}
				
				pCurrent = pCurrent->Flink;
			}
			
			// Obfuscate memory regions
			MEMORY_BASIC_INFORMATION mbi;
			uintptr_t addr = (uintptr_t)hModule;
			
			while (VirtualQuery((LPCVOID)addr, &mbi, sizeof(mbi))) {
				if (mbi.State == MEM_COMMIT && mbi.Type == MEM_PRIVATE) {
					// XOR memory region
					uint8_t* data = (uint8_t*)mbi.BaseAddress;
					for (size_t i = 0; i < mbi.RegionSize; i++) {
						data[i] ^= 0xAA;
					}
				}
				
				addr += mbi.RegionSize;
			}
			
			// Anti-debugging measures
			if (IsDebuggerPresent()) {
				throw std::runtime_error("Debugger detected");
			}
			
			// Check for common debugger artifacts
			if (CheckRemoteDebuggerPresent(GetCurrentProcess(), NULL)) {
				throw std::runtime_error("Remote debugger detected");
			}
			
			Utils::Logger::LogInfo("DLL protection completed successfully");
		}
		catch (const std::exception& e) {
			Utils::Logger::LogError(std::string("Failed to protect DLL: ") + e.what());
		}
	}
	
	void GetTargetProcessInfo() {
		try {
			// Get process ID
			DWORD processId = GetCurrentProcessId();
			
			// Get process handle
			HANDLE hProcess = GetCurrentProcess();
			
			// Get process name
			WCHAR processName[MAX_PATH];
			if (GetModuleFileNameW(NULL, processName, MAX_PATH) == 0) {
				throw std::runtime_error("Failed to get process name");
			}
			
			// Get process path
			WCHAR processPath[MAX_PATH];
			if (GetModuleFileNameExW(hProcess, NULL, processPath, MAX_PATH) == 0) {
				throw std::runtime_error("Failed to get process path");
			}
			
			// Get architecture
			SYSTEM_INFO sysInfo;
			GetNativeSystemInfo(&sysInfo);
			
			// Get process memory info
			PROCESS_MEMORY_COUNTERS_EX pmc;
			if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
				// Log memory info
				std::stringstream ss;
				ss << "Process Memory Info:" << std::endl;
				ss << "  Working Set Size: " << pmc.WorkingSetSize << " bytes" << std::endl;
				ss << "  Private Usage: " << pmc.PrivateUsage << " bytes" << std::endl;
				Utils::Logger::LogInfo(ss.str());
			}
			
			// Log process info
			std::wstringstream ss;
			ss << L"Process ID: " << processId << std::endl;
			ss << L"Process Name: " << processName << std::endl;
			ss << L"Process Path: " << processPath << std::endl;
			ss << L"Architecture: " << (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? L"x64" : L"x86") << std::endl;
			
			Utils::Logger::LogInfo(std::string(ss.str().begin(), ss.str().end()));
		}
		catch (const std::exception& e) {
			Utils::Logger::LogError(std::string("Failed to get process info: ") + e.what());
		}
	}
}

// Memory Manager
namespace DyMain::Memory {
	bool ReadMemory(uintptr_t addr, void* buffer, size_t size) {
		try {
			return ReadProcessMemory(GetCurrentProcess(), (LPCVOID)addr, buffer, size, NULL) != 0;
		}
		catch (const std::exception& e) {
			Utils::Logger::LogError(std::string("Failed to read memory: ") + e.what());
			return false;
		}
	}
	
	bool WriteMemory(uintptr_t addr, const void* data, size_t size) {
		try {
			DWORD oldProtect;
			if (!VirtualProtect((LPVOID)addr, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
				return false;
			}
			
			bool result = WriteProcessMemory(GetCurrentProcess(), (LPVOID)addr, data, size, NULL) != 0;
			
			VirtualProtect((LPVOID)addr, size, oldProtect, &oldProtect);
			return result;
		}
		catch (const std::exception& e) {
			Utils::Logger::LogError(std::string("Failed to write memory: ") + e.what());
			return false;
		}
	}
	
	void* AllocateMemory(size_t size) {
		try {
			return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		}
		catch (const std::exception& e) {
			Utils::Logger::LogError(std::string("Failed to allocate memory: ") + e.what());
			return NULL;
		}
	}
	
	void FreeMemory(void* ptr) {
		try {
			VirtualFree(ptr, 0, MEM_RELEASE);
		}
		catch (const std::exception& e) {
			Utils::Logger::LogError(std::string("Failed to free memory: ") + e.what());
		}
	}

	void AnalyzeMemoryRegions() {
		try {
			MEMORY_BASIC_INFORMATION mbi;
			uintptr_t addr = 0;
			
			while (VirtualQuery((LPCVOID)addr, &mbi, sizeof(mbi))) {
				if (mbi.State == MEM_COMMIT) {
					std::stringstream ss;
					ss << "Memory Region at " << std::hex << (uintptr_t)mbi.BaseAddress << std::endl;
					ss << "  Size: " << mbi.RegionSize << " bytes" << std::endl;
					ss << "  State: " << (mbi.State == MEM_COMMIT ? "Committed" : "Reserved") << std::endl;
					ss << "  Type: " << (mbi.Type == MEM_IMAGE ? "Image" : 
									   mbi.Type == MEM_MAPPED ? "Mapped" : "Private") << std::endl;
					ss << "  Protection: " << std::hex << mbi.Protect << std::endl;
					
					// Analyze region content
					if (mbi.State == MEM_COMMIT && mbi.Type == MEM_PRIVATE) {
						std::vector<uint8_t> buffer(mbi.RegionSize);
						if (ReadProcessMemory(GetCurrentProcess(), mbi.BaseAddress, buffer.data(), mbi.RegionSize, NULL)) {
							// Implement pattern scanning
							std::vector<std::string> patterns = {
								"48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20", // Common function prologue
								"48 8B C4 48 89 58 ?? 48 89 70 ?? 48 89 78 ?? 55", // Another common pattern
								"48 83 EC 28 48 8B 05 ?? ?? ?? ?? 48 85 C0 74 ?? 48 8B 40" // Another pattern
							};

							for (const auto& pattern : patterns) {
								std::vector<int> bytes;
								if (DyMain::Utils::PatternToByteArray(pattern, bytes)) {
									std::vector<uintptr_t> matches;
									if (DyMain::Utils::ScanMemoryForPatterns(pattern, matches)) {
										ss << "  Found pattern matches at:" << std::endl;
										for (const auto& match : matches) {
											ss << "    " << std::hex << match << std::endl;
										}
									}
								}
							}
						}
					}
					
					Utils::Logger::LogInfo(ss.str());
				}
				
				addr += mbi.RegionSize;
			}
		}
		catch (const std::exception& e) {
			Utils::Logger::LogError(std::string("Failed to analyze memory regions: ") + e.what());
		}
	}
}

// Hook Manager
namespace DyMain::Hook {
	struct HookInfo {
		void* target;
		void* detour;
		void* original;
		std::string name;
	};
	
	static std::vector<HookInfo> g_hooks;
	
	bool CreateInlineHook(void* target, void* detour, void** original) {
		try {
			if (!target || !detour) {
				throw std::runtime_error("Invalid hook parameters");
			}
			
			// Create hook
			if (DetourTransactionBegin() != NO_ERROR) {
				throw std::runtime_error("Failed to begin hook transaction");
			}
			
			if (DetourUpdateThread(GetCurrentThread()) != NO_ERROR) {
				throw std::runtime_error("Failed to update thread");
			}
			
			if (DetourAttach(&(PVOID&)target, detour) != NO_ERROR) {
				throw std::runtime_error("Failed to attach hook");
			}
			
			if (DetourTransactionCommit() != NO_ERROR) {
				throw std::runtime_error("Failed to commit hook transaction");
			}
			
			// Store hook info
			HookInfo hook;
			hook.target = target;
			hook.detour = detour;
			hook.original = *original;
			hook.name = "Unknown";
			g_hooks.push_back(hook);
			
			Utils::Logger::LogInfo("Hook created successfully");
			return true;
		}
		catch (const std::exception& e) {
			Utils::Logger::LogError(std::string("Failed to create hook: ") + e.what());
			return false;
		}
	}
	
	bool RemoveHook(void* target) {
		try {
			if (!target) {
				throw std::runtime_error("Invalid hook target");
			}
			
			// Find hook
			auto it = std::find_if(g_hooks.begin(), g_hooks.end(), 
				[target](const HookInfo& hook) { return hook.target == target; });
			
			if (it == g_hooks.end()) {
				throw std::runtime_error("Hook not found");
			}
			
			// Remove hook
			if (DetourTransactionBegin() != NO_ERROR) {
				throw std::runtime_error("Failed to begin hook transaction");
			}
			
			if (DetourUpdateThread(GetCurrentThread()) != NO_ERROR) {
				throw std::runtime_error("Failed to update thread");
			}
			
			if (DetourDetach(&(PVOID&)target, it->detour) != NO_ERROR) {
				throw std::runtime_error("Failed to detach hook");
			}
			
			if (DetourTransactionCommit() != NO_ERROR) {
				throw std::runtime_error("Failed to commit hook transaction");
			}
			
			// Remove from list
			g_hooks.erase(it);
			
			Utils::Logger::LogInfo("Hook removed successfully");
			return true;
		}
		catch (const std::exception& e) {
			Utils::Logger::LogError(std::string("Failed to remove hook: ") + e.what());
			return false;
		}
	}
	
	void ListActiveHooks() {
		try {
			std::stringstream ss;
			ss << "Active Hooks:" << std::endl;
			
			for (const auto& hook : g_hooks) {
				ss << "  Target: " << hook.target << std::endl;
				ss << "  Detour: " << hook.detour << std::endl;
				ss << "  Original: " << hook.original << std::endl;
				ss << "  Name: " << hook.name << std::endl;
				ss << "  ---" << std::endl;
			}
			
			Utils::Logger::LogInfo(ss.str());
		}
		catch (const std::exception& e) {
			Utils::Logger::LogError(std::string("Failed to list hooks: ") + e.what());
		}
	}
}

// Mod Manager
namespace DyMain::Mod {
	struct ModInfo {
		std::string name;
		std::string path;
		HMODULE handle;
		bool loaded;
		std::vector<std::string> dependencies;
	};
	
	static std::vector<ModInfo> g_mods;
	
	bool LoadMod(const std::string& modPath) {
		try {
			// Check if mod is already loaded
			auto it = std::find_if(g_mods.begin(), g_mods.end(), 
				[&modPath](const ModInfo& mod) { return mod.path == modPath; });
			
			if (it != g_mods.end()) {
				if (it->loaded) {
					Utils::Logger::LogWarning("Mod already loaded: " + modPath);
					return true;
				}
			}
			
			// Load DLL
			HMODULE hModule = LoadLibraryA(modPath.c_str());
			if (!hModule) {
				throw std::runtime_error("Failed to load mod DLL");
			}
			
			// Get mod info
			ModInfo mod;
			mod.name = modPath.substr(modPath.find_last_of("/\\") + 1);
			mod.path = modPath;
			mod.handle = hModule;
			mod.loaded = true;
			
			// Get dependencies
			std::vector<std::string> dependencies;
			if (GetDependencies(modPath, dependencies)) {
				mod.dependencies = dependencies;
			}
			
			// Add to list
			g_mods.push_back(mod);
			
			Utils::Logger::LogInfo("Mod loaded successfully: " + modPath);
			return true;
		}
		catch (const std::exception& e) {
			Utils::Logger::LogError(std::string("Failed to load mod: ") + e.what());
			return false;
		}
	}

	bool GetDependencies(const std::string& modPath, std::vector<std::string>& dependencies) {
		try {
			// Carica la DLL per ottenere le dipendenze
			HMODULE hModule = LoadLibraryA(modPath.c_str());
			if (!hModule) {
				return false;
			}

			// Cerca la funzione GetDependencies
			typedef bool (*GetDependenciesFunc)(std::vector<std::string>&);
			GetDependenciesFunc getDeps = (GetDependenciesFunc)GetProcAddress(hModule, "GetDependencies");
			
			if (getDeps) {
				bool result = getDeps(dependencies);
				FreeLibrary(hModule);
				return result;
			}

			FreeLibrary(hModule);
			return false;
		}
		catch (const std::exception& e) {
			Utils::Logger::LogError(std::string("Failed to get dependencies: ") + e.what());
			return false;
		}
	}
	
	bool UnloadMod(const std::string& modName) {
		try {
			// Find mod
			auto it = std::find_if(g_mods.begin(), g_mods.end(), 
				[&modName](const ModInfo& mod) { return mod.name == modName; });
			
			if (it == g_mods.end()) {
				throw std::runtime_error("Mod not found: " + modName);
			}
			
			if (!it->loaded) {
				throw std::runtime_error("Mod not loaded: " + modName);
			}
			
			// Unload DLL
			if (!FreeLibrary(it->handle)) {
				throw std::runtime_error("Failed to unload mod DLL");
			}
			
			// Remove from list
			g_mods.erase(it);
			
			Utils::Logger::LogInfo("Mod unloaded successfully: " + modName);
			return true;
		}
		catch (const std::exception& e) {
			Utils::Logger::LogError(std::string("Failed to unload mod: ") + e.what());
			return false;
		}
	}
	
	void ListMods() {
		try {
			std::stringstream ss;
			ss << "Loaded Mods:" << std::endl;
			
			for (const auto& mod : g_mods) {
				ss << "  Name: " << mod.name << std::endl;
				ss << "  Path: " << mod.path << std::endl;
				ss << "  Handle: " << mod.handle << std::endl;
				ss << "  Loaded: " << (mod.loaded ? "Yes" : "No") << std::endl;
				ss << "  Dependencies: " << std::endl;
				for (const auto& dep : mod.dependencies) {
					ss << "    - " << dep << std::endl;
				}
				ss << "  ---" << std::endl;
			}
			
			Utils::Logger::LogInfo(ss.str());
		}
		catch (const std::exception& e) {
			Utils::Logger::LogError(std::string("Failed to list mods: ") + e.what());
		}
	}
}

// Deep Analyzer
namespace DyMain::Analyzer {
	void ListThreads() {
		try {
			HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
			if (hSnapshot == INVALID_HANDLE_VALUE) {
				throw std::runtime_error("Failed to create thread snapshot");
			}
			
			THREADENTRY32 te32;
			te32.dwSize = sizeof(te32);
			
			if (Thread32First(hSnapshot, &te32)) {
				do {
					if (te32.th32OwnerProcessID == GetCurrentProcessId()) {
						// Get thread context
						HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, te32.th32ThreadID);
						if (hThread) {
							CONTEXT ctx;
							ctx.ContextFlags = CONTEXT_ALL;
							
							if (GetThreadContext(hThread, &ctx)) {
								std::stringstream ss;
								ss << "Thread ID: " << te32.th32ThreadID << std::endl;
								ss << "  Base Priority: " << te32.tpBasePri << std::endl;
								ss << "  Priority: " << te32.tpDeltaPri << std::endl;
								ss << "  Context:" << std::endl;
								ss << "    RIP: " << std::hex << ctx.Rip << std::endl;
								ss << "    RSP: " << std::hex << ctx.Rsp << std::endl;
								ss << "    RBP: " << std::hex << ctx.Rbp << std::endl;
								Utils::Logger::LogInfo(ss.str());
							}
							
							CloseHandle(hThread);
						}
					}
				} while (Thread32Next(hSnapshot, &te32));
			}
			
			CloseHandle(hSnapshot);
		}
		catch (const std::exception& e) {
			Utils::Logger::LogError(std::string("Failed to list threads: ") + e.what());
		}
	}
	
	void DumpModules() {
		try {
			HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
			if (hSnapshot == INVALID_HANDLE_VALUE) {
				throw std::runtime_error("Failed to create module snapshot");
			}
			
			MODULEENTRY32 me32;
			me32.dwSize = sizeof(me32);
			
			if (Module32First(hSnapshot, &me32)) {
				do {
					// Get module info
					std::stringstream ss;
					ss << "Module: " << me32.szModule << std::endl;
					ss << "  Path: " << me32.szExePath << std::endl;
					ss << "  Base Address: " << std::hex << (uintptr_t)me32.modBaseAddr << std::endl;
					ss << "  Size: " << me32.modBaseSize << " bytes" << std::endl;
					ss << "  Entry Point: " << std::hex << (uintptr_t)me32.modBaseAddr + me32.modBaseSize << std::endl;
					
					// Get module memory info
					MEMORY_BASIC_INFORMATION mbi;
					if (VirtualQuery(me32.modBaseAddr, &mbi, sizeof(mbi))) {
						ss << "  Memory Info:" << std::endl;
						ss << "    State: " << (mbi.State == MEM_COMMIT ? "Committed" : "Reserved") << std::endl;
						ss << "    Type: " << (mbi.Type == MEM_IMAGE ? "Image" : "Private") << std::endl;
						ss << "    Protection: " << std::hex << mbi.Protect << std::endl;
					}
					
					Utils::Logger::LogInfo(ss.str());
				} while (Module32Next(hSnapshot, &me32));
			}
			
			CloseHandle(hSnapshot);
		}
		catch (const std::exception& e) {
			Utils::Logger::LogError(std::string("Failed to dump modules: ") + e.what());
		}
	}
	
	void DetectAntiCheatMechanisms() {
		try {
			// Check for common anti-cheat processes
			HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (hSnapshot == INVALID_HANDLE_VALUE) {
				throw std::runtime_error("Failed to create process snapshot");
			}
			
			PROCESSENTRY32W pe32;
			pe32.dwSize = sizeof(pe32);
			
			if (Process32FirstW(hSnapshot, &pe32)) {
				do {
					// Check for known anti-cheat processes
					std::wstring processName(pe32.szExeFile);
					std::transform(processName.begin(), processName.end(), processName.begin(), ::tolower);
					
					if (processName.find(L"battleye") != std::wstring::npos ||
						processName.find(L"easyanticheat") != std::wstring::npos ||
						processName.find(L"vac") != std::wstring::npos) {
						std::stringstream ss;
						ss << "Anti-cheat process detected: " << std::string(processName.begin(), processName.end());
						Utils::Logger::LogWarning(ss.str());
					}
				} while (Process32NextW(hSnapshot, &pe32));
			}
			
			CloseHandle(hSnapshot);
			
			// Implement driver scanning
			std::vector<std::wstring> driverNames = {
				L"BEClient_x64.sys",
				L"EasyAntiCheat.sys",
				L"vac.sys"
			};

			for (const auto& driverName : driverNames) {
				HANDLE hDriver = CreateFileW(
					driverName.c_str(),
					GENERIC_READ,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL
				);

				if (hDriver != INVALID_HANDLE_VALUE) {
					std::stringstream ss;
					ss << "Anti-cheat driver detected: " << std::string(driverName.begin(), driverName.end());
					Utils::Logger::LogWarning(ss.str());
					CloseHandle(hDriver);
				}
			}
			
			// Implement registry scanning
			std::vector<std::wstring> registryPaths = {
				L"SOFTWARE\\BattlEye",
				L"SOFTWARE\\EasyAntiCheat",
				L"SOFTWARE\\Valve\\VAC"
			};

			for (const auto& path : registryPaths) {
				HKEY hKey;
				if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, path.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
					std::stringstream ss;
					ss << "Anti-cheat registry key detected: " << std::string(path.begin(), path.end());
					Utils::Logger::LogWarning(ss.str());
					RegCloseKey(hKey);
				}
			}
		}
		catch (const std::exception& e) {
			Utils::Logger::LogError(std::string("Failed to detect anti-cheat mechanisms: ") + e.what());
		}
	}

	void AnalyzeMemoryRegions() {
		try {
			MEMORY_BASIC_INFORMATION mbi;
			uintptr_t addr = 0;
			
			while (VirtualQuery((LPCVOID)addr, &mbi, sizeof(mbi))) {
				if (mbi.State == MEM_COMMIT) {
					std::stringstream ss;
					ss << "Memory Region at " << std::hex << (uintptr_t)mbi.BaseAddress << std::endl;
					ss << "  Size: " << mbi.RegionSize << " bytes" << std::endl;
					ss << "  State: " << (mbi.State == MEM_COMMIT ? "Committed" : "Reserved") << std::endl;
					ss << "  Type: " << (mbi.Type == MEM_IMAGE ? "Image" : 
									   mbi.Type == MEM_MAPPED ? "Mapped" : "Private") << std::endl;
					ss << "  Protection: " << std::hex << mbi.Protect << std::endl;
					
					// Analyze region content
					if (mbi.State == MEM_COMMIT && mbi.Type == MEM_PRIVATE) {
						std::vector<uint8_t> buffer(mbi.RegionSize);
						if (ReadProcessMemory(GetCurrentProcess(), mbi.BaseAddress, buffer.data(), mbi.RegionSize, NULL)) {
							// Implement pattern scanning
							std::vector<std::string> patterns = {
								"48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20", // Common function prologue
								"48 8B C4 48 89 58 ?? 48 89 70 ?? 48 89 78 ?? 55", // Another common pattern
								"48 83 EC 28 48 8B 05 ?? ?? ?? ?? 48 85 C0 74 ?? 48 8B 40" // Another pattern
							};

							for (const auto& pattern : patterns) {
								std::vector<int> bytes;
								if (DyMain::Utils::PatternToByteArray(pattern, bytes)) {
									std::vector<uintptr_t> matches;
									if (DyMain::Utils::ScanMemoryForPatterns(pattern, matches)) {
										ss << "  Found pattern matches at:" << std::endl;
										for (const auto& match : matches) {
											ss << "    " << std::hex << match << std::endl;
										}
									}
								}
							}
						}
					}
					
					Utils::Logger::LogInfo(ss.str());
				}
				
				addr += mbi.RegionSize;
			}
		}
		catch (const std::exception& e) {
			Utils::Logger::LogError(std::string("Failed to analyze memory regions: ") + e.what());
		}
	}
}

// Web Server Integration
namespace DyMain::WebServer {
	struct WebServerConfig {
		int port;
		std::string rootPath;
		bool enableWebSocket;
		bool enableSSL;
		std::string sslCert;
		std::string sslKey;
	};
	
	static WebServerConfig g_config;
	static bool g_running = false;
	static struct mg_mgr g_mgr;
	static struct mg_connection* g_ws_connection = NULL;
	
	// HTTP request handler
	static void HandleRequest(struct mg_connection* c, int ev, void* ev_data) {
		if (ev == MG_EV_HTTP_MSG) {
			struct mg_http_message* hm = static_cast<struct mg_http_message*>(ev_data);
			
			// Handle different endpoints
			std::string uri = std::string(hm->uri.buf, hm->uri.len);
			
			if (uri == "/memory") {
				// Return memory information
				std::stringstream ss;
				ss << "{\"status\":\"ok\",\"data\":{";
				
				// Get memory info
				PROCESS_MEMORY_COUNTERS_EX pmc;
				if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
					ss << "\"workingSetSize\":" << pmc.WorkingSetSize << ",";
					ss << "\"privateUsage\":" << pmc.PrivateUsage;
				}
				
				ss << "}}";
				
				mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", ss.str().c_str());
			}
			else if (uri == "/hooks") {
				// Return active hooks
				std::stringstream ss;
				ss << "{\"status\":\"ok\",\"data\":[";
				
				bool first = true;
				for (const auto& hook : Hook::g_hooks) {
					if (!first) ss << ",";
					ss << "{\"target\":\"" << hook.target << "\",";
					ss << "\"detour\":\"" << hook.detour << "\",";
					ss << "\"original\":\"" << hook.original << "\",";
					ss << "\"name\":\"" << hook.name << "\"}";
					first = false;
				}
				
				ss << "]}";
				
				mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", ss.str().c_str());
			}
			else if (uri == "/mods") {
				// Return loaded mods
				std::stringstream ss;
				ss << "{\"status\":\"ok\",\"data\":[";
				
				bool first = true;
				for (const auto& mod : Mod::g_mods) {
					if (!first) ss << ",";
					ss << "{\"name\":\"" << mod.name << "\",";
					ss << "\"path\":\"" << mod.path << "\",";
					ss << "\"handle\":\"" << mod.handle << "\",";
					ss << "\"loaded\":" << (mod.loaded ? "true" : "false") << ",";
					ss << "\"dependencies\":[";
					
					bool firstDep = true;
					for (const auto& dep : mod.dependencies) {
						if (!firstDep) ss << ",";
						ss << "\"" << dep << "\"";
						firstDep = false;
					}
					
					ss << "]}";
					first = false;
				}
				
				ss << "]}";
				
				mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", ss.str().c_str());
			}
			else if (uri == "/process") {
				// Return process information
				std::stringstream ss;
				ss << "{\"status\":\"ok\",\"data\":{";
				
				// Get process info
				DWORD processId = GetCurrentProcessId();
				HANDLE hProcess = GetCurrentProcess();
				
				// Get process name
				WCHAR processName[MAX_PATH];
				if (GetModuleFileNameW(NULL, processName, MAX_PATH)) {
					ss << "\"name\":\"" << std::string(processName, processName + wcslen(processName)) << "\",";
				}
				
				// Get process path
				WCHAR processPath[MAX_PATH];
				if (GetModuleFileNameExW(hProcess, NULL, processPath, MAX_PATH)) {
					ss << "\"path\":\"" << std::string(processPath, processPath + wcslen(processPath)) << "\",";
				}
				
				// Get architecture
				SYSTEM_INFO sysInfo;
				GetNativeSystemInfo(&sysInfo);
				ss << "\"architecture\":\"" << (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? "x64" : "x86") << "\",";
				
				// Get memory info
				PROCESS_MEMORY_COUNTERS_EX pmc;
				if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
					ss << "\"workingSetSize\":" << pmc.WorkingSetSize << ",";
					ss << "\"privateUsage\":" << pmc.PrivateUsage;
				}
				
				ss << "}}";
				
				mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", ss.str().c_str());
			}
			else if (uri == "/command") {
				// Handle command execution
				struct mg_http_message* hm = (struct mg_http_message*)ev_data;
				char cmd[256];
				if (mg_http_get_var_value(hm, "command", cmd, sizeof(cmd)) < 0) {
					mg_http_reply(c, 400, "Content-Type: application/json\r\n", 
								"{\"status\":\"error\",\"message\":\"Missing command parameter\"}");
					return;
				}
				
				// Process command
				std::string command(cmd);
				std::istringstream iss(command);
				std::string cmdType;
				iss >> cmdType;
				
				std::string response;
				if (cmdType == "START_ANALYSIS") {
					Analyzer::ListThreads();
					Analyzer::DumpModules();
					Analyzer::AnalyzeMemoryRegions();
					Analyzer::DetectAntiCheatMechanisms();
					response = "{\"status\":\"ok\",\"message\":\"Analysis started\"}";
				}
				else if (cmdType == "STOP_ANALYSIS") {
					// TODO: Implement analysis stop
					response = "{\"status\":\"ok\",\"message\":\"Analysis stopped\"}";
				}
				else if (cmdType == "LOAD_MOD") {
					std::string modPath;
					iss >> modPath;
					if (Mod::LoadMod(modPath)) {
						response = "{\"status\":\"ok\",\"message\":\"Mod loaded\"}";
					}
					else {
						response = "{\"status\":\"error\",\"message\":\"Failed to load mod\"}";
					}
				}
				else if (cmdType == "UNLOAD_MOD") {
					std::string modName;
					iss >> modName;
					if (Mod::UnloadMod(modName)) {
						response = "{\"status\":\"ok\",\"message\":\"Mod unloaded\"}";
					}
					else {
						response = "{\"status\":\"error\",\"message\":\"Failed to unload mod\"}";
					}
				}
				else {
					response = "{\"status\":\"error\",\"message\":\"Unknown command\"}";
				}
				
				mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", response.c_str());
			}
			else {
				// Serve static files
				struct mg_http_serve_opts opts = {0};
				opts.root_dir = g_config.rootPath.c_str();
				mg_http_serve_dir(c, static_cast<struct mg_http_message*>(ev_data), &opts);
			}
		}
	}
	
	bool StartWebServer(int port) {
		if (g_running) {
			return true;
		}
		
		mg_mgr_init(&g_mgr);
		
		std::string addr = "http://localhost:" + std::to_string(port);
		struct mg_connection* c = mg_http_listen(&g_mgr, addr.c_str(), HandleRequest, nullptr);
		
		if (c == nullptr) {
			mg_mgr_free(&g_mgr);
			return false;
		}
		
		g_running = true;
		return true;
	}
	
	void StopWebServer() {
		try {
			if (!g_running) {
				return;
			}
			
			g_running = false;
			
			// Cleanup Mongoose
			mg_mgr_free(&g_mgr);
			
			Utils::Logger::LogInfo("Web server stopped");
		}
		catch (const std::exception& e) {
			Utils::Logger::LogError(std::string("Failed to stop web server: ") + e.what());
		}
	}
	
	void BroadcastEvent(const std::string& event) {
		try {
			if (!g_running || !g_ws_connection) {
				throw std::runtime_error("WebSocket not available");
			}
			
			// Send WebSocket message
			if (mg_websocket_send(g_ws_connection, event.c_str(), event.length(), MG_WS_OP_TEXT) < 0) {
				throw std::runtime_error("Failed to send WebSocket message");
			}
			
			Utils::Logger::LogInfo("Event broadcasted: " + event);
		}
		catch (const std::exception& e) {
			Utils::Logger::LogError(std::string("Failed to broadcast event: ") + e.what());
		}
	}
}

// Utility Helpers
namespace DyMain::Utils {
	bool PatternToByteArray(const std::string& pattern, std::vector<int>& bytes) {
		try {
			std::istringstream iss(pattern);
			std::string byte;
			
			while (std::getline(iss, byte, ' ')) {
				if (byte == "?") {
					bytes.push_back(-1);
				}
				else {
					bytes.push_back(std::stoi(byte, nullptr, 16));
				}
			}
			
			return true;
		}
		catch (const std::exception& e) {
			Logger::LogError(std::string("Failed to convert pattern to byte array: ") + e.what());
			return false;
		}
	}
	
	bool CompareMemoryPattern(const uint8_t* data, const std::vector<int>& pattern) {
		for (size_t i = 0; i < pattern.size(); i++) {
			if (pattern[i] != -1 && data[i] != pattern[i]) {
				return false;
			}
		}
		return true;
	}
	
	bool ScanMemoryForPatterns(const std::string& pattern, std::vector<uintptr_t>& matches) {
		try {
			std::vector<int> bytes;
			if (!PatternToByteArray(pattern, bytes)) {
				return false;
			}
			
			MEMORY_BASIC_INFORMATION mbi;
			uintptr_t addr = 0;
			
			while (VirtualQuery((LPCVOID)addr, &mbi, sizeof(mbi))) {
				if (mbi.State == MEM_COMMIT && 
					(mbi.Protect & PAGE_READWRITE) && 
					!(mbi.Protect & PAGE_GUARD)) {
					
					std::vector<uint8_t> buffer(mbi.RegionSize);
					if (ReadProcessMemory(GetCurrentProcess(), mbi.BaseAddress, buffer.data(), mbi.RegionSize, NULL)) {
						for (size_t i = 0; i < buffer.size() - bytes.size(); i++) {
							if (CompareMemoryPattern(&buffer[i], bytes)) {
								matches.push_back((uintptr_t)mbi.BaseAddress + i);
							}
						}
					}
				}
				
				addr += mbi.RegionSize;
			}
			
			return true;
		}
		catch (const std::exception& e) {
			Logger::LogError(std::string("Failed to scan memory: ") + e.what());
			return false;
		}
	}
	
	void HexDump(const void* ptr, size_t size) {
		const uint8_t* data = (const uint8_t*)ptr;
		std::stringstream ss;
		
		for (size_t i = 0; i < size; i += 16) {
			ss << std::hex << std::setw(8) << std::setfill('0') << i << ": ";
			
			for (size_t j = 0; j < 16; j++) {
				if (i + j < size) {
					ss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i + j] << " ";
				}
				else {
					ss << "   ";
				}
			}
			
			ss << " ";
			
			for (size_t j = 0; j < 16; j++) {
				if (i + j < size) {
					char c = data[i + j];
					ss << (isprint(c) ? c : '.');
				}
			}
			
			ss << "\n";
		}
		
		Logger::LogInfo(ss.str());
	}
	
	namespace Logger {
		void LogInfo(const std::string& msg) {
			if (g_logFile.is_open()) {
				auto now = std::chrono::system_clock::now();
				auto time = std::chrono::system_clock::to_time_t(now);
				g_logFile << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") 
						 << " [INFO] " << msg << std::endl;
			}
		}
		
		void LogWarning(const std::string& msg) {
			if (g_logFile.is_open()) {
				auto now = std::chrono::system_clock::now();
				auto time = std::chrono::system_clock::to_time_t(now);
				g_logFile << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") 
						 << " [WARNING] " << msg << std::endl;
			}
		}
		
		void LogError(const std::string& msg) {
			if (g_logFile.is_open()) {
				auto now = std::chrono::system_clock::now();
				auto time = std::chrono::system_clock::to_time_t(now);
				g_logFile << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") 
						 << " [ERROR] " << msg << std::endl;
			}
		}
	}
}

// DLL entry point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
	switch (reason) {
		case DLL_PROCESS_ATTACH:
			return DyMain::StartDyMain(hModule);
			
		case DLL_PROCESS_DETACH:
			DyMain::StopDyMain();
			break;
	}
	
	return TRUE;
}
