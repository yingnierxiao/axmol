# 构建和测试指南

## Phase 1: 核心引擎测试

### Windows 测试

#### 1. 环境准备

```powershell
# 检查.NET SDK
dotnet --version
# 应该显示 8.0.x 或更高版本

# 设置环境变量（如果需要）
$env:DOTNET_ROOT = "C:\Program Files\dotnet"
```

#### 2. 构建C#项目

```powershell
cd extensions\scripting\dotnet-bindings\managed

# 构建C#程序集
dotnet build -c Release

# 输出文件应该在：
# bin\Release\net8.0\Axmol.Scripting.dll
# bin\Release\net8.0\Axmol.Scripting.runtimeconfig.json
```

#### 3. 构建C++项目

```powershell
# 返回项目根目录
cd ..\..\..\..

# 配置CMake（启用.NET支持）
cmake -B build -DAX_ENABLE_EXT_DOTNET=ON -DCMAKE_BUILD_TYPE=Release

# 构建
cmake --build build --config Release
```

#### 4. 准备测试环境

```powershell
# 复制C#输出到游戏资源目录
# 假设你的游戏项目在 projects\MyGame\
$GAME_RES = "projects\MyGame\Resources"

copy extensions\scripting\dotnet-bindings\managed\bin\Release\net8.0\*.dll $GAME_RES\
copy extensions\scripting\dotnet-bindings\managed\bin\Release\net8.0\*.json $GAME_RES\
```

#### 5. 集成测试代码

在你的游戏项目中（如 `projects/MyGame/Source/AppDelegate.cpp`）：

```cpp
#include "extensions/scripting/dotnet-bindings/common/ScriptEngine.h"
#include "extensions/scripting/dotnet-bindings/tests/BasicTest.cpp"

bool AppDelegate::applicationDidFinishLaunching()
{
    // ... 现有初始化代码 ...

    // 运行.NET基础测试
    AXLOG("===================================");
    AXLOG("Starting .NET Scripting Test");
    AXLOG("===================================");

    if (!DotNetBasicTest::runTest())
    {
        AXLOGERROR("FAILED: .NET test failed!");
        return false;
    }

    AXLOG("===================================");
    AXLOG("SUCCESS: .NET test passed!");
    AXLOG("===================================");

    // ... 继续游戏初始化 ...
    return true;
}
```

#### 6. 运行测试

```powershell
# 运行游戏
.\build\bin\Release\MyGame.exe
```

**预期输出：**

```
===================================
Starting .NET Scripting Test
===================================
=== Starting .NET Basic Test ===
[Test] Step 1: Initialize engine
[CoreCLR] Loading hostfxr library
[CoreCLR] Found hostfxr at: C:\...\hostfxr.dll
[CoreCLR] Initialization complete
[Test] PASSED: Engine initialized
[Test] Runtime type: CoreCLR
[Test] Step 2: Get engine instance
[Test] PASSED: Engine instance obtained
[Test] Step 3: Load assembly
[CoreCLR] Loading assembly: Axmol.Scripting.dll
[Test] PASSED: Assembly loaded
[Test] Step 4: Execute static method
[CoreCLR] Executing method: Axmol.Scripting.EngineAPI::axmol_dotnet_initialize
[Axmol.NET] Scripting system initialized
[Axmol.NET] Runtime: .NET 8.0.0
[Axmol.NET] Platform: Microsoft Windows 10.0.xxxxx
[Game] Initializing game logic
[Test] Method returned: 0
[Test] PASSED: Method executed successfully
[Test] Step 5: Shutdown
[CoreCLR] Shutting down
[Test] PASSED: Shutdown complete
=== .NET Basic Test COMPLETED SUCCESSFULLY ===
===================================
SUCCESS: .NET test passed!
===================================
```

---

### Linux 测试

#### 1. 环境准备

```bash
# 安装.NET SDK
wget https://dot.net/v1/dotnet-install.sh
chmod +x dotnet-install.sh
./dotnet-install.sh --channel 8.0

# 设置环境变量
export DOTNET_ROOT=$HOME/.dotnet
export PATH=$PATH:$DOTNET_ROOT
export LD_LIBRARY_PATH=$DOTNET_ROOT:$LD_LIBRARY_PATH

# 验证安装
dotnet --version
```

#### 2. 构建C#项目

```bash
cd extensions/scripting/dotnet-bindings/managed
dotnet build -c Release
```

#### 3. 构建C++项目

