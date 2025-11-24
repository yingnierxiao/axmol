-- /****************************************************************************
-- Axmol Engine - xmake 完整配置
-- 包含第三方库 + 引擎核心 + SlotGame 支持
-- ****************************************************************************/

add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = "$(builddir)"})

set_languages("c++20")
set_warnings("all")

-- ========== 平台配置 ==========
if is_plat("windows") then
    -- 强制使用 MSVC 工具链
    set_toolchains("msvc")
    add_defines("WIN32", "_WINDOWS", "UNICODE", "_UNICODE")
    -- UTF-8 编码（fmt 库要求）
    add_cxflags("/utf-8")
    -- 禁用常见警告
    add_cxflags("/wd4819", "/wd4068", "/wd4244", "/wd4018", "/wd4013", "/wd4129", "/wd4267")
end

-- ========== 确认有源码的库 ==========

-- UTF 编码转换
target("convert-utf")
    set_kind("static")
    add_files("3rdparty/ConvertUTF/*.cpp")
    add_includedirs("3rdparty/ConvertUTF/", {public = true})
target_end()

-- xxHash 快速哈希
target("xxhash")
    set_kind("static")
    add_files("3rdparty/xxhash/xxhash.c")
    add_includedirs("3rdparty/xxhash", {public = true})
target_end()

-- fastlz 压缩
target("fastlz")
    set_kind("static")
    add_files("3rdparty/fastlz/fastlz.c")
    add_includedirs("3rdparty/fastlz", {public = true})
target_end()

-- llhttp HTTP 解析器
target("llhttp")
    set_kind("static")
    add_files("3rdparty/llhttp/src/*.c")
    add_includedirs("3rdparty/llhttp/include", {public = true})
target_end()

-- websocket-parser
if not is_plat("wasm") then
    target("websocket-parser")
        set_kind("static")
        add_files("3rdparty/websocket-parser/*.c")
        add_includedirs("3rdparty/websocket-parser", {public = true})
    target_end()
end

-- clipper2 多边形裁剪
target("clipper2")
    set_kind("static")
    add_files("3rdparty/clipper2/src/clipper.*.cpp")
    add_includedirs("3rdparty/clipper2/include", {public = true})
target_end()

-- poly2tri 三角剖分
target("poly2tri")
    set_kind("static")
    add_files("3rdparty/poly2tri/**/*.cc")
    add_includedirs("3rdparty/poly2tri", {public = true})
target_end()

-- pugixml XML 解析器
target("pugixml")
    set_kind("static")
    add_files("3rdparty/pugixml/*.cpp")
    add_includedirs("3rdparty/pugixml", {public = true})
target_end()

-- simdjson JSON 解析器
target("simdjson")
    set_kind("static")
    add_files("3rdparty/simdjson/simdjson.cpp")
    add_includedirs("3rdparty/simdjson", {public = true})

    if is_mode("release") then
        set_optimize("fastest")
    end
target_end()

-- box2d 2D 物理引擎
target("box2d")
    set_kind("static")
    add_files("3rdparty/box2d/src/**/*.cpp")
    add_includedirs(
        "3rdparty/box2d/include",
        "3rdparty/box2d/src",
        {public = true}
    )
target_end()

-- fmt 格式化库（使用 header-only 模式）
target("fmt")
    set_kind("headeronly")
    add_includedirs("3rdparty/fmt/include", {public = true})
    add_defines("FMT_HEADER_ONLY=1", {public = true})
target_end()

-- yasio 网络库（不链接 SSL）
target("yasio")
    set_kind("static")
    add_files("3rdparty/yasio/yasio/*.cpp")
    add_includedirs("3rdparty/yasio/", {public = true})

    if is_plat("windows") then
        add_syslinks("ws2_32", "wsock32")
    end

    add_defines("YASIO_DISABLE_SSL=1")
target_end()

-- unzip 解压库（依赖本地 zlib）
target("unzip")
    set_kind("static")
    add_files("3rdparty/unzip/*.c", "3rdparty/unzip/*.cpp")
    add_includedirs("3rdparty/unzip/", {public = true})
    add_deps("zlib")
    add_defines("NOUNCRYPT=1")
target_end()

-- glad OpenGL 加载器
target("glad")
    set_kind("static")
    add_files("3rdparty/glad/src/*.c")
    add_includedirs("3rdparty/glad/include", {public = true})
target_end()

-- zlib 压缩库（预编译）
target("zlib")
    set_kind("phony")
    add_includedirs("3rdparty/zlib/_x/include", {public = true})
    on_load(function(target)
        if is_plat("windows") then
            target:add("linkdirs", path.join(os.projectdir(), "3rdparty/zlib/_x/lib"))
            target:add("links", "zlib")
            target:add("defines", "ZLIB_DLL=1", {public = true})
        end
    end)
