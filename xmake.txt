add_requires(
    "c-ares",
    "fmt",
    "zlib",
    "freetype",
    "clipper2 Clipper2_1.5.2",
    "stb",
    "pugixml",
    "libpng",
    "libjpeg-turbo 3.1.0",
    "poly2tri", -- is the library still supported?
    "simdjson v3.12.3",
    "axslcc",
    "box2d v2.4.2",
    "openssl3 3.3.2"
)

add_requires("astc-encoder", {configs = {cli = false}}) -- enable intrinsics

if is_plat("linux") then
    add_requires("fontconfig", {system = true}) -- linux only lib
    add_requires("gtk3", {system = true})
    add_requires("glfw", {configs = {wayland = false}})
    add_requires("opengl")
end

target("yasio")
    set_kind("static")
    add_files("3rdparty/yasio/yasio/*.cpp")
    add_includedirs("3rdparty/yasio/", {public = true})
    add_packages("openssl3", "c-ares")

target("convert-utf")
    set_kind("static")
    add_files("3rdparty/ConvertUTF/*.cpp")
    add_includedirs("3rdparty/ConvertUTF/", {public = true})

target("unzip")
    set_kind("static")
    add_files("3rdparty/unzip/*.c", "3rdparty/unzip/*.cpp")
    add_includedirs("3rdparty/unzip/", {public = true})
    add_packages("zlib")
    add_defines("NOUNCRYPT=1")

-- TODO: use glad from xrepo
target("glad")
    set_kind("static")
    add_files("3rdparty/glad/src/*.c")
    add_includedirs("3rdparty/glad/include", {public = true})

target("xxhash")
    set_kind("static")
    add_files("3rdparty/xxhash/*.c")
    add_includedirs("3rdparty/xxhash", {public = true})

rule("axslcc")
    set_extensions(".frag", ".vert")

    on_buildcmd_file(function(target, batchcmds, sourcefile, opt)
        -- TODO: include dirs
        -- TODO: difference between find_program vs find_tool
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
            "--include-dirs=" .. includedir -- TODO: default should be axmol/core/renderer/shaders
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

        -- add deps
        batchcmds:add_depfiles(sourcefile)
        batchcmds:set_depmtime(os.mtime(outputfile))
        batchcmds:set_depcache(target:dependfile(outputfile))
    end)
rule_end()

target("axmol")
    set_kind("static")
    add_files(
        "core/*.cpp",
        "core/base/*.cpp",
        "core/platform/*.cpp",
        "core/renderer/*.cpp",
        "core/renderer/backend/*.cpp",
        "core/2d/*.cpp",
        "core/3d/*.cpp",
        "core/math/*.cpp",
        "core/ui/*.cpp",
        "core/ui/UIEditBox/UIEditBox.cpp",
        "core/ui/UIEditBox/UIEditBoxImpl-common.cpp",
        "extensions/physics-nodes/src/physics-nodes/PhysicsSpriteBox2D.cpp",
        "core/renderer/backend/opengl/*.cpp"
    ) -- except apple

    add_files(
        "core/platform/$(plat)/*.cpp",
        "core/ui/UIEditBox/UIEditBoxImpl-$(plat).cpp"
    )
    add_includedirs("core/platform/$(plat)", {public = true})
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

    add_deps("yasio", "convert-utf", "unzip", "glad", "xxhash")
    add_packages(
        "freetype",
        "clipper2",
        "stb",
        "pugixml",
        "libpng",
        "libjpeg-turbo",
        "astc-encoder",
        "poly2tri",
        "simdjson",
        "openssl3"
    )

    if is_plat("linux") then
        add_packages("glfw", "gtk3", "opengl", "fontconfig")
        add_defines("AX_USE_GL=1", {public = true})
        add_links("GL", {public = true})
    end

    add_packages("axslcc", {host = true})

    add_packages("fmt", {public = true})
    add_packages("box2d", {public = true})
    add_links("pthread")

    add_defines(
        "AX_USE_WEBP=0", "AX_ENABLE_3D=1",
        "AX_VERSION_STR_FULL=\"2.6.0\"", -- move to axmolver.h.in
        "AX_MAX_DIRECTIONAL_LIGHT=1", "AX_MAX_POINT_LIGHT=1", "AX_MAX_SPOT_LIGHT=1",
        "AX_ENABLE_SCRIPT_BINDING=1",
        "_AX_DEBUG=1",
        -- "AX_ENABLE_PHYSICS=1",
        {public = true}
    )

    add_rules("axslcc", {includedir = "core/renderer/shaders"})
    add_files("core/**.frag", "core/**.vert")

    if is_plat("wasm") then
        add_ldflags(
            "-sUSE_GLFW=3 "..
            "-sASSERTIONS "..
            "-sMIN_WEBGL_VERSION=2 "..
            "-sGL_ENABLE_GET_PROC_ADDRESS "..
            "--use-preload-cache "..
            -- "-pthread -sPTHREAD_POOL_SIZE=4 "..
            "-sFORCE_FILESYSTEM=1 -sFETCH=1 "..
            "-lidbfs.js",
            {expand = false}
        )
        add_defines("AX_GLES_PROFILE=300", {public = true})
    end