# macOS和Linux平台.NET支持 - 构建指南

## 当前状态

✅ **已完成**：
- CMake配置已支持macOS和Linux平台检测
- CoreCLR引擎实现（coreclr/CoreCLREngine.cpp）
- 跨平台架构设计
- 自动查找.NET SDK路径
- nethost库链接配置

✅ **运行时要求**：
- macOS和Linux使用**CoreCLR运行时**（与Windows相同）
- 支持JIT编译，性能最优
- 需要系统安装.NET SDK 8.0+

⏳ **待测试**：
- 在macOS系统上编译测试
- 在Linux系统上编译测试
- 验证.NET程序集加载和执行

---

## macOS平台

### 环境准备

**系统要求**：
- macOS 10.15 Catalina或更高
- Xcode 12.0+（提供编译器）
- CMake 3.22+
- .NET SDK 8.0+

**安装.NET SDK**：
```bash
# 方法1：使用Homebrew（推荐）
brew install dotnet-sdk

# 方法2：从官网下载安装包
# https://dotnet.microsoft.com/download

# 验证安装
dotnet --version
# 输出：8.0.x
```

**验证nethost库**：
```bash
# nethost库应该在以下位置
ls /usr/local/share/dotnet/packs/Microsoft.NETCore.App.Host.osx-x64/*/runtimes/osx-x64/native/libnethost.dylib

# 或检查DOTNET_ROOT
echo $DOTNET_ROOT
ls $DOTNET_ROOT/packs/Microsoft.NETCore.App.Host.osx-x64/*/runtimes/osx-x64/native/
```

### 构建步骤

#### 1. 配置CMake
```bash
cd projects/SlotGame

# 配置项目（Debug模式）
cmake -B build \
      -DCMAKE_BUILD_TYPE=Debug \
      -DAX_ENABLE_EXT_DOTNET=ON \
      -G "Unix Makefiles"

# 或使用Xcode生成器（如果喜欢用Xcode）
cmake -B build-xcode \
      -DCMAKE_BUILD_TYPE=Debug \
      -DAX_ENABLE_EXT_DOTNET=ON \
      -G Xcode
```

**CMake输出应包含**：
```
.NET Scripting: Using CoreCLR for desktop platform
.NET Scripting: Found nethost at /usr/local/share/dotnet/.../libnethost.dylib
.NET Scripting: Using nethost include directory: .../native
.NET Scripting: Configuration complete (CoreCLR)
```

#### 2. 编译
```bash
# Unix Makefiles方式
cmake --build build --config Debug --target SlotGame

# Xcode方式
cmake --build build-xcode --config Debug --target SlotGame
# 或打开Xcode项目：open build-xcode/SlotGame.xcodeproj
```

#### 3. 编译C#程序集
```bash
cd extensions/scripting/dotnet-bindings/managed

# 编译为macOS目标
dotnet build -c Release
```

#### 4. 部署和运行
```bash
# 复制DLL到游戏可执行文件目录
cp extensions/scripting/dotnet-bindings/managed/bin/Release/net8.0/Axmol.Scripting.dll \
   projects/SlotGame/build/bin/SlotGame/Debug/

cp extensions/scripting/dotnet-bindings/managed/bin/Release/net8.0/Axmol.Scripting.runtimeconfig.json \
   projects/SlotGame/build/bin/SlotGame/Debug/

# 运行游戏
cd projects/SlotGame/build/bin/SlotGame/Debug
./SlotGame
```

#### 5. 验证.NET初始化
游戏启动后，检查日志输出：
```
========================================
初始化 .NET 脚本引擎...
========================================
[CoreCLR] Initializing CoreCLR runtime...
[CoreCLR] Runtime loaded successfully
[AppDelegate] .NET Engine initialized successfully
[CoreCLR] Method executed successfully. Return value: 0
[AppDelegate] .NET C# Initialize method called
========================================
```

并检查是否创建了标记文件：
```bash
cat DOTNET_INITIALIZED.txt
# 输出：
# [Axmol.NET] Initialized at 2025/11/24 ...
# Runtime: .NET 8.0.20
# Platform: Darwin ...
```

### macOS特定注意事项

**1. 代码签名（Release构建）**：
```bash
# macOS可能要求应用签名
codesign -s - ./SlotGame

# 或在CMakeLists.txt中配置
set_target_properties(SlotGame PROPERTIES
    MACOSX_BUNDLE TRUE
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
)
```

**2. 库搜索路径**：
macOS使用`@rpath`机制，确保nethost.dylib可以被找到：
```bash
# 检查依赖
otool -L SlotGame | grep nethost

# 如果需要，设置运行时搜索路径
install_name_tool -add_rpath /usr/local/share/dotnet SlotGame
```

**3. Apple Silicon（M1/M2）**：
确保使用arm64架构：
```bash
# 检查架构
file SlotGame
# 输出应该包含：Mach-O 64-bit executable arm64

# 如果需要通用二进制（x86_64 + arm64）
cmake -B build-universal \
      -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" \
      -DAX_ENABLE_EXT_DOTNET=ON
```

---

## Linux平台

### 环境准备

**系统要求**：
- Ubuntu 20.04+ / Debian 11+ / Fedora 36+ / Arch Linux
- GCC 9+ 或 Clang 10+
- CMake 3.22+
- .NET SDK 8.0+

**安装.NET SDK**：

```bash
# Ubuntu/Debian
wget https://packages.microsoft.com/config/ubuntu/22.04/packages-microsoft-prod.deb -O packages-microsoft-prod.deb
sudo dpkg -i packages-microsoft-prod.deb
sudo apt update
sudo apt install -y dotnet-sdk-8.0

# Fedora
sudo dnf install dotnet-sdk-8.0

# Arch Linux
sudo pacman -S dotnet-sdk

# 验证安装
dotnet --version
```

**安装构建依赖**：
```bash
# Ubuntu/Debian
sudo apt install -y build-essential cmake git \
    libx11-dev libxrandr-dev libxcursor-dev libxi-dev \
    libudev-dev libgl1-mesa-dev

# Fedora
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake libX11-devel mesa-libGL-devel

# Arch Linux
sudo pacman -S base-devel cmake libx11 mesa
```

**验证nethost库**：
```bash
# nethost库应该在以下位置
ls /usr/share/dotnet/packs/Microsoft.NETCore.App.Host.linux-x64/*/runtimes/linux-x64/native/libnethost.so

# 或检查DOTNET_ROOT
echo $DOTNET_ROOT
ls $DOTNET_ROOT/packs/Microsoft.NETCore.App.Host.linux-x64/*/runtimes/linux-x64/native/
```

### 构建步骤

#### 1. 配置CMake
```bash
cd projects/SlotGame

# 配置项目（Debug模式）
cmake -B build \
      -DCMAKE_BUILD_TYPE=Debug \
      -DAX_ENABLE_EXT_DOTNET=ON \
      -G "Unix Makefiles"

# 或使用Ninja（更快）
cmake -B build-ninja \
      -DCMAKE_BUILD_TYPE=Debug \
      -DAX_ENABLE_EXT_DOTNET=ON \
      -G Ninja
```

**CMake输出应包含**：
```
.NET Scripting: Using CoreCLR for desktop platform
.NET Scripting: Found nethost at /usr/share/dotnet/.../libnethost.so
.NET Scripting: Using nethost include directory: .../native
.NET Scripting: Configuration complete (CoreCLR)
```

#### 2. 编译
```bash
# Makefiles方式
cmake --build build --config Debug --target SlotGame -j$(nproc)

# Ninja方式
cmake --build build-ninja --target SlotGame
```

#### 3. 编译C#程序集
```bash
cd extensions/scripting/dotnet-bindings/managed

# 编译为Linux目标
dotnet build -c Release
```

#### 4. 部署和运行
```bash
# 复制DLL到游戏可执行文件目录
cp extensions/scripting/dotnet-bindings/managed/bin/Release/net8.0/Axmol.Scripting.dll \
   projects/SlotGame/build/bin/SlotGame/Debug/

cp extensions/scripting/dotnet-bindings/managed/bin/Release/net8.0/Axmol.Scripting.runtimeconfig.json \
   projects/SlotGame/build/bin/SlotGame/Debug/

# 运行游戏
cd projects/SlotGame/build/bin/SlotGame/Debug
./SlotGame
```

#### 5. 验证.NET初始化
检查控制台输出和DOTNET_INITIALIZED.txt文件（内容同macOS）。

### Linux特定注意事项

**1. 库搜索路径**：
Linux使用`LD_LIBRARY_PATH`，确保运行时库可以被找到：
```bash
# 检查依赖
ldd SlotGame | grep nethost

# 如果找不到nethost.so，设置环境变量
export LD_LIBRARY_PATH=/usr/share/dotnet/shared/Microsoft.NETCore.App/8.0.x:$LD_LIBRARY_PATH

# 或使用rpath（推荐）
cmake -B build -DAX_ENABLE_EXT_DOTNET=ON \
      -DCMAKE_BUILD_RPATH_USE_ORIGIN=ON
```

**2. 权限问题**：
```bash
# 确保可执行权限
chmod +x SlotGame

# SELinux可能阻止加载动态库（Fedora/RHEL）
sudo setsebool -P allow_execstack 1
```

**3. 调试工具**：
```bash
# 使用GDB调试
gdb ./SlotGame
(gdb) run

# 使用Valgrind检查内存泄漏
valgrind --leak-check=full ./SlotGame
```

---

## 故障排除

### 问题1：nethost库找不到

**症状**：
```
.NET Scripting: nethost library not found. Please install .NET SDK 8.0+
```

**解决方案（macOS）**：
```bash
# 设置DOTNET_ROOT环境变量
export DOTNET_ROOT=/usr/local/share/dotnet

# 或安装.NET SDK
brew install dotnet-sdk

# 重新运行CMake
cmake -B build -DAX_ENABLE_EXT_DOTNET=ON
```