```bash
cd ../../../../
cmake -B build -DAX_ENABLE_EXT_DOTNET=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

#### 4. 运行测试

```bash
# 复制C#输出
GAME_RES="projects/MyGame/Resources"
cp extensions/scripting/dotnet-bindings/managed/bin/Release/net8.0/*.dll $GAME_RES/
cp extensions/scripting/dotnet-bindings/managed/bin/Release/net8.0/*.json $GAME_RES/

# 运行
./build/bin/MyGame
```

---

### macOS 测试

#### 1. 环境准备

```bash
# 使用Homebrew安装
brew install dotnet-sdk

# 设置环境变量
export DOTNET_ROOT=/usr/local/share/dotnet
export PATH=$PATH:$DOTNET_ROOT
```

#### 2. 构建和测试

与Linux相同，使用上述Linux步骤。

---

### iOS 测试（需要Mono）

#### 1. 环境准备

```bash
# 安装Mono
brew install mono

# 验证
mono --version
```

#### 2. 创建Xcode项目

```bash
cmake -B build-ios \
    -G Xcode \
    -DCMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake \
    -DPLATFORM=OS64 \
    -DAX_ENABLE_EXT_DOTNET=ON
```

#### 3. AOT编译C#程序集

```bash
cd extensions/scripting/dotnet-bindings/managed

# 编译DLL
dotnet build -c Release

# AOT编译（iOS必需）
mono --aot=full,static bin/Release/net8.0/Axmol.Scripting.dll
# 这将生成 Axmol.Scripting.dll.o

# 将.o文件添加到Xcode项目
```

#### 4. 在Xcode中测试

- 打开 `build-ios/axmol.xcodeproj`
- 添加 `Axmol.Scripting.dll.o` 到项目
- 链接Mono库
- 运行到iOS设备或模拟器

---

### Android 测试（需要Mono）

#### 1. 环境准备

```bash
# 设置Android NDK
export ANDROID_NDK=/path/to/ndk

# 安装Mono
# Ubuntu:
sudo apt-get install mono-complete
```

#### 2. 构建项目

```bash
cmake -B build-android \
    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-24 \
    -DAX_ENABLE_EXT_DOTNET=ON

cmake --build build-android
```

#### 3. 打包APK

需要在APK中包含：
- `libmono-android.so`
- `libmonosgen-2.0.so`
- `Axmol.Scripting.dll`

---

## 故障排除

### 问题1: nethost.lib 找不到

**Windows:**
```powershell
# 手动指定路径
cmake -B build -DAX_ENABLE_EXT_DOTNET=ON `
    -DNETHOST_LIB="C:\Program Files\dotnet\packs\Microsoft.NETCore.App.Host.win-x64\8.0.0\runtimes\win-x64\native\nethost.lib"
```

**Linux/macOS:**
```bash
# 确保DOTNET_ROOT正确
export DOTNET_ROOT=/usr/share/dotnet  # 或你的安装路径
```

### 问题2: 运行时找不到hostfxr.dll

**Windows:**
- 添加到PATH或复制到游戏exe目录
- 或使用self-contained发布

**Linux:**
```bash
export LD_LIBRARY_PATH=$DOTNET_ROOT:$LD_LIBRARY_PATH
```

### 问题3: 调用C#方法失败

检查：
1. 方法是否有 `[UnmanagedCallersOnly]` 属性
2. EntryPoint 名称是否匹配
3. 程序集是否正确加载

```csharp
// ✅ 正确
[UnmanagedCallersOnly(EntryPoint = "axmol_dotnet_initialize")]
public static int Initialize(IntPtr args, int size) { ... }

// ❌ 错误
public static int Initialize(IntPtr args, int size) { ... }
```

### 问题4: iOS编译失败

确保：
1. 使用AOT编译所有程序集
2. 不使用动态代码生成（Reflection.Emit等）
3. 所有方法使用 `[UnmanagedCallersOnly]` 而不是委托

---

## 验证清单

### ✅ Windows
- [ ] CMake配置成功
- [ ] C++编译通过
- [ ] C#程序集构建
- [ ] BasicTest运行成功
- [ ] 日志输出正确

### ✅ Linux
- [ ] CMake配置成功
- [ ] C++编译通过
- [ ] C#程序集构建
- [ ] BasicTest运行成功
- [ ] nethost.so加载成功

### ✅ macOS
- [ ] CMake配置成功（x64和arm64）
- [ ] C++编译通过
- [ ] C#程序集构建
- [ ] BasicTest运行成功
- [ ] 签名问题解决

### ✅ iOS
- [ ] Mono安装正确
- [ ] AOT编译成功
- [ ] Xcode项目配置
- [ ] 设备测试通过
- [ ] 无JIT错误

### ✅ Android
- [ ] NDK配置正确
- [ ] Mono库打包
- [ ] APK安装成功
- [ ] 程序集加载
- [ ] 运行测试通过

---

## 性能测试

运行性能测试验证调用开销：

```cpp
#include <chrono>

void performanceTest()
{
    auto* engine = ScriptEngineManager::getInstance().getEngine();

    const int iterations = 10000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i)
    {
        engine->executeMethod("Axmol.Scripting.EngineAPI", "axmol_dotnet_update");
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    double avgUs = duration.count() / (double)iterations;
    AXLOG("Performance: %d calls in %lld μs (avg: %.2f μs/call)",
          iterations, duration.count(), avgUs);
}
```

**预期结果：**
- CoreCLR: < 1 μs/call
- Mono: < 2 μs/call

---

## 下一步

一旦所有平台的基础测试通过，就可以进入：
- **Phase 2**: 完整的API绑定
- **Phase 3**: 实际游戏示例