target_end()

-- openssl 加密库（预编译）
target("openssl")
    set_kind("phony")
    add_includedirs("3rdparty/openssl/_x/include", {public = true})
    on_load(function(target)
        if is_plat("windows") then
            target:add("linkdirs", path.join(os.projectdir(), "3rdparty/openssl/_x/lib"))
            target:add("links", "libcrypto", "libssl")
        end
    end)
target_end()

-- GLFW 窗口库
target("glfw")
    set_kind("static")
    add_includedirs("3rdparty/glfw/include", {public = true})

    if is_plat("windows") then
        add_files(
            "3rdparty/glfw/src/context.c",
            "3rdparty/glfw/src/init.c",
            "3rdparty/glfw/src/input.c",
            "3rdparty/glfw/src/monitor.c",
            "3rdparty/glfw/src/platform.c",
            "3rdparty/glfw/src/vulkan.c",
            "3rdparty/glfw/src/window.c",
            "3rdparty/glfw/src/egl_context.c",
            "3rdparty/glfw/src/osmesa_context.c",
            "3rdparty/glfw/src/null_init.c",
            "3rdparty/glfw/src/null_monitor.c",
            "3rdparty/glfw/src/null_window.c",
            "3rdparty/glfw/src/null_joystick.c",
            "3rdparty/glfw/src/win32_init.c",
            "3rdparty/glfw/src/win32_joystick.c",
            "3rdparty/glfw/src/win32_monitor.c",
            "3rdparty/glfw/src/win32_time.c",
            "3rdparty/glfw/src/win32_thread.c",
            "3rdparty/glfw/src/win32_window.c",
            "3rdparty/glfw/src/win32_module.c",
            "3rdparty/glfw/src/wgl_context.c"
        )
        add_defines("_GLFW_WIN32")
    end
target_end()

-- libpng 图像库
target("png")
    set_kind("static")
    add_files("3rdparty/png/*.c")
    add_includedirs("3rdparty/png", {public = true})
    add_deps("zlib")
    add_defines("_CRT_SECURE_NO_WARNINGS")
target_end()

-- jpeg-turbo 图像库
-- jpeg-turbo 图像库（预编译）
target("jpeg-turbo")
    set_kind("phony")
    add_includedirs("3rdparty/jpeg-turbo/_x/include", {public = true})
    on_load(function(target)
        if is_plat("windows") then
            target:add("linkdirs", path.join(os.projectdir(), "3rdparty/jpeg-turbo/_x/lib"))
            target:add("links", "turbojpeg-static", "jpeg-static")
        end
    end)
target_end()

-- stb (header-only)
target("stb")
    set_kind("headeronly")
    add_includedirs("3rdparty/stb", {public = true})
target_end()

-- freetype 字体库（完全独立，不依赖其他库）
target("freetype")
    set_kind("static")

    -- 只编译核心必需的模块
    add_files(
        -- Base 模块（核心）
        "3rdparty/freetype/src/base/ftbase.c",
        "3rdparty/freetype/src/base/ftbitmap.c",
        "3rdparty/freetype/src/base/ftbbox.c",
        "3rdparty/freetype/src/base/ftglyph.c",
        "3rdparty/freetype/src/base/ftinit.c",
        "3rdparty/freetype/src/base/ftsystem.c",
        "3rdparty/freetype/src/base/ftdebug.c",

        -- AutoFit 模块（自动hinting）
        "3rdparty/freetype/src/autofit/autofit.c",

        -- Rasterizers（光栅化）
        "3rdparty/freetype/src/raster/raster.c",
        "3rdparty/freetype/src/smooth/smooth.c",

        -- Font Drivers（字体驱动，只选择常用的）
        "3rdparty/freetype/src/truetype/truetype.c", -- TrueType
        "3rdparty/freetype/src/cff/cff.c",           -- CFF/OpenType
        "3rdparty/freetype/src/sfnt/sfnt.c",         -- SFNT (TrueType/OpenType container)

        -- Auxiliary modules（辅助模块）
        "3rdparty/freetype/src/psaux/psaux.c",
        "3rdparty/freetype/src/psnames/psnames.c"
    )

    add_includedirs("3rdparty/freetype/include", {public = true})

    -- 关键配置：禁用所有压缩支持，完全独立编译
    add_defines(
        "FT2_BUILD_LIBRARY",        -- 构建库
        "FT_CONFIG_OPTION_NO_GZIP", -- 禁用 gzip
        "FT_CONFIG_OPTION_NO_BZIP2",  -- 禁用 bzip2
        "FT_CONFIG_OPTION_NO_LZW"   -- 禁用 lzw
    )

    if is_plat("windows") then
        add_defines("_CRT_SECURE_NO_WARNINGS")
    end
