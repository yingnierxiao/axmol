using System;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;

namespace Axmol.Scripting
{
    /// <summary>
    /// Main entry point for Axmol .NET scripting engine
    /// This class provides C++ interop methods
    /// </summary>
    public static class EngineAPI
    {
        /// <summary>
        /// Initialize the scripting system (called from C++)
        /// </summary>
        [UnmanagedCallersOnly(EntryPoint = "axmol_dotnet_initialize")]
        public static int Initialize(IntPtr args, int size)
        {
            try
            {
                // 创建标记文件证明.NET被调用
                System.IO.File.WriteAllText("DOTNET_INITIALIZED.txt",
                    $"[Axmol.NET] Initialized at {DateTime.Now}\r\n" +
                    $"Runtime: {RuntimeInformation.FrameworkDescription}\r\n" +
                    $"Platform: {RuntimeInformation.OSDescription}\r\n");

                Console.WriteLine("[Axmol.NET] Scripting system initialized");
                Console.WriteLine($"[Axmol.NET] Runtime: {RuntimeInformation.FrameworkDescription}");
                Console.WriteLine($"[Axmol.NET] Platform: {RuntimeInformation.OSDescription}");

                // Call your game initialization here
                Game.Initialize();

                return 0; // Success
            }
            catch (Exception ex)
            {
                System.IO.File.WriteAllText("DOTNET_ERROR.txt",
                    $"[Axmol.NET] Initialization failed: {ex}");
                Console.WriteLine($"[Axmol.NET] Initialization failed: {ex}");
                return -1; // Error
            }
        }

        /// <summary>
        /// Update game logic every frame (called from C++)
        /// </summary>
        [UnmanagedCallersOnly(EntryPoint = "axmol_dotnet_update")]
        public static void Update(float deltaTime)
        {
            try
            {
                Game.Update(deltaTime);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[Axmol.NET] Update error: {ex}");
            }
        }

        /// <summary>
        /// Shutdown the scripting system (called from C++)
        /// </summary>
        [UnmanagedCallersOnly(EntryPoint = "axmol_dotnet_shutdown")]
        public static void Shutdown()
        {
            try
            {
                Console.WriteLine("[Axmol.NET] Shutting down");
                Game.Shutdown();
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[Axmol.NET] Shutdown error: {ex}");
            }
        }

        /// <summary>
        /// Log message to C++ engine (helper method)
        /// </summary>
        [DllImport("__Internal", EntryPoint = "axmol_log")]
        public static extern void Log(
            [MarshalAs(UnmanagedType.LPUTF8Str)] string message
        );
    }

    /// <summary>
    /// Your game logic class
    /// </summary>
    public static class Game
    {
        private static int _frameCount = 0;

        public static void Initialize()
        {
            Console.WriteLine("[Game] Initializing game logic");
            _frameCount = 0;
        }

        public static void Update(float deltaTime)
        {
            _frameCount++;

            // Log every 60 frames (~1 second at 60fps)
            if (_frameCount % 60 == 0)
            {
                Console.WriteLine($"[Game] Frame {_frameCount}, Delta: {deltaTime:F4}s");
            }

            // Add your game logic here
        }

        public static void Shutdown()
        {
            Console.WriteLine($"[Game] Shutdown after {_frameCount} frames");
        }
    }
}
