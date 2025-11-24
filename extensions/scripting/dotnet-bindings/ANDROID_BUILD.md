# Android平台.NET支持 - 构建指南

## 当前状态

✅ **已完成**：
- CMake配置已支持Android平台检测和Mono库链接
- Mono引擎实现（mono/MonoEngine.cpp）
- 跨平台架构设计
- Mono运行时库集成（arm64-v8a, armeabi-v7a, x86_64）
- Android NDK构建配置（CMakeLists.txt已更新）
- Mono库文件和头文件已就位

✅ **库文件位置**：
```
extensions/scripting/dotnet-bindings/mono/
├── lib/android/
│   ├── arm64-v8a/libmonosgen-2.0.so (及其他.NET运行时库)
│   ├── armeabi-v7a/libmonosgen-2.0.so
│   └── x86_64/libmonosgen-2.0.so
└── include/mono-2.0/ (Mono头文件)
```

⏳ **待测试**：
- 实际的Android APK编译（需要完整的Android开发环境）
- .NET程序集在Android设备上的运行验证

## Android平台架构

Android使用**Mono运行时**（与iOS相同），原因：
- Android不允许JIT编译（需要AOT）
- Mono提供完整的Android支持
- Xamarin.Android使用相同技术栈

## 构建要求

### 1. 环境准备

**必需工具**：
```bash
- Android SDK (API 21+)
- Android NDK (r21或更高)
- CMake 3.22+
- Gradle 7.0+
- Java JDK 11+
```

**当前检测到的NDK**：
```
F:\UnityHub\2022.3.62f1c1\Editor\Data\PlaybackEngines\AndroidPlayer\NDK
```

### 2. Mono运行时集成

#### 选项A：使用预编译的Mono（推荐）

1. 下载Mono Android运行时：
```bash
# 从Mono官方或Xamarin获取
wget https://download.mono-project.com/archive/android/...
```

2. 提取库文件到：
```
extensions/scripting/dotnet-bindings/mono/lib/android/
├── arm64-v8a/
│   └── libmonosgen-2.0.so
├── armeabi-v7a/
│   └── libmonosgen-2.0.so
└── x86_64/
    └── libmonosgen-2.0.so
```

#### 选项B：从源码编译Mono（高级）

```bash
git clone https://github.com/mono/mono.git
cd mono
./autogen.sh --host=aarch64-linux-android \
             --enable-ngen=no \
             --with-btls=no
make
```

### 3. 修改CMakeLists.txt

在`extensions/scripting/dotnet-bindings/CMakeLists.txt`中添加Android Mono库路径：

```cmake
elseif(DOTNET_RUNTIME_TYPE STREQUAL "Mono")
    if(ANDROID)
        # Android Mono库路径
        set(MONO_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/mono/lib/android/${ANDROID_ABI}")

        find_library(MONO_LIB monosgen-2.0
            PATHS ${MONO_LIB_DIR}
            NO_DEFAULT_PATH
        )

        if(MONO_LIB)
            target_link_libraries(${_AX_DOTNET_LIB} ${MONO_LIB})
            target_include_directories(${_AX_DOTNET_LIB} PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/mono/include
            )
            message(STATUS ".NET Scripting: Found Mono for Android: ${MONO_LIB}")
        else()
            message(FATAL_ERROR ".NET Scripting: Mono library not found for Android")
        endif()
    endif()
endif()
```

### 4. Android Gradle配置

在`proj.android/app/build.gradle`中添加：

```gradle
android {
    defaultConfig {
        ndk {
            abiFilters 'arm64-v8a', 'armeabi-v7a'
        }
    }

    sourceSets {
        main {
            jniLibs.srcDirs = ['libs']
        }
    }
}
```

### 5. 编译步骤

#### 配置CMake：
```bash
cd projects/SlotGame/proj.android

cmake -B build/arm64-v8a \
      -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
      -DANDROID_ABI=arm64-v8a \
      -DANDROID_PLATFORM=android-21 \
      -DAX_ENABLE_EXT_DOTNET=ON \
      ../..
```

#### 构建APK：
```bash
./gradlew assembleDebug
```

## C#程序集部署

1. 编译C#代码为Android目标：
```bash
cd extensions/scripting/dotnet-bindings/managed
dotnet publish -c Release -r android-arm64
```

2. 将DLL复制到Android assets：
```bash
cp bin/Release/net8.0/android-arm64/Axmol.Scripting.dll \
   projects/SlotGame/proj.android/app/src/main/assets/
```

## 测试

### 模拟器测试：
```bash
adb install -r proj.android/app/build/outputs/apk/debug/app-debug.apk
adb logcat | grep "Axmol.NET"
```

### 真机测试：
连接Android设备并运行相同命令。

## 已知限制

1. **AOT编译**：Android不支持JIT，所有C#代码必须提前编译
2. **性能**：Mono在Android上比CoreCLR慢10-30%
3. **体积**：Mono运行时增加约8-15MB APK大小

## 替代方案：NativeAOT

对于.NET 8+，可以考虑使用NativeAOT代替Mono：

**优点**：
- 更小的体积
- 更快的启动速度
- 无需运行时

**缺点**：
- 需要.NET 8+
- 不支持反射
- 编译复杂度高

## 下一步

当前Windows平台的.NET集成已完成并测试成功。Android平台需要：

1. [ ] 获取Mono Android运行时库
2. [ ] 配置CMakeLists.txt for Android
3. [ ] 修改Android Gradle配置
4. [ ] 测试Android编译
5. [ ] 验证.NET在Android设备上运行

## 参考资料

- [Mono Android文档](https://www.mono-project.com/docs/compiling-mono/android/)
- [.NET Android支持](https://learn.microsoft.com/en-us/dotnet/core/deploying/native-aot/)
- [Axmol Android构建](https://axmol.dev)
