#pragma once

#include "include/DyHexInjectTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#ifdef DYHEXINJECT_EXPORTS
#define DYHEXINJECT_API __declspec(dllexport)
#else
#define DYHEXINJECT_API __declspec(dllimport)
#endif
#else
#define DYHEXINJECT_API
#endif

// Legacy API for backward compatibility
DYHEXINJECT_API int InjectDll(const char* processName, const char* dllPath);
DYHEXINJECT_API int InjectDllByPID(int processId, const char* dllPath);

#ifdef __cplusplus
}
#endif