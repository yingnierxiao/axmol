#[[****************************************************************************
 Copyright (c) 2025 Axmol Engine contributors

 https://axmol.dev/

 CMake configuration for .NET scripting bindings
 Supports:
 - Desktop platforms (Windows/Linux/macOS) using CoreCLR
 - Mobile platforms (iOS/Android) using Mono
****************************************************************************]]

set(ax_root ${_AX_ROOT})

# Common source files
set(dotnet_bindings_common_headers
    common/ScriptEngine.h
    common/ScriptTypes.h
)

set(dotnet_bindings_common_files
    common/ScriptEngine.cpp
)

# Platform-specific source files
if(WINDOWS OR LINUX OR (MACOSX AND NOT IOS))
    # Desktop platforms: use CoreCLR
    message(STATUS ".NET Scripting: Using CoreCLR for desktop platform")

    set(dotnet_bindings_platform_headers
        coreclr/CoreCLREngine.h
    )
    set(dotnet_bindings_platform_files
        coreclr/CoreCLREngine.cpp
    )

    set(DOTNET_RUNTIME_TYPE "CoreCLR")

elseif(IOS OR ANDROID)
    # Mobile platforms: use Mono
    message(STATUS ".NET Scripting: Using Mono for mobile platform")

    set(dotnet_bindings_platform_headers
        mono/MonoEngine.h
    )
    set(dotnet_bindings_platform_files
        mono/MonoEngine.cpp
    )

    set(DOTNET_RUNTIME_TYPE "Mono")

else()
    message(FATAL_ERROR ".NET Scripting: Unsupported platform")
endif()

# Combine all source files
set(dotnet_bindings_files
    ${dotnet_bindings_common_files}
    ${dotnet_bindings_platform_files}
)

set(dotnet_bindings_headers
    ${dotnet_bindings_common_headers}
    ${dotnet_bindings_platform_headers}
)

list(APPEND dotnet_bindings_files ${dotnet_bindings_headers})

# Create static library
set(_AX_DOTNET_LIB axdotnet CACHE INTERNAL "The axmol .NET bindings lib name")

add_library(${_AX_DOTNET_LIB} STATIC ${dotnet_bindings_files})

# Platform-specific configuration
if(DOTNET_RUNTIME_TYPE STREQUAL "CoreCLR")
    # CoreCLR: Link nethost library
    find_package(nethost QUIET)

    if(nethost_FOUND)
        target_link_libraries(${_AX_DOTNET_LIB} nethost)
        message(STATUS ".NET Scripting: Found nethost via find_package")
    else()
        # Manual nethost configuration
        if(WIN32)
            # Windows: nethost.lib
            find_library(NETHOST_LIB nethost
                PATHS
                    "$ENV{DOTNET_ROOT}/packs/Microsoft.NETCore.App.Host.win-x64/*/runtimes/win-x64/native"
                    "C:/Program Files/dotnet/packs/Microsoft.NETCore.App.Host.win-x64/*/runtimes/win-x64/native"
                NO_DEFAULT_PATH
            )
        elseif(APPLE)
            # macOS: libnethost.dylib
            find_library(NETHOST_LIB nethost
                PATHS
                    "$ENV{DOTNET_ROOT}/packs/Microsoft.NETCore.App.Host.osx-x64/*/runtimes/osx-x64/native"
                    "/usr/local/share/dotnet/packs/Microsoft.NETCore.App.Host.osx-x64/*/runtimes/osx-x64/native"
                NO_DEFAULT_PATH
            )
        elseif(LINUX)
            # Linux: libnethost.so
            find_library(NETHOST_LIB nethost
                PATHS
                    "$ENV{DOTNET_ROOT}/packs/Microsoft.NETCore.App.Host.linux-x64/*/runtimes/linux-x64/native"
                    "/usr/share/dotnet/packs/Microsoft.NETCore.App.Host.linux-x64/*/runtimes/linux-x64/native"
                NO_DEFAULT_PATH
            )
        endif()

        if(NETHOST_LIB)
            target_link_libraries(${_AX_DOTNET_LIB} ${NETHOST_LIB})
            message(STATUS ".NET Scripting: Found nethost at ${NETHOST_LIB}")

            # Extract nethost include directory from library path
            get_filename_component(NETHOST_DIR ${NETHOST_LIB} DIRECTORY)
            target_include_directories(${_AX_DOTNET_LIB} PRIVATE ${NETHOST_DIR})
            message(STATUS ".NET Scripting: Using nethost include directory: ${NETHOST_DIR}")
        else()
            message(WARNING ".NET Scripting: nethost library not found. Please install .NET SDK 8.0+")
            message(WARNING "  Set DOTNET_ROOT environment variable or install .NET from https://dotnet.microsoft.com/")
        endif()
    endif()

elseif(DOTNET_RUNTIME_TYPE STREQUAL "Mono")
    # Mono: Link mono libraries
    if(ANDROID)
        # Android: Use bundled Mono libraries
        message(STATUS ".NET Scripting: Configuring Mono for Android (${ANDROID_ABI})")

        set(MONO_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/mono/lib/android/${ANDROID_ABI}")
        set(MONO_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/mono/include")

        # Find libmonosgen-2.0.so
        find_library(MONO_LIB monosgen-2.0
            PATHS ${MONO_LIB_DIR}
            NO_DEFAULT_PATH
        )

        if(MONO_LIB)
            target_link_libraries(${_AX_DOTNET_LIB} ${MONO_LIB})
            target_include_directories(${_AX_DOTNET_LIB} PRIVATE ${MONO_INCLUDE_DIR})
            message(STATUS ".NET Scripting: Found Mono for Android: ${MONO_LIB}")
            message(STATUS ".NET Scripting: Mono include dir: ${MONO_INCLUDE_DIR}")
        else()
            message(FATAL_ERROR ".NET Scripting: Mono library not found for Android at ${MONO_LIB_DIR}")
            message(FATAL_ERROR "  Please follow instructions in ANDROID_BUILD.md to install Mono runtime")
        endif()

    elseif(IOS)
        # iOS: Use bundled Mono static libraries
        message(STATUS ".NET Scripting: Configuring Mono for iOS")
        target_compile_definitions(${_AX_DOTNET_LIB} PRIVATE IOS_MONO_AOT_ONLY)

        # Determine iOS architecture
        if(CMAKE_OSX_SYSROOT MATCHES "iPhoneSimulator")
            # iOS Simulator
            if(CMAKE_OSX_ARCHITECTURES MATCHES "arm64")
                set(IOS_ARCH "ios-simulator/arm64")
                message(STATUS ".NET Scripting: iOS Simulator ARM64")
            else()
                set(IOS_ARCH "ios-simulator/x64")
                message(STATUS ".NET Scripting: iOS Simulator x64")
            endif()
        else()
            # iOS Device
            set(IOS_ARCH "ios/arm64")
            message(STATUS ".NET Scripting: iOS Device ARM64")
        endif()

        set(MONO_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/mono/lib/${IOS_ARCH}")
        set(MONO_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/mono/include")

        # Find libmonosgen-2.0.a
        find_library(MONO_LIB monosgen-2.0
            PATHS ${MONO_LIB_DIR}
            NO_DEFAULT_PATH
        )

        if(MONO_LIB)
            # Link main Mono library
            target_link_libraries(${_AX_DOTNET_LIB} ${MONO_LIB})

            # Link all required Mono component libraries
            file(GLOB MONO_COMPONENT_LIBS "${MONO_LIB_DIR}/libmono-component-*.a")
            foreach(lib ${MONO_COMPONENT_LIBS})
                target_link_libraries(${_AX_DOTNET_LIB} ${lib})
            endforeach()

            # Link System native libraries
            file(GLOB SYSTEM_NATIVE_LIBS "${MONO_LIB_DIR}/libSystem.*.a")
            foreach(lib ${SYSTEM_NATIVE_LIBS})
                target_link_libraries(${_AX_DOTNET_LIB} ${lib})
            endforeach()

            # Link ICU libraries (iOS specific)
            file(GLOB ICU_LIBS "${MONO_LIB_DIR}/libicu*.a")
            foreach(lib ${ICU_LIBS})
                target_link_libraries(${_AX_DOTNET_LIB} ${lib})
            endforeach()

            target_include_directories(${_AX_DOTNET_LIB} PRIVATE ${MONO_INCLUDE_DIR})
            message(STATUS ".NET Scripting: Found Mono for iOS: ${MONO_LIB}")
            message(STATUS ".NET Scripting: Mono include dir: ${MONO_INCLUDE_DIR}")
        else()
            message(FATAL_ERROR ".NET Scripting: Mono library not found for iOS at ${MONO_LIB_DIR}")
            message(FATAL_ERROR "  Please run the iOS Mono setup script to download runtime libraries")
        endif()

    else()
        # Other platforms: Try pkg-config
        find_package(PkgConfig)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(MONO mono-2)
            if(MONO_FOUND)
                target_include_directories(${_AX_DOTNET_LIB} PRIVATE ${MONO_INCLUDE_DIRS})
                target_link_libraries(${_AX_DOTNET_LIB} ${MONO_LIBRARIES})
                message(STATUS ".NET Scripting: Found Mono via pkg-config")
            else()
                message(WARNING ".NET Scripting: Mono not found via pkg-config")
            endif()
        else()
            message(WARNING ".NET Scripting: pkg-config not found, Mono detection may fail")
        endif()
    endif()
endif()

# Setup extension configuration (similar to axlua)
setup_ax_extension_config(${_AX_DOTNET_LIB} DNTLINK)

# Include directories
target_include_directories(${_AX_DOTNET_LIB}
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/common
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/coreclr
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/mono
)

# Compiler definitions
target_compile_definitions(${_AX_DOTNET_LIB}
    PRIVATE AX_DOTNET_RUNTIME_${DOTNET_RUNTIME_TYPE}
)

# Set target properties
set_target_properties(${_AX_DOTNET_LIB}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    FOLDER "Extensions"
)

# Mark code files for IDE
if(XCODE OR VS)
    ax_mark_code_files("${_AX_DOTNET_LIB}")
endif()

message(STATUS ".NET Scripting: Configuration complete (${DOTNET_RUNTIME_TYPE})")