target_end()

-- ========== axslcc shader 编译规则 ==========
rule("axslcc")
    set_extensions(".frag", ".vert")

    on_buildcmd_file(function(target, batchcmds, sourcefile, opt)
        import("lib.detect.find_program")
        local axslcc = assert(find_program("axslcc"), "axslcc not found!")
        local includedir = target:extraconf("rules", "axslcc", "includedir")
        if not path.is_absolute(includedir) then
            includedir = path.join(target:scriptdir(), includedir)
        end
        local flags = {
            "--lang=gl" .. (is_plat("wasm") and "es" or "sl"),
            "--profile=3" .. (is_plat("wasm") and "00" or "30"),
            "--automap",
            "--no-suffix",
            "--err-format=msvc",
            "--defines=MAX_DIRECTIONAL_LIGHT_NUM=1,MAX_POINT_LIGHT_NUM=1,MAX_SPOT_LIGHT_NUM=1",
            "--include-dirs=" .. includedir
        }
        local extension = path.extension(sourcefile)
        local filename = path.basename(sourcefile)

        batchcmds:show_progress(opt.progress, "${color.build.object}compiling shader via axslcc %s", sourcefile)
        if extension == ".frag" then
            table.append(flags, "--frag=" .. path.absolute(sourcefile))
            filename = filename .. "_fs"
        elseif extension == ".vert" then
            table.append(flags, "--vert=" .. path.absolute(sourcefile))
            filename = filename .. "_vs"
        else
            print("extension " .. extension .. "not supported, file: " .. sourcefile)
        end

        os.mkdir(target:targetdir("axslc"))
        local axslc_dir = path.join(target:targetdir(), "axslc")
        os.mkdir(axslc_dir)
        local outputfile = path.absolute(path.join(axslc_dir, filename))
        table.append(flags, "--output=" .. outputfile)
        batchcmds:vrunv(axslcc, flags)

        batchcmds:add_depfiles(sourcefile)
        batchcmds:set_depmtime(os.mtime(outputfile))
        batchcmds:set_depcache(target:dependfile(outputfile))
    end)
rule_end()

-- ========== axmol 引擎核心 ==========
target("axmol")
    set_kind("static")

    -- 强制包含必要的头文件（解决未定义问题）
    add_forceincludes("base/Macros.h", "base/Types.h", "renderer/backend/DriverBase.h", "base/UTF8.h", "base/Scheduler.h", "platform/FileStream.h", "platform/FileUtils.h", "platform/Common.h", "renderer/backend/VertexLayout.h", "renderer/backend/ProgramManager.h", "math/MathUtil.h")

    add_files(
        "core/*.cpp",
        "core/base/*.cpp",
        "core/platform/*.cpp",
        "core/renderer/*.cpp",
        "core/renderer/backend/*.cpp",          -- backend 基础类
        "core/renderer/backend/opengl/*.cpp",   -- OpenGL 实现
        "core/2d/*.cpp",
        "core/3d/*.cpp",
        "core/math/*.cpp",
        "core/ui/*.cpp",
        "core/ui/UIEditBox/UIEditBox.cpp",
        "core/ui/UIEditBox/UIEditBoxImpl-common.cpp",
        "extensions/physics-nodes/src/physics-nodes/PhysicsSpriteBox2D.cpp"
    )

    -- Windows 平台文件（目录名为 win32，不是 windows）
    if is_plat("windows") then
        add_files("core/platform/win32/*.cpp")
        add_files("core/ui/UIEditBox/UIEditBoxImpl-win32.cpp")
        add_includedirs("core/platform/win32", {public = true})
    else
        add_files(
            "core/platform/$(plat)/*.cpp",
            "core/ui/UIEditBox/UIEditBoxImpl-$(plat).cpp"
        )
        add_includedirs("core/platform/$(plat)", {public = true})
    end
    remove_files(
        "core/base/Controller-android.cpp",
        "core/base/Controller-linux-win32.cpp"
    )

    if is_plat("linux") then
        add_files("core/base/Controller-linux-win32.cpp")
    end

    add_includedirs(
        "core",
        "3rdparty/robin-map/include",
        "3rdparty/",
        "extensions/physics-nodes/src/",
        ".",
        "core/base",
        "core/platform",
        {public = true}
    )

    -- 本地编译的第三方库
    add_deps(
        "yasio", "convert-utf", "unzip", "glad", "xxhash",
        "zlib", "openssl", "glfw", "png", "jpeg-turbo", "stb"
    )

    -- 本地编译的库（作为 public 依赖）
    add_deps("freetype", "clipper2", "poly2tri", "simdjson", "box2d", "fmt", {public = true})

    if is_plat("linux") then
        -- Linux 系统库
        add_defines("AX_USE_GL=1", {public = true})
        add_links("GL", {public = true})
    end

    add_links("pthread")

    add_defines(
        "AX_USE_WEBP=0", "AX_ENABLE_3D=1",
        "AX_VERSION_STR_FULL=\"2.6.0\"",
        "AX_MAX_DIRECTIONAL_LIGHT=1", "AX_MAX_POINT_LIGHT=1", "AX_MAX_SPOT_LIGHT=1",
        "AX_ENABLE_SCRIPT_BINDING=1",
        "_AX_DEBUG=1",
        {public = true}
    )

    -- Shader 编译（仅当 axslcc 工具可用时）
    -- add_rules("axslcc", {includedir = "core/renderer/shaders"})
    -- add_files("core/**.frag", "core/**.vert")

    if is_plat("wasm") then
        add_ldflags(
            "-sUSE_GLFW=3 "..
            "-sASSERTIONS "..
            "-sMIN_WEBGL_VERSION=2 "..
            "-sGL_ENABLE_GET_PROC_ADDRESS "..
            "--use-preload-cache "..
            "-sFORCE_FILESYSTEM=1 -sFETCH=1 "..
            "-lidbfs.js",
            {expand = false}
        )
        add_defines("AX_GLES_PROFILE=300", {public = true})
    end
