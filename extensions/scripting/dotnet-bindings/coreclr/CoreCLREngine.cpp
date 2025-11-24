/****************************************************************************
 Copyright (c) 2025 Axmol Engine contributors

 https://axmol.dev/

 CoreCLR implementation for desktop platforms
 Uses official .NET hosting API (nethost + hostfxr)
 ****************************************************************************/

#include "CoreCLREngine.h"
#include "axmol.h"
#include <filesystem>
#include <cstring>

// Platform-specific library loading
#ifdef _WIN32
    #include <windows.h>
    #define STR(s) L##s
    #define CHAR_T wchar_t
    #define LOAD_LIBRARY(path) LoadLibraryW(path)
    #define GET_EXPORT(lib, name) GetProcAddress((HMODULE)lib, name)
    #define FREE_LIBRARY(lib) FreeLibrary((HMODULE)lib)
    #define PATH_MAX MAX_PATH
#else
    #include <dlfcn.h>
    #include <limits.h>
    #define STR(s) s
    #define CHAR_T char
    #define LOAD_LIBRARY(path) dlopen(path, RTLD_LAZY | RTLD_LOCAL)
    #define GET_EXPORT(lib, name) dlsym(lib, name)
    #define FREE_LIBRARY(lib) dlclose(lib)
    #ifndef PATH_MAX
        #define PATH_MAX 4096
    #endif
#endif

