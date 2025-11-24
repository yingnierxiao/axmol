/****************************************************************************
 Copyright (c) 2025 Axmol Engine contributors

 https://axmol.dev/

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#pragma once

#include <string>
#include <memory>
#include "ScriptTypes.h"

namespace ax {
namespace scripting {
namespace dotnet {

/**
 * @brief Abstract base class for .NET runtime engines
 *
 * This provides a unified interface for different .NET runtime implementations:
 * - CoreCLR for desktop platforms (Windows/Linux/macOS)
 * - Mono for mobile platforms (iOS/Android)
 */
class ScriptEngine
{
public:
    virtual ~ScriptEngine() = default;

    /**
     * @brief Initialize the .NET runtime
     * @param config Runtime configuration (runtimeconfig.json path for CoreCLR, or assembly path for Mono)
     * @return true if initialization succeeded
     */
    virtual bool initialize(const ScriptEngineConfig& config) = 0;

    /**
     * @brief Load a .NET assembly
     * @param assemblyPath Path to the .dll file
     * @return true if assembly loaded successfully
     */
    virtual bool loadAssembly(const std::string& assemblyPath) = 0;

    /**
     * @brief Execute a static method in C#
     * @param typeName Fully qualified type name (e.g., "MyNamespace.MyClass")
     * @param methodName Method name
     * @param args Optional arguments
     * @return Return value from the method
     */
    virtual ScriptValue executeMethod(const std::string& typeName,
                                       const std::string& methodName,
                                       const ScriptArgs* args = nullptr) = 0;

    /**
     * @brief Create an instance of a C# class
     * @param typeName Fully qualified type name
     * @return Handle to the managed object
     */
    virtual ScriptObjectHandle createInstance(const std::string& typeName) = 0;

    /**
     * @brief Call an instance method on a managed object
     * @param handle Object handle
     * @param methodName Method name
     * @param args Optional arguments
     * @return Return value from the method
     */
    virtual ScriptValue callMethod(ScriptObjectHandle handle,
                                    const std::string& methodName,
                                    const ScriptArgs* args = nullptr) = 0;

    /**
     * @brief Shutdown the .NET runtime and release all resources
     */
    virtual void shutdown() = 0;

    /**
     * @brief Get the type of runtime (CoreCLR or Mono)
     */
    virtual RuntimeType getRuntimeType() const = 0;

    /**
     * @brief Check if the runtime is initialized
     */
    virtual bool isInitialized() const = 0;

    /**
     * @brief Factory method to create appropriate engine for current platform
     * @return Unique pointer to platform-specific engine implementation
     */
    static std::unique_ptr<ScriptEngine> create();

protected:
    ScriptEngine() = default;
};

/**
 * @brief Singleton accessor for the global script engine instance
 */
class ScriptEngineManager
{
public:
    static ScriptEngineManager& getInstance();

    ScriptEngine* getEngine() const { return _engine.get(); }

    bool initialize(const ScriptEngineConfig& config);
    void shutdown();

private:
    ScriptEngineManager() = default;
    ~ScriptEngineManager() = default;
    ScriptEngineManager(const ScriptEngineManager&) = delete;
    ScriptEngineManager& operator=(const ScriptEngineManager&) = delete;

    std::unique_ptr<ScriptEngine> _engine;
};

} // namespace dotnet
} // namespace scripting
} // namespace ax
