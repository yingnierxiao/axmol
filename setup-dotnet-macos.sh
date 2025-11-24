#!/bin/bash
# Axmol .NET集成 - macOS设置脚本
# 用于在Mac上应用补丁后配置.NET运行时库

set -e

echo "========================================="
echo "Axmol .NET Integration - macOS Setup"
echo "========================================="
echo ""

# 检查是否在macOS上运行
if [[ "$OSTYPE" != "darwin"* ]]; then
    echo "错误：此脚本必须在macOS上运行"
    exit 1
fi

# 检查必需工具
echo "检查必需工具..."

if ! command -v dotnet &> /dev/null; then
    echo "错误：未找到.NET SDK"
    echo "请使用以下命令安装："
    echo "  brew install dotnet-sdk"
    echo "或从官网下载: https://dotnet.microsoft.com/download"
    exit 1
fi

if ! command -v cmake &> /dev/null; then
    echo "错误：未找到CMake"
    echo "请使用以下命令安装："
    echo "  brew install cmake"
    exit 1
fi

echo "✓ .NET SDK: $(dotnet --version)"
echo "✓ CMake: $(cmake --version | head -1)"
echo ""

# 创建必要的目录
echo "创建Mono库目录..."
MONO_DIR="extensions/scripting/dotnet-bindings/mono"
mkdir -p "$MONO_DIR/lib/ios/arm64"
mkdir -p "$MONO_DIR/lib/ios-simulator/arm64"
mkdir -p "$MONO_DIR/lib/ios-simulator/x64"
mkdir -p "$MONO_DIR/include"
mkdir -p "$MONO_DIR/nuget"

echo "✓ 目录创建完成"
echo ""

# 下载iOS Mono运行时
echo "下载iOS Mono运行时库..."
cd "$MONO_DIR/nuget"

# iOS真机 arm64
if [ ! -f "mono-ios-arm64.nupkg" ]; then
    echo "  下载 iOS arm64..."
    curl -L -o mono-ios-arm64.nupkg \
        "https://api.nuget.org/v3-flatcontainer/microsoft.netcore.app.runtime.mono.ios-arm64/8.0.11/microsoft.netcore.app.runtime.mono.ios-arm64.8.0.11.nupkg"
fi

# iOS模拟器 arm64
if [ ! -f "mono-iossimulator-arm64.nupkg" ]; then
    echo "  下载 iOS Simulator arm64..."
    curl -L -o mono-iossimulator-arm64.nupkg \
        "https://api.nuget.org/v3-flatcontainer/microsoft.netcore.app.runtime.mono.iossimulator-arm64/8.0.11/microsoft.netcore.app.runtime.mono.iossimulator-arm64.8.0.11.nupkg"
fi

# iOS模拟器 x64
if [ ! -f "mono-iossimulator-x64.nupkg" ]; then
    echo "  下载 iOS Simulator x64..."
    curl -L -o mono-iossimulator-x64.nupkg \
        "https://api.nuget.org/v3-flatcontainer/microsoft.netcore.app.runtime.mono.iossimulator-x64/8.0.11/microsoft.netcore.app.runtime.mono.iossimulator-x64.8.0.11.nupkg"
fi

echo "✓ 下载完成"
echo ""

# 解压库文件
echo "解压Mono库文件..."
unzip -q -o mono-ios-arm64.nupkg -d mono-ios-arm64
unzip -q -o mono-iossimulator-arm64.nupkg -d mono-iossimulator-arm64
unzip -q -o mono-iossimulator-x64.nupkg -d mono-iossimulator-x64
echo "✓ 解压完成"
echo ""

# 复制库文件到标准位置
echo "复制库文件..."
cp mono-ios-arm64/runtimes/ios-arm64/native/*.a ../lib/ios/arm64/
cp mono-iossimulator-arm64/runtimes/iossimulator-arm64/native/*.a ../lib/ios-simulator/arm64/
cp mono-iossimulator-x64/runtimes/iossimulator-x64/native/*.a ../lib/ios-simulator/x64/

# 复制头文件
if [ ! -d "../include/mono-2.0" ]; then
    cp -r mono-ios-arm64/runtimes/ios-arm64/native/include/mono-2.0 ../include/
fi

echo "✓ 库文件复制完成"
echo ""

# 返回根目录
cd ../../../../..

# 统计文件
echo "验证库文件..."
IOS_LIBS=$(find extensions/scripting/dotnet-bindings/mono/lib/ios* -name "*.a" | wc -l)
HEADERS=$(find extensions/scripting/dotnet-bindings/mono/include -name "*.h" | wc -l)
echo "  iOS静态库: $IOS_LIBS 个"
echo "  Mono头文件: $HEADERS 个"
echo ""

# 编译C#程序集
echo "编译C#程序集..."
cd extensions/scripting/dotnet-bindings/managed
dotnet build -c Release
echo "✓ C#程序集编译完成"
echo ""
cd ../../../..

# 测试CMake配置
echo "测试CMake配置..."
mkdir -p build-test
cd build-test
cmake .. -DAX_ENABLE_EXT_DOTNET=ON -DCMAKE_BUILD_TYPE=Debug > cmake-output.txt 2>&1

if grep -q ".NET Scripting: Configuration complete" cmake-output.txt; then
    echo "✓ CMake配置成功"

    # 检查是否找到了nethost
    if grep -q "Found nethost" cmake-output.txt; then
        echo "✓ 找到nethost库"
    else
        echo "⚠ 未找到nethost库（正常，macOS使用Mono或系统.NET）"
    fi
else
    echo "❌ CMake配置失败"
    echo "详细信息："
    cat cmake-output.txt
    exit 1
fi
cd ..

echo ""
echo "========================================="
echo "✅ macOS .NET集成设置完成！"
echo "========================================="
echo ""
echo "下一步："
echo "1. 编译SlotGame项目："
echo "   cd projects/SlotGame"
echo "   cmake -B build -DCMAKE_BUILD_TYPE=Debug -DAX_ENABLE_EXT_DOTNET=ON"
echo "   cmake --build build --config Debug"
echo ""
echo "2. 运行游戏："
echo "   cd build/bin/SlotGame/Debug"
echo "   ./SlotGame"
echo ""
echo "3. 检查.NET初始化："
echo "   查看控制台输出是否包含："
echo "   [AppDelegate] .NET Engine initialized successfully"
echo ""
echo "详细文档："
echo "  - extensions/scripting/dotnet-bindings/DESKTOP_BUILD.md"
echo "  - extensions/scripting/dotnet-bindings/PLATFORM_STATUS.md"
echo "========================================="