**解决方案（Linux）**：
```bash
# 检查.NET安装
dotnet --list-sdks
dotnet --list-runtimes

# 设置DOTNET_ROOT
export DOTNET_ROOT=/usr/share/dotnet

# 重新安装.NET SDK
sudo apt install --reinstall dotnet-sdk-8.0

# 重新运行CMake
cmake -B build -DAX_ENABLE_EXT_DOTNET=ON
```

### 问题2：运行时加载nethost失败

**症状（macOS）**：
```
dyld: Library not loaded: libnethost.dylib
```

**解决方案**：
```bash
# 复制nethost.dylib到游戏目录
cp $(find /usr/local/share/dotnet -name libnethost.dylib | head -1) \
   projects/SlotGame/build/bin/SlotGame/Debug/

# 或设置DYLD_LIBRARY_PATH（仅开发时）
export DYLD_LIBRARY_PATH=/usr/local/share/dotnet/shared/Microsoft.NETCore.App/8.0.x
```

**症状（Linux）**：
```
error while loading shared libraries: libnethost.so: cannot open shared object file
```

**解决方案**：
```bash
# 复制libnethost.so到游戏目录
cp $(find /usr/share/dotnet -name libnethost.so | head -1) \
   projects/SlotGame/build/bin/SlotGame/Debug/

# 或添加到系统库路径
sudo ldconfig /usr/share/dotnet/shared/Microsoft.NETCore.App/8.0.x
```

### 问题3：C#程序集加载失败

**症状**：
```
Failed to load assembly: Axmol.Scripting.dll
Error code: 0x80008081
```

**解决方案**：
```bash
# 确认DLL和runtimeconfig.json都在正确位置
ls projects/SlotGame/build/bin/SlotGame/Debug/*.dll
ls projects/SlotGame/build/bin/SlotGame/Debug/*.runtimeconfig.json

# 检查DLL是否为当前平台编译
file Axmol.Scripting.dll
# 输出应该是：PE32+ executable (DLL) (console) x86-64, for MS Windows 或
#              Mono/.Net assembly

# 重新编译C#程序集
cd extensions/scripting/dotnet-bindings/managed
rm -rf bin obj
dotnet build -c Release
```

### 问题4：编译错误 - C++17要求

**症状**：
```
error: 'filesystem' is not a member of 'std'
```

**解决方案**：
```bash
# 在CMakeLists.txt中确保C++17
set_target_properties(SlotGame PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# 或在命令行指定
cmake -B build -DAX_ENABLE_EXT_DOTNET=ON -DCMAKE_CXX_STANDARD=17
```

---

## 性能和优化

### CoreCLR vs Mono性能对比

| 指标 | CoreCLR (macOS/Linux) | Mono (iOS/Android) |
|------|----------------------|-------------------|
| JIT编译 | ✅ 支持 | ❌ 仅AOT |
| 启动速度 | 较慢（JIT预热） | 快（预编译） |
| 运行性能 | 最优（动态优化） | 良好（固定优化） |
| 内存占用 | 中等 | 较小 |

### 发布优化

**Release构建**：
```bash
# C++端
cmake -B build-release \
      -DCMAKE_BUILD_TYPE=Release \
      -DAX_ENABLE_EXT_DOTNET=ON
cmake --build build-release --config Release

# C#端
cd extensions/scripting/dotnet-bindings/managed
dotnet publish -c Release -o ../../bin/Release

# 启用ReadyToRun（预编译IL）
dotnet publish -c Release -p:PublishReadyToRun=true
```

**裁剪未使用代码**：
```bash
# 在.csproj中启用
<PublishTrimmed>true</PublishTrimmed>
<TrimMode>partial</TrimMode>
```

---

## 测试清单

在macOS/Linux上完成以下测试：

- [ ] CMake配置成功，找到nethost库
- [ ] C++编译成功，链接axdotnet库
- [ ] C#程序集编译成功
- [ ] 游戏启动，.NET引擎初始化成功
- [ ] C# Initialize方法被调用
- [ ] DOTNET_INITIALIZED.txt文件创建成功
- [ ] 日志输出正确的运行时版本和平台信息
- [ ] Release构建正常工作
- [ ] 长时间运行稳定，无内存泄漏

---

## 下一步

macOS和Linux平台的.NET集成配置已完成。需要在相应系统上进行实际测试：

**macOS测试**：
1. 在Intel或Apple Silicon Mac上安装.NET SDK
2. 编译SlotGame项目
3. 验证.NET初始化和C#调用
4. 测试Release构建

**Linux测试**：
1. 在Ubuntu/Debian系统上安装.NET SDK
2. 编译SlotGame项目
3. 验证.NET初始化和C#调用
4. 测试不同发行版兼容性

---

## 参考资料

- [.NET Hosting API](https://learn.microsoft.com/en-us/dotnet/core/tutorials/netcore-hosting)
- [macOS .NET安装](https://learn.microsoft.com/en-us/dotnet/core/install/macos)
- [Linux .NET安装](https://learn.microsoft.com/en-us/dotnet/core/install/linux)
- [CoreCLR架构](https://github.com/dotnet/runtime/blob/main/docs/design/coreclr/botr/README.md)

---

**最后更新**: 2025-11-24
**版本**: 1.0 (配置完成，待测试)