target_end()

-- ========== 默认编译目标 ==========
target("test_all")
    set_kind("phony")
    set_default(true)

    add_deps(
        -- 本地编译的第三方库
        "convert-utf",
        "xxhash",
        "fastlz",
        "llhttp",
        "clipper2",
        "poly2tri",
        "pugixml",
        "simdjson",
        "box2d",
        "fmt",
        "yasio",
        "freetype",
        "zlib",
        "unzip",
        "glad",
        "png",
        "jpeg-turbo",
        "stb",
        -- 引擎核心
        "axmol"
    )

    if not is_plat("wasm") then
        add_deps("websocket-parser")
    end

    after_build(function(target)
        print("")
        cprint("${bright green}========================================")
        cprint("  ✅ Axmol Engine 编译成功！")
        cprint("========================================${clear}")
        print("")
        cprint("${cyan}第三方库 (18个):${clear}")
        cprint("  • convert-utf     - UTF 编码转换")
        cprint("  • xxhash          - 快速哈希")
        cprint("  • fastlz          - 压缩库")
        cprint("  • llhttp          - HTTP 解析")
        cprint("  • websocket-parser- WebSocket 解析")
        cprint("  • clipper2        - 多边形裁剪")
        cprint("  • poly2tri        - 三角剖分")
        cprint("  • pugixml         - XML 解析")
        cprint("  • simdjson        - JSON 解析")
        cprint("  • box2d           - 2D 物理引擎")
        cprint("  • fmt             - 格式化库")
        cprint("  • yasio           - 网络库")
        cprint("  • ${bright}zlib${clear}            - 压缩库 ${green}✓${clear}")
        cprint("  • ${bright}freetype${clear}        - 字体库 ${green}✓${clear}")
        cprint("  • ${bright}unzip${clear}           - ZIP 解压 ${green}✓${clear}")
        cprint("  • ${bright}glad${clear}            - OpenGL 加载器 ${green}✓${clear}")
        cprint("  • ${bright}png${clear}             - PNG 图像 ${green}✓${clear}")
        cprint("  • ${bright}jpeg-turbo${clear}      - JPEG 图像 ${green}✓${clear}")
        cprint("  • ${bright}stb${clear}             - 图像工具 ${green}✓${clear}")
        print("")
        cprint("${cyan}引擎核心:${clear}")
        cprint("  • ${bright green}axmol${clear}           - 引擎核心库 ${green}✓${clear}")
        print("")
        cprint("${yellow}提示:${clear} 现在可以编译 SlotGame 项目了！")
        cprint("${dim}  cd projects/SlotGame && xmake${clear}")
        print("")
    end)
target_end()

-- ========== 编译进度提示 ==========
after_build(function(target)
    if target:name() ~= "test_all" then
        cprint("${dim}[${clear}${green}✓${clear}${dim}]${clear} %s", target:name())
    end
end)

print("🔧 完整配置加载完成 - 第三方库 + 引擎核心 + SlotGame 支持")
