/****************************************************************************
 Copyright (c) 2025 Axmol Engine contributors

 https://axmol.dev/

 Mono implementation for mobile platforms (iOS/Android)
 Uses Mono embedding API
 Reference: https://www.mono-project.com/docs/advanced/embedding/
 ****************************************************************************/

#include "MonoEngine.h"
#include "axmol.h"
#include <filesystem>
#include <cstring>

namespace ax {
namespace scripting {
namespace dotnet {

MonoEngine::MonoEngine()
{
    AXLOG("[Mono] Engine instance created");
}

MonoEngine::~MonoEngine()
{
    shutdown();
}

bool MonoEngine::initialize(const ScriptEngineConfig& config)
{
    if (_initialized)
    {
        AXLOG("[Mono] Already initialized");
        return true;
    }

    AXLOG("[Mono] Initializing Mono runtime");

    // Validate config
    if (config.configPath.empty())
    {
        AXLOGERROR("[Mono] Config path (assembly path) is empty");
        return false;
    }

    // Set Mono assembly search paths
    if (!config.assemblySearchPaths.empty())
    {
        for (const auto& path : config.assemblySearchPaths)
        {
            AXLOG("[Mono] Adding assembly search path: %s", path.c_str());
            mono_set_assemblies_path(path.c_str());
        }
    }

    // Initialize Mono configuration
    mono_config_parse(nullptr);

#ifdef IOS_MONO_AOT_ONLY
    AXLOG("[Mono] iOS platform - using AOT mode only");
#else
    AXLOG("[Mono] Platform supports JIT compilation");
#endif

    // Create Mono domain
    const char* domainName = "AxmolScriptingDomain";
    _domain = mono_jit_init(domainName);
    if (!_domain)
    {
        AXLOGERROR("[Mono] Failed to initialize JIT domain");
        return false;
    }

    AXLOG("[Mono] JIT domain created: %s", domainName);

    // Load main assembly if provided
    if (!config.configPath.empty())
    {
        if (!loadAssembly(config.configPath))
        {
            AXLOGERROR("[Mono] Failed to load main assembly");
            return false;
        }
        _mainAssembly = _loadedAssemblies[config.configPath];
        _mainImage = _loadedImages[config.configPath];
    }

    _initialized = true;
    AXLOG("[Mono] Initialization complete");
    return true;
}

bool MonoEngine::loadAssembly(const std::string& assemblyPath)
{
    if (!_initialized)
    {
        AXLOGERROR("[Mono] Engine not initialized");
        return false;
    }

    AXLOG("[Mono] Loading assembly: %s", assemblyPath.c_str());

    // Check if already loaded
    if (_loadedAssemblies.find(assemblyPath) != _loadedAssemblies.end())
    {
        AXLOG("[Mono] Assembly already loaded: %s", assemblyPath.c_str());
        return true;
    }

    // Verify file exists
    if (!std::filesystem::exists(assemblyPath))
    {
        AXLOGERROR("[Mono] Assembly file not found: %s", assemblyPath.c_str());
        return false;
    }

    // Open assembly
    MonoAssembly* assembly = mono_domain_assembly_open(_domain, assemblyPath.c_str());
    if (!assembly)
    {
        AXLOGERROR("[Mono] Failed to open assembly: %s", assemblyPath.c_str());
        return false;
    }

    // Get assembly image
    MonoImage* image = mono_assembly_get_image(assembly);
    if (!image)
    {
        AXLOGERROR("[Mono] Failed to get assembly image");
        return false;
    }

    // Store assembly and image
    _loadedAssemblies[assemblyPath] = assembly;
    _loadedImages[assemblyPath] = image;

    AXLOG("[Mono] Assembly loaded successfully: %s", assemblyPath.c_str());
    return true;
}

MonoMethod* MonoEngine::findMethod(MonoClass* klass, const std::string& methodName, int paramCount)
{
    if (!klass)
        return nullptr;

    // Search for method with matching name and parameter count
    MonoMethod* iter = nullptr;
    while ((iter = mono_class_get_methods(klass, (void**)&iter)))
    {
        const char* name = mono_method_get_name(iter);
        if (strcmp(name, methodName.c_str()) == 0)
        {
            MonoMethodSignature* sig = mono_method_signature(iter);
            int params = mono_signature_get_param_count(sig);
            if (paramCount < 0 || params == paramCount)
            {
                return iter;
            }
        }
    }

    return nullptr;
}

MonoObject* MonoEngine::invokeMethod(MonoMethod* method, MonoObject* instance, void** params)
{
    if (!method)
        return nullptr;

    MonoObject* exception = nullptr;
    MonoObject* result = mono_runtime_invoke(method, instance, params, &exception);

    if (exception)
    {
        // Get exception message
        MonoClass* exceptionClass = mono_object_get_class(exception);
        MonoProperty* msgProp = mono_class_get_property_from_name(exceptionClass, "Message");
        if (msgProp)
        {
            MonoMethod* getter = mono_property_get_get_method(msgProp);
            MonoObject* msgObj = mono_runtime_invoke(getter, exception, nullptr, nullptr);
            if (msgObj)
            {
                char* msg = mono_string_to_utf8((MonoString*)msgObj);
                AXLOGERROR("[Mono] Method invocation threw exception: %s", msg);
                mono_free(msg);
            }
        }
        return nullptr;
    }

    return result;
}

ScriptValue MonoEngine::executeMethod(const std::string& typeName,
                                       const std::string& methodName,
                                       const ScriptArgs* args)
{
    if (!_initialized || !_mainImage)
    {
        AXLOGERROR("[Mono] Engine not initialized or no main assembly");
        return ScriptValue{};
    }

    AXLOG("[Mono] Executing method: %s::%s", typeName.c_str(), methodName.c_str());

    // Parse namespace and class name
    std::string namespaceName;
    std::string className;

    size_t lastDot = typeName.find_last_of('.');
    if (lastDot != std::string::npos)
    {
        namespaceName = typeName.substr(0, lastDot);
        className = typeName.substr(lastDot + 1);
    }
    else
    {
        className = typeName;
    }

    AXLOG("[Mono] Looking for class: %s in namespace: %s",
          className.c_str(),
          namespaceName.empty() ? "(global)" : namespaceName.c_str());

    // Get class from image
    MonoClass* klass = mono_class_from_name(
        _mainImage,
        namespaceName.c_str(),
        className.c_str()
    );

    if (!klass)
    {
        AXLOGERROR("[Mono] Class not found: %s", typeName.c_str());
        return ScriptValue{};
    }

    // Find method (assuming no parameters for now)
    int paramCount = args ? static_cast<int>(args->values.size()) : 0;
    MonoMethod* method = findMethod(klass, methodName, paramCount);

    if (!method)
    {
        AXLOGERROR("[Mono] Method not found: %s::%s", typeName.c_str(), methodName.c_str());
        return ScriptValue{};
    }

    // Prepare parameters
    void** monoParams = nullptr;
    if (args && !args->values.empty())
    {
        monoParams = new void*[args->values.size()];
        for (size_t i = 0; i < args->values.size(); ++i)
        {
            // TODO: Convert ScriptValue to MonoObject*
            monoParams[i] = nullptr;
        }
    }

    // Invoke method (nullptr for static methods)
    MonoObject* result = invokeMethod(method, nullptr, monoParams);

    // Clean up parameters
    if (monoParams)
    {
        delete[] monoParams;
    }

    // Convert result
    if (result)
    {
        return convertMonoObjectToScriptValue(result);
    }

    AXLOG("[Mono] Method executed successfully");
    return ScriptValue{0}; // Success
}

ScriptValue MonoEngine::convertMonoObjectToScriptValue(MonoObject* obj)
{
    if (!obj)
        return ScriptValue{};

    MonoClass* klass = mono_object_get_class(obj);
    const char* className = mono_class_get_name(klass);

    // Handle basic types
    if (strcmp(className, "Int32") == 0)
    {
        return ScriptValue{*(int32_t*)mono_object_unbox(obj)};
    }
    else if (strcmp(className, "Int64") == 0)
    {
        return ScriptValue{*(int64_t*)mono_object_unbox(obj)};
    }
    else if (strcmp(className, "Single") == 0)
    {
        return ScriptValue{*(float*)mono_object_unbox(obj)};
    }
    else if (strcmp(className, "Double") == 0)
    {
        return ScriptValue{*(double*)mono_object_unbox(obj)};
    }
    else if (strcmp(className, "Boolean") == 0)
    {
        return ScriptValue{*(bool*)mono_object_unbox(obj)};
    }
    else if (strcmp(className, "String") == 0)
    {
        char* str = mono_string_to_utf8((MonoString*)obj);
        ScriptValue value{std::string(str)};
        mono_free(str);
        return value;
    }

    // For other types, return as handle
    return ScriptValue{(ScriptObjectHandle)obj};
}

MonoObject* MonoEngine::convertScriptValueToMonoObject(const ScriptValue& value)
{
    // TODO: Implement conversion from ScriptValue to MonoObject
    return nullptr;
}

ScriptObjectHandle MonoEngine::createInstance(const std::string& typeName)
{
    if (!_initialized || !_mainImage)
    {
        AXLOGERROR("[Mono] Engine not initialized or no main assembly");
        return nullptr;
    }

    AXLOG("[Mono] Creating instance of: %s", typeName.c_str());

    // Parse namespace and class name
    std::string namespaceName;
    std::string className;

    size_t lastDot = typeName.find_last_of('.');
    if (lastDot != std::string::npos)
    {
        namespaceName = typeName.substr(0, lastDot);
        className = typeName.substr(lastDot + 1);
    }
    else
    {
        className = typeName;
    }

    // Get class
    MonoClass* klass = mono_class_from_name(
        _mainImage,
        namespaceName.c_str(),
        className.c_str()
    );

    if (!klass)
    {
        AXLOGERROR("[Mono] Class not found: %s", typeName.c_str());
        return nullptr;
    }

    // Create instance
    MonoObject* obj = mono_object_new(_domain, klass);
    if (!obj)
    {
        AXLOGERROR("[Mono] Failed to create object");
        return nullptr;
    }

    // Call default constructor
    mono_runtime_object_init(obj);

    AXLOG("[Mono] Instance created successfully");
    return (ScriptObjectHandle)obj;
}

ScriptValue MonoEngine::callMethod(ScriptObjectHandle handle,
                                    const std::string& methodName,
                                    const ScriptArgs* args)
{
    if (!handle)
    {
        AXLOGERROR("[Mono] Invalid object handle");
        return ScriptValue{};
    }

    MonoObject* obj = (MonoObject*)handle;
    MonoClass* klass = mono_object_get_class(obj);

    AXLOG("[Mono] Calling instance method: %s", methodName.c_str());

    // Find method
    int paramCount = args ? static_cast<int>(args->values.size()) : 0;
    MonoMethod* method = findMethod(klass, methodName, paramCount);

    if (!method)
    {
        AXLOGERROR("[Mono] Method not found: %s", methodName.c_str());
        return ScriptValue{};
    }

    // Prepare parameters
    void** monoParams = nullptr;
    if (args && !args->values.empty())
    {
        monoParams = new void*[args->values.size()];
        for (size_t i = 0; i < args->values.size(); ++i)
        {
            // TODO: Convert ScriptValue to MonoObject*
            monoParams[i] = nullptr;
        }
    }

    // Invoke method on instance
    MonoObject* result = invokeMethod(method, obj, monoParams);

    // Clean up
    if (monoParams)
    {
        delete[] monoParams;
    }

    // Convert result
    if (result)
    {
        return convertMonoObjectToScriptValue(result);
    }

    return ScriptValue{0};
}

void MonoEngine::shutdown()
{
    if (!_initialized)
        return;

    AXLOG("[Mono] Shutting down");

    // Clear loaded assemblies
    _loadedAssemblies.clear();
    _loadedImages.clear();
    _mainAssembly = nullptr;
    _mainImage = nullptr;

    // Note: mono_jit_cleanup() is not recommended for embedded scenarios
    // as it may cause crashes. Just let the domain be cleaned up on process exit.
    // If you really need to clean up:
    // mono_jit_cleanup(_domain);

    _domain = nullptr;
    _initialized = false;

    AXLOG("[Mono] Shutdown complete");
}

} // namespace dotnet
} // namespace scripting
} // namespace ax
