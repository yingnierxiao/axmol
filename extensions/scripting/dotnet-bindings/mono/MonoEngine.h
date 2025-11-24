/****************************************************************************
 Copyright (c) 2025 Axmol Engine contributors

 https://axmol.dev/

 Mono implementation for mobile platforms (iOS/Android)
 Uses Mono embedding API
 ****************************************************************************/

#pragma once

#include "../common/ScriptEngine.h"
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/object.h>
#include <mono/metadata/mono-config.h>
#include <unordered_map>

namespace ax {
namespace scripting {
namespace dotnet {

/**
 * @brief Mono-based script engine for mobile platforms
 *
 * This implementation uses the Mono embedding API which supports:
 * - iOS (AOT compiled)
 * - Android (AOT + JIT)
 *
 * Reference: https://www.mono-project.com/docs/advanced/embedding/
 */
class MonoEngine : public ScriptEngine
{
public:
    MonoEngine();
    virtual ~MonoEngine() override;

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
    virtual RuntimeType getRuntimeType() const override { return RuntimeType::Mono; }
    virtual bool isInitialized() const override { return _initialized; }

private:
    MonoMethod* findMethod(MonoClass* klass, const std::string& methodName, int paramCount);
    MonoObject* invokeMethod(MonoMethod* method, MonoObject* instance, void** params);
    ScriptValue convertMonoObjectToScriptValue(MonoObject* obj);
    MonoObject* convertScriptValueToMonoObject(const ScriptValue& value);

    // Mono runtime state
    MonoDomain* _domain = nullptr;
    MonoAssembly* _mainAssembly = nullptr;
    MonoImage* _mainImage = nullptr;
    bool _initialized = false;

    // Loaded assemblies
    std::unordered_map<std::string, MonoAssembly*> _loadedAssemblies;
    std::unordered_map<std::string, MonoImage*> _loadedImages;
};

} // namespace dotnet
} // namespace scripting
} // namespace ax
