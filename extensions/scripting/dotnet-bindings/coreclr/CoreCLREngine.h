/****************************************************************************
 Copyright (c) 2025 Axmol Engine contributors

 https://axmol.dev/

 CoreCLR implementation for desktop platforms (Windows/Linux/macOS)
 Uses official .NET hosting API (nethost + hostfxr)
 ****************************************************************************/

#pragma once

#include "../common/ScriptEngine.h"
#include <nethost.h>
#include <hostfxr.h>
#include <coreclr_delegates.h>
#include <unordered_map>

namespace ax {
namespace scripting {
namespace dotnet {

/**
 * @brief CoreCLR-based script engine for desktop platforms
 *
 * This implementation uses the official .NET hosting APIs:
 * - nethost.dll: Locates the .NET runtime
 * - hostfxr.dll: Initializes and manages the runtime
 */
class CoreCLREngine : public ScriptEngine
{
public:
    CoreCLREngine();
    virtual ~CoreCLREngine() override;

    // ScriptEngine interface implementation
    virtual bool initialize(const ScriptEngineConfig& config) override;
    virtual bool loadAssembly(const std::string& assemblyPath) override;
    virtual ScriptValue executeMethod(const std::string& typeName,
                                       const std::string& methodName,
                                       const ScriptArgs* args = nullptr) override;
    virtual ScriptObjectHandle createInstance(const std::string& typeName) override;
    virtual ScriptValue callMethod(ScriptObjectHandle handle,
                                    const std::string& methodName,
                                    const ScriptArgs* args = nullptr) override;
    virtual void shutdown() override;
    virtual RuntimeType getRuntimeType() const override { return RuntimeType::CoreCLR; }
    virtual bool isInitialized() const override { return _initialized; }

private:
    bool loadHostFxr();
    bool initializeRuntime(const std::string& runtimeConfigPath);
    bool loadFunctionPointers();

    // hostfxr function pointers
    hostfxr_initialize_for_runtime_config_fn _initFxr = nullptr;
    hostfxr_get_runtime_delegate_fn _getRuntimeDelegate = nullptr;
    hostfxr_close_fn _closeFxr = nullptr;

    // Runtime delegates
    load_assembly_and_get_function_pointer_fn _loadAssemblyAndGetFunctionPointer = nullptr;

    // Runtime state
    hostfxr_handle _hostContext = nullptr;
    void* _hostfxrLib = nullptr;
    bool _initialized = false;

    // Loaded assemblies
    std::unordered_map<std::string, void*> _loadedAssemblies;
};

} // namespace dotnet
} // namespace scripting
} // namespace ax