namespace ax {
namespace scripting {
namespace dotnet {

CoreCLREngine::CoreCLREngine()
{
    AXLOG("[CoreCLR] Engine instance created");
}

CoreCLREngine::~CoreCLREngine()
{
    shutdown();
}

bool CoreCLREngine::initialize(const ScriptEngineConfig& config)
{
    if (_initialized)
    {
        AXLOG("[CoreCLR] Already initialized");
        return true;
    }

    AXLOG("[CoreCLR] Initializing with config: %s", config.configPath.c_str());

    // Validate config
    if (config.configPath.empty())
    {
        AXLOGERROR("[CoreCLR] Config path is empty");
        return false;
    }

    // Step 1: Load hostfxr library
    if (!loadHostFxr())
    {
        AXLOGERROR("[CoreCLR] Failed to load hostfxr");
        return false;
    }

    // Step 2: Initialize runtime with config
    if (!initializeRuntime(config.configPath))
    {
        AXLOGERROR("[CoreCLR] Failed to initialize runtime");
        return false;
    }

    // Step 3: Load function pointers for assembly loading
    if (!loadFunctionPointers())
    {
        AXLOGERROR("[CoreCLR] Failed to load function pointers");
        return false;
    }

    _initialized = true;
    AXLOG("[CoreCLR] Initialization complete");
    return true;
}

bool CoreCLREngine::loadHostFxr()
{
    AXLOG("[CoreCLR] Loading hostfxr library");

    // Use nethost to find hostfxr
    CHAR_T buffer[PATH_MAX];
    size_t buffer_size = sizeof(buffer) / sizeof(CHAR_T);

    int rc = get_hostfxr_path(buffer, &buffer_size, nullptr);
    if (rc != 0)
    {
        AXLOGERROR("[CoreCLR] Failed to find hostfxr. Error code: %d", rc);
        AXLOGERROR("[CoreCLR] Make sure .NET SDK 8.0+ is installed");
        AXLOGERROR("[CoreCLR] Set DOTNET_ROOT environment variable if needed");
        return false;
    }

#ifdef _WIN32
    AXLOG("[CoreCLR] Found hostfxr at: %ls", buffer);
#else
    AXLOG("[CoreCLR] Found hostfxr at: %s", buffer);
#endif

    // Load hostfxr library
    _hostfxrLib = LOAD_LIBRARY(buffer);
    if (!_hostfxrLib)
    {
        AXLOGERROR("[CoreCLR] Failed to load hostfxr library");
        return false;
    }

    // Load function pointers
    _initFxr = (hostfxr_initialize_for_runtime_config_fn)
        GET_EXPORT(_hostfxrLib, "hostfxr_initialize_for_runtime_config");
    _getRuntimeDelegate = (hostfxr_get_runtime_delegate_fn)
        GET_EXPORT(_hostfxrLib, "hostfxr_get_runtime_delegate");
    _closeFxr = (hostfxr_close_fn)
        GET_EXPORT(_hostfxrLib, "hostfxr_close");

    if (!_initFxr || !_getRuntimeDelegate || !_closeFxr)
    {
        AXLOGERROR("[CoreCLR] Failed to load hostfxr function pointers");
        return false;
    }

    AXLOG("[CoreCLR] hostfxr loaded successfully");
    return true;
}

bool CoreCLREngine::initializeRuntime(const std::string& runtimeConfigPath)
{
    AXLOG("[CoreCLR] Initializing runtime with: %s", runtimeConfigPath.c_str());

    // Convert path to platform string
#ifdef _WIN32
    // Convert UTF-8 to wide string
    int wlen = MultiByteToWideChar(CP_UTF8, 0, runtimeConfigPath.c_str(), -1, nullptr, 0);
    std::wstring wpath(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, runtimeConfigPath.c_str(), -1, &wpath[0], wlen);
    const CHAR_T* config_path = wpath.c_str();
#else
    const CHAR_T* config_path = runtimeConfigPath.c_str();
#endif

    // Initialize runtime
    int rc = _initFxr(config_path, nullptr, &_hostContext);
    if (rc != 0 || !_hostContext)
    {
        AXLOGERROR("[CoreCLR] Failed to initialize runtime. Error code: 0x%08X", rc);
        AXLOGERROR("[CoreCLR] Make sure runtimeconfig.json exists and is valid");
        return false;
    }

    AXLOG("[CoreCLR] Runtime context initialized");
    return true;
}

bool CoreCLREngine::loadFunctionPointers()
{
    AXLOG("[CoreCLR] Loading assembly function pointer");

    int rc = _getRuntimeDelegate(
        _hostContext,
        hdt_load_assembly_and_get_function_pointer,
        (void**)&_loadAssemblyAndGetFunctionPointer
    );

    if (rc != 0 || !_loadAssemblyAndGetFunctionPointer)
    {
        AXLOGERROR("[CoreCLR] Failed to get function pointer delegate. Error: 0x%08X", rc);
        return false;
    }

    AXLOG("[CoreCLR] Function pointers loaded");
    return true;
}

bool CoreCLREngine::loadAssembly(const std::string& assemblyPath)
{
    if (!_initialized)
    {
        AXLOGERROR("[CoreCLR] Engine not initialized");
        return false;
    }

    AXLOG("[CoreCLR] Loading assembly: %s", assemblyPath.c_str());

    // Check if already loaded
    if (_loadedAssemblies.find(assemblyPath) != _loadedAssemblies.end())
    {
        AXLOG("[CoreCLR] Assembly already loaded: %s", assemblyPath.c_str());
        return true;
    }

    // Verify file exists
    if (!std::filesystem::exists(assemblyPath))
    {
        AXLOGERROR("[CoreCLR] Assembly file not found: %s", assemblyPath.c_str());
        return false;
    }

    // Mark as loaded (actual loading happens on first method call)
    _loadedAssemblies[assemblyPath] = nullptr;
    AXLOG("[CoreCLR] Assembly registered: %s", assemblyPath.c_str());

    return true;
}

ScriptValue CoreCLREngine::executeMethod(const std::string& typeName,
                                          const std::string& methodName,
                                          const ScriptArgs* args)
{
    if (!_initialized)
    {
        AXLOGERROR("[CoreCLR] Engine not initialized");
        return ScriptValue{};
    }

    AXLOG("[CoreCLR] Executing method: %s::%s", typeName.c_str(), methodName.c_str());

    // For now, we'll use the first loaded assembly
    // TODO: Support specifying which assembly to use
    if (_loadedAssemblies.empty())
    {
        AXLOGERROR("[CoreCLR] No assemblies loaded");
        return ScriptValue{};
    }

    std::string assemblyPath = _loadedAssemblies.begin()->first;

#ifdef _WIN32
    // Convert UTF-8 to wide string
    int wlen1 = MultiByteToWideChar(CP_UTF8, 0, assemblyPath.c_str(), -1, nullptr, 0);
    std::wstring wassembly(wlen1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, assemblyPath.c_str(), -1, &wassembly[0], wlen1);

    int wlen2 = MultiByteToWideChar(CP_UTF8, 0, typeName.c_str(), -1, nullptr, 0);
    std::wstring wtype(wlen2, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, typeName.c_str(), -1, &wtype[0], wlen2);

    int wlen3 = MultiByteToWideChar(CP_UTF8, 0, methodName.c_str(), -1, nullptr, 0);
    std::wstring wmethod(wlen3, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, methodName.c_str(), -1, &wmethod[0], wlen3);

    const CHAR_T* assembly_path = wassembly.c_str();
    const CHAR_T* type_name = wtype.c_str();
    const CHAR_T* method_name = wmethod.c_str();
#else
    const CHAR_T* assembly_path = assemblyPath.c_str();
    const CHAR_T* type_name = typeName.c_str();
    const CHAR_T* method_name = methodName.c_str();
#endif

    // Get function pointer
    typedef int (CORECLR_DELEGATE_CALLTYPE *custom_entry_point_fn)(void* arg, int arg_size);
    custom_entry_point_fn func = nullptr;

    // For UnmanagedCallersOnly methods, we need to pass delegate_type_name as
    // "UnmanagedCallersOnly" or use the UNMANAGEDCALLERSONLY_METHOD macro
    int rc = _loadAssemblyAndGetFunctionPointer(
        assembly_path,
        type_name,
        method_name,
        UNMANAGEDCALLERSONLY_METHOD,  // Special value for UnmanagedCallersOnly
        nullptr,  // reserved
        (void**)&func
    );

    if (rc != 0 || !func)
    {
        AXLOGERROR("[CoreCLR] Failed to get function pointer. Error: 0x%08X", rc);
        AXLOGERROR("[CoreCLR] Make sure the method has [UnmanagedCallersOnly] attribute");
        return ScriptValue{};
    }

    // Call the function
    try
    {
        int result = func(nullptr, 0);
        AXLOG("[CoreCLR] Method executed successfully. Return value: %d", result);
        return ScriptValue{result};
    }
    catch (...)
    {
        AXLOGERROR("[CoreCLR] Exception during method execution");
        return ScriptValue{};
    }
}

ScriptObjectHandle CoreCLREngine::createInstance(const std::string& typeName)
{
    AXLOGERROR("[CoreCLR] createInstance not yet implemented");
    // TODO: Implement instance creation
    return nullptr;
}

ScriptValue CoreCLREngine::callMethod(ScriptObjectHandle handle,
                                       const std::string& methodName,
                                       const ScriptArgs* args)
{
    AXLOGERROR("[CoreCLR] callMethod not yet implemented");
    // TODO: Implement instance method calls
    return ScriptValue{};
}

void CoreCLREngine::shutdown()
{
    if (!_initialized)
        return;

    AXLOG("[CoreCLR] Shutting down");

    // Close runtime context
    if (_hostContext && _closeFxr)
    {
        _closeFxr(_hostContext);
        _hostContext = nullptr;
    }

    // Unload hostfxr library
    if (_hostfxrLib)
    {
        FREE_LIBRARY(_hostfxrLib);
        _hostfxrLib = nullptr;
    }

    // Clear state
    _loadedAssemblies.clear();
    _initFxr = nullptr;
    _getRuntimeDelegate = nullptr;
    _closeFxr = nullptr;
    _loadAssemblyAndGetFunctionPointer = nullptr;

    _initialized = false;
    AXLOG("[CoreCLR] Shutdown complete");
}

} // namespace dotnet
} // namespace scripting
} // namespace ax
