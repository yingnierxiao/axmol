# Windows平台完整测试指南

## 🎯 测试目标
在Windows上验证.NET脚本绑定的完整功能

---

## 📋 前置要求检查

### 1. 检查.NET SDK

```powershell
# 打开PowerShell，运行：
dotnet --version

# 应该显示 8.0.x 或更高版本
# 如果没有，下载安装：
# https://dotnet.microsoft.com/download/dotnet/8.0
```

### 2. 检查环境变量

```powershell
# 检查DOTNET_ROOT
$env:DOTNET_ROOT

# 如果为空，设置：
$env:DOTNET_ROOT = "C:\Program Files\dotnet"

# 永久设置（可选）：
[System.Environment]::SetEnvironmentVariable('DOTNET_ROOT', 'C:\Program Files\dotnet', 'User')
```

### 3. 检查CMake和编译器

```powershell
# 检查CMake
cmake --version
# 需要 3.22+

# 检查MSVC（Visual Studio）
# 需要 Visual Studio 2019 或 2022
```

---

## 🚀 步骤1: 启用.NET扩展

### 修改 `core/CMakeLists.txt`

找到扩展选项部分（大约在第122行附近），添加：

```cmake
# 在 AX_ENABLE_EXT_LUA 等选项之后添加
option(AX_ENABLE_EXT_DOTNET "Build .NET scripting support" ON)
```

### 修改 `extensions/CMakeLists.txt`

在文件末尾（大约第110行之后）添加：

```cmake
# .NET Scripting Extension
if(AX_ENABLE_EXT_DOTNET)
  message(STATUS "Adding .NET scripting extension")
  add_subdirectory(scripting/dotnet-bindings)
endif()
```

---

## 🔨 步骤2: 构建C#项目

```powershell
# 进入C#项目目录
cd D:\COP\cop_mytools\axmol\extensions\scripting\dotnet-bindings\managed

# 复制模板文件
copy Axmol.Scripting.csproj.template Axmol.Scripting.csproj
copy EngineAPI.cs.template EngineAPI.cs

# 构建C#程序集
dotnet build -c Release

# 验证输出
dir bin\Release\net8.0\
# 应该看到：
# - Axmol.Scripting.dll
# - Axmol.Scripting.runtimeconfig.json
```

**预期输出：**
```
Microsoft (R) Build Engine version 17.8.x
...
Build succeeded.
    0 Warning(s)
    0 Error(s)
```

---

## 🏗️ 步骤3: 构建C++项目

```powershell
# 返回项目根目录
cd D:\COP\cop_mytools\axmol

# 配置CMake（首次）
cmake -B build -DAX_ENABLE_EXT_DOTNET=ON -DCMAKE_BUILD_TYPE=Release

# 如果遇到nethost找不到的错误，添加路径：
cmake -B build -DAX_ENABLE_EXT_DOTNET=ON `
    -DDOTNET_ROOT="C:\Program Files\dotnet"

# 构建项目
cmake --build build --config Release

# 或者使用Visual Studio
# 打开 build\axmol.sln，选择Release模式编译
```

**预期输出：**
```
-- Adding .NET scripting extension
-- .NET Scripting: Using CoreCLR for desktop platform
-- .NET Scripting: Found nethost at C:\Program Files\dotnet\...
-- .NET Scripting: Configuration complete (CoreCLR)
...
Build succeeded.
```

---

## 🧪 步骤4: 创建测试项目

### 4.1 使用现有测试项目（推荐）

如果你有 `projects/SlotGame` 或其他项目：

```powershell
# 复制C#输出到项目资源目录
$GAME_RES = "D:\COP\cop_mytools\axmol\projects\SlotGame\Content"

copy extensions\scripting\dotnet-bindings\managed\bin\Release\net8.0\*.dll $GAME_RES\
copy extensions\scripting\dotnet-bindings\managed\bin\Release\net8.0\*.json $GAME_RES\

# 验证
dir $GAME_RES\*.dll
dir $GAME_RES\*.json
```

### 4.2 或创建新的测试项目

```powershell
# 创建测试目录
mkdir extensions\scripting\dotnet-bindings\test-project
cd extensions\scripting\dotnet-bindings\test-project

# 创建测试应用
New-Item -ItemType File -Name TestApp.cpp
```

**TestApp.cpp:**
```cpp
#include "axmol.h"
#include "../common/ScriptEngine.h"
#include "../tests/BasicTest.cpp"

