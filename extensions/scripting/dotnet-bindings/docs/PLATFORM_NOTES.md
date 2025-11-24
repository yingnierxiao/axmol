# 平台特定说明

## Windows (CoreCLR)

### 环境配置
```cmd
# 安装 .NET SDK
winget install Microsoft.DotNet.SDK.8

# 或下载安装
# https://dotnet.microsoft.com/download

# 验证安装
dotnet --version
```

### CMake配置
```bash
cmake -B build -DAX_ENABLE_EXT_DOTNET=ON -DCMAKE_BUILD_TYPE=Release
```

### 运行时分发
打包时需要包含：
- `nethost.dll` - 位于 `%DOTNET_ROOT%\packs\Microsoft.NETCore.App.Host.win-x64\*\runtimes\win-x64\native\`
- `.NET Runtime` - 或使用 self-contained 部署

---

## Linux (CoreCLR)

### 环境配置
```bash
# Ubuntu/Debian
wget https://dot.net/v1/dotnet-install.sh
chmod +x dotnet-install.sh
./dotnet-install.sh --channel 8.0

# Arch Linux
sudo pacman -S dotnet-sdk

# 设置环境变量
export DOTNET_ROOT=$HOME/.dotnet
export PATH=$PATH:$DOTNET_ROOT
```

### 依赖库
```bash
# 需要安装
sudo apt-get install libicu-dev
```

### CMake配置
```bash
cmake -B build -DAX_ENABLE_EXT_DOTNET=ON
make -C build -j$(nproc)
```

---

## macOS (CoreCLR)

### 环境配置
```bash
# 使用 Homebrew
brew install dotnet-sdk

# 或下载 PKG 安装
# https://dotnet.microsoft.com/download

# 设置环境变量
export DOTNET_ROOT=/usr/local/share/dotnet
export PATH=$PATH:$DOTNET_ROOT
```

### CMake配置
```bash
cmake -B build -G Xcode -DAX_ENABLE_EXT_DOTNET=ON
cmake --build build --config Release
```

### 签名问题
macOS Catalina+ 需要对 `libnethost.dylib` 进行签名：
```bash
codesign --force --deep --sign - libnethost.dylib
```

---

## iOS (Mono)

### 环境配置
```bash
# 安装 Mono
brew install mono

# 或使用 Xamarin.iOS
# https://dotnet.microsoft.com/apps/xamarin
```

### AOT 编译要求
iOS **不支持JIT**，必须使用AOT (Ahead-of-Time) 编译：

```bash
# 编译为AOT程序集
mono --aot=full YourAssembly.dll
```

### C# 代码限制
```csharp
// ❌ 不支持：动态委托
public delegate void DynamicDelegate();
DynamicDelegate del = () => { };

// ✅ 支持：UnmanagedCallersOnly
[UnmanagedCallersOnly(EntryPoint = "my_function")]
public static void MyFunction() { }

// ❌ 不支持：Reflection.Emit
var method = new DynamicMethod(...);

// ✅ 支持：Source Generators（编译时）
```

### CMake配置
```bash
cmake -B build-ios \
    -G Xcode \
    -DCMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake \
    -DPLATFORM=OS64 \
    -DAX_ENABLE_EXT_DOTNET=ON

cmake --build build-ios --config Release
```

### 打包
```bash
# 1. 编译C#为AOT
mono --aot=full,static Axmol.Scripting.dll

# 2. 生成的 .o 文件需要链接到iOS应用
# Axmol.Scripting.dll.o
```

---

## Android (Mono)

### 环境配置
```bash
# 安装 Mono
# Ubuntu
sudo apt-get install mono-complete

# macOS
brew install mono
```

### CMake配置
```bash
export ANDROID_NDK=/path/to/ndk

cmake -B build-android \
    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-24 \
    -DAX_ENABLE_EXT_DOTNET=ON

cmake --build build-android
```

### Mono 库集成
Android需要包含Mono运行时：

```cmake
# 在 CMakeLists.txt 中
if(ANDROID)
    # 方式1：使用系统 Mono
    find_library(MONO_LIB mono-2.0)

    # 方式2：使用预编译的 Mono
    set(MONO_LIBS
        ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/mono/android/${ANDROID_ABI}/libmono-android.release.so
    )
    target_link_libraries(${PROJECT_NAME} ${MONO_LIBS})
endif()
```

### APK 打包
需要在APK中包含：
- `libmono-android.so`
- `libmonosgen-2.0.so`
- C#程序集 (.dll)

---

## 跨平台API兼容性

### ✅ 所有平台支持
```csharp
// 基本类型
int, float, string, bool

// System.* 核心库
System.Collections.Generic
System.Linq
System.IO
System.Text

// P/Invoke
[DllImport("...")]
[UnmanagedCallersOnly]
```

### ⚠️ 平台限制

| 功能 | Windows | Linux | macOS | iOS | Android |
|------|---------|-------|-------|-----|---------|
| JIT编译 | ✅ | ✅ | ✅ | ❌ | ✅ |
| Reflection.Emit | ✅ | ✅ | ✅ | ❌ | ✅ |
| 动态加载DLL | ✅ | ✅ | ✅ | ❌ | ✅ |
| 调试器附加 | ✅ | ✅ | ✅ | ⚠️ | ⚠️ |

### iOS特殊处理
```csharp
#if IOS
    // iOS特定代码（AOT安全）
    [UnmanagedCallersOnly]
    public static void IOSOnlyMethod() { }
#else
    // 其他平台
    public void RegularMethod() { }
#endif
```

---

## 性能对比

| 平台 | 启动时间 | 调用开销 | 内存占用 |
|------|----------|----------|----------|
| **Windows (CoreCLR)** | ~200ms | ~10ns | ~80MB |
| **Linux (CoreCLR)** | ~180ms | ~10ns | ~70MB |
| **macOS (CoreCLR)** | ~220ms | ~10ns | ~80MB |
| **iOS (Mono AOT)** | ~100ms | ~5ns | ~20MB |
| **Android (Mono)** | ~150ms | ~8ns | ~25MB |

*测试环境：空项目，仅初始化运行时*

---

## 常见问题

### Q: 为什么iOS不能用CoreCLR？
A: Apple禁止应用在运行时生成可执行代码（JIT），CoreCLR依赖JIT，因此无法使用。Mono支持纯AOT模式。

### Q: Android可以用CoreCLR吗？
A: 技术上可以，但Mono更轻量，且有更好的Android集成（Xamarin.Android）。

### Q: 如何在iOS上调试C#代码？
A: 使用Xamarin Studio或Visual Studio for Mac，支持远程调试AOT代码。

### Q: 程序集大小如何优化？
A: 使用IL Linker（Trimming）：
```xml
<PropertyGroup>
  <PublishTrimmed>true</PublishTrimmed>
  <TrimMode>link</TrimMode>
</PropertyGroup>
```

---

## 下一步

- 参考 [README.md](../README.md) 了解基本使用
- 查看 [API_REFERENCE.md](API_REFERENCE.md) 了解完整API
- 查看 `tests/dotnet-tests/` 中的示例代码
