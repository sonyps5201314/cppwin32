using System;
using System.Runtime.InteropServices;

namespace Windows.Win32.Test.ImportFirst
{
    public static class Apis
    {
        [DllImport("kernel32.dll", EntryPoint = "GetCurrentProcessId")]
        public static extern uint LibraryProbe();
    }
}

namespace Windows.Win32.Test.ImportSecond
{
    public static class Apis
    {
        [DllImport("user32.dll", EntryPoint = "IsWindow")]
        public static extern int LibraryProbe(IntPtr window);
    }
}