int main()
{
    // 设置资源路径
    ax::FileUtils::getInstance()->setDefaultResourceRootPath("D:/COP/cop_mytools/axmol/extensions/scripting/dotnet-bindings/managed/bin/Release/net8.0");

    // 运行测试
    if (DotNetBasicTest::runTest())
    {
        std::cout << "\n=== TEST PASSED ===" << std::endl;
        return 0;
    }
    else
    {
        std::cerr << "\n=== TEST FAILED ===" << std::endl;
        return 1;
    }
}
```

---

## 🎮 步骤5: 集成到现有游戏

### 修改 `projects/SlotGame/Source/AppDelegate.cpp`

```cpp
// 在文件顶部添加
#include "extensions/scripting/dotnet-bindings/common/ScriptEngine.h"

bool AppDelegate::applicationDidFinishLaunching()
{
    // ... 现有初始化代码 ...

    // ========== .NET 脚本初始化 ==========
    AXLOG("========================================");
    AXLOG("Initializing .NET Scripting");
    AXLOG("========================================");

    auto& scriptManager = ax::scripting::dotnet::ScriptEngineManager::getInstance();

    ax::scripting::dotnet::ScriptEngineConfig config;
    config.configPath = ax::FileUtils::getInstance()->fullPathForFilename(
        "Axmol.Scripting.runtimeconfig.json"
    );

    if (scriptManager.initialize(config))
    {
        AXLOG("[AppDelegate] .NET Engine initialized");

        auto* engine = scriptManager.getEngine();

        // 加载程序集
        std::string dllPath = ax::FileUtils::getInstance()->fullPathForFilename(
            "Axmol.Scripting.dll"
        );

        if (engine->loadAssembly(dllPath))
        {
            AXLOG("[AppDelegate] Assembly loaded");

            // 调用C#初始化方法
            ax::scripting::dotnet::ScriptValue result = engine->executeMethod(
                "Axmol.Scripting.EngineAPI",
                "axmol_dotnet_initialize"
            );

            AXLOG("[AppDelegate] C# Initialize called");
        }
    }
    else
    {
        AXLOGERROR("[AppDelegate] Failed to initialize .NET engine");
    }

    AXLOG("========================================");
    // ========== .NET 初始化完成 ==========

    // ... 继续现有代码 ...
    return true;
}
```

---

## ▶️ 步骤6: 运行测试

### 方式1: 直接运行游戏

```powershell
# 如果使用SlotGame
cd D:\COP\cop_mytools\axmol\projects\SlotGame\build\windows\x64\release

# 运行
.\SlotGame.exe

# 或者从build目录
cd D:\COP\cop_mytools\axmol\build\bin\Release
.\SlotGame.exe
```

### 方式2: Visual Studio调试

1. 打开 `build/axmol.sln`
2. 设置 SlotGame 为启动项目
3. 按 F5 调试运行

---

## ✅ 预期输出

运行游戏后，在控制台应该看到：

```
========================================
Initializing .NET Scripting
========================================
[.NET] Creating CoreCLR engine for desktop platform
[CoreCLR] Engine instance created
[CoreCLR] Initializing with config: D:/COP/.../Axmol.Scripting.runtimeconfig.json
[CoreCLR] Loading hostfxr library
[CoreCLR] Found hostfxr at: C:\Program Files\dotnet\host\fxr\8.0.x\hostfxr.dll
[CoreCLR] hostfxr loaded successfully
[CoreCLR] Runtime context initialized
[CoreCLR] Function pointers loaded
[CoreCLR] Initialization complete
[AppDelegate] .NET Engine initialized
[CoreCLR] Loading assembly: D:/COP/.../Axmol.Scripting.dll
[CoreCLR] Assembly registered: D:/COP/.../Axmol.Scripting.dll
[AppDelegate] Assembly loaded
[CoreCLR] Executing method: Axmol.Scripting.EngineAPI::axmol_dotnet_initialize
[Axmol.NET] Scripting system initialized
[Axmol.NET] Runtime: .NET 8.0.0
[Axmol.NET] Platform: Microsoft Windows 10.0.xxxxx
[Game] Initializing game logic
[CoreCLR] Method executed successfully. Return value: 0
[AppDelegate] C# Initialize called
========================================
```

---

## 🐛 故障排除

### 问题1: "nethost.lib not found"

```powershell
# 手动查找nethost.lib位置
dir "C:\Program Files\dotnet\packs\" -Recurse -Filter "nethost.lib"

# 设置CMake变量
cmake -B build -DAX_ENABLE_EXT_DOTNET=ON `
    -DNETHOST_LIB="C:\Program Files\dotnet\packs\Microsoft.NETCore.App.Host.win-x64\8.0.0\runtimes\win-x64\native\nethost.lib"
```

### 问题2: "hostfxr.dll not found" 运行时错误

**方式1: 添加到PATH**
```powershell
$env:PATH += ";C:\Program Files\dotnet\host\fxr\8.0.0"
```

**方式2: 复制DLL到游戏目录**
```powershell
copy "C:\Program Files\dotnet\host\fxr\8.0.0\hostfxr.dll" build\bin\Release\
```

**方式3: 使用Self-Contained发布**
```powershell
cd extensions\scripting\dotnet-bindings\managed
dotnet publish -c Release --self-contained -r win-x64
```

### 问题3: "runtimeconfig.json not found"

```powershell
# 确认文件存在
dir extensions\scripting\dotnet-bindings\managed\bin\Release\net8.0\*.json

# 确认复制到了资源目录
dir projects\SlotGame\Content\*.json

# 手动复制
copy extensions\scripting\dotnet-bindings\managed\bin\Release\net8.0\*.json projects\SlotGame\Content\
```

### 问题4: C#方法调用失败 "Method not found"

检查C#代码：

```csharp
// ✅ 正确 - 必须有 UnmanagedCallersOnly 和 EntryPoint
[UnmanagedCallersOnly(EntryPoint = "axmol_dotnet_initialize")]
public static int Initialize(IntPtr args, int size)
{
    // ...
    return 0;
}

// ❌ 错误 - 缺少属性
public static int Initialize(IntPtr args, int size)
{
    // ...
}
```

### 问题5: 编译错误 "ScriptEngine.h not found"

确保CMake正确配置了include路径：

```powershell
# 重新配置
cmake -B build -DAX_ENABLE_EXT_DOTNET=ON

# 检查输出
# 应该看到: "Adding .NET scripting extension"
```

---

## 🔍 调试技巧

### 启用详细日志

在C++代码中添加：

```cpp
// 在初始化前
ax::log("Current directory: %s", std::filesystem::current_path().string().c_str());
ax::log("Resource root: %s", ax::FileUtils::getInstance()->getDefaultResourceRootPath().c_str());

// 检查文件是否存在
std::string dllPath = ax::FileUtils::getInstance()->fullPathForFilename("Axmol.Scripting.dll");
ax::log("DLL path: %s", dllPath.c_str());
ax::log("DLL exists: %s", std::filesystem::exists(dllPath) ? "YES" : "NO");
```

### 使用Visual Studio调试器

1. 设置断点在 `AppDelegate::applicationDidFinishLaunching`
2. 按F5启动调试
3. 逐步执行查看变量值
4. 查看输出窗口的日志

---

## 📊 性能测试

添加性能测试代码：

```cpp
#include <chrono>

void performanceTest()
{
    auto* engine = ax::scripting::dotnet::ScriptEngineManager::getInstance().getEngine();

    const int iterations = 10000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i)
    {
        engine->executeMethod("Axmol.Scripting.EngineAPI", "axmol_dotnet_initialize");
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    double avgUs = duration.count() / (double)iterations;
    AXLOG("Performance: %d calls in %lld μs (avg: %.2f μs/call)",
          iterations, duration.count(), avgUs);
}
```

**预期结果：**
- 平均调用时间: < 1 μs/call
- 10,000次调用: < 10ms

---

## ✅ 验证清单

完成测试后，确认以下内容：

- [ ] CMake配置成功（看到 "Adding .NET scripting extension"）
- [ ] C#项目编译成功（生成了.dll和.json文件）
- [ ] C++项目编译成功（无错误）
- [ ] 游戏能启动
- [ ] 控制台输出正确的日志
- [ ] 看到 "[Axmol.NET] Scripting system initialized"
- [ ] 看到 "[Game] Initializing game logic"
- [ ] 无崩溃或错误

---

## 🎉 测试成功！

如果看到以下输出，说明测试成功：

```
[Axmol.NET] Scripting system initialized
[Axmol.NET] Runtime: .NET 8.0.0
[Game] Initializing game logic
```

恭喜！你已经在Windows上成功运行了.NET脚本绑定！

---

## 下一步

✅ Windows测试通过后，可以：

1. **测试更多功能**
   - 调用多个C#方法
   - 传递参数
   - 测试性能

2. **其他平台测试**
   - Linux测试
   - macOS测试

3. **Phase 2: API绑定**
   - 导出Axmol API
   - 创建C#包装

---

## 获取帮助

如果遇到问题：
1. 查看 `PHASE1_COMPLETE.md` 的故障排除部分
2. 查看 `BUILD_AND_TEST.md` 的详细说明
3. 在Discord或GitHub提问

**测试愉快！** 🚀
