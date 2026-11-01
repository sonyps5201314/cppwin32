using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Windows.Win32;
using Windows.Win32.Foundation;

namespace CsWin32Test
{
    using unsafe LPRECT2 = Windows.Win32.Foundation.RECT*;
    using unsafe GVVOID = void;
    internal class Program
    {
        [DllImport("USER32.dll", CallingConvention = CallingConvention.StdCall, ExactSpelling = true, SetLastError = true)]
        public static extern int GetWindowRect([In] IntPtr hWnd, LPRECT lpRect);
        public unsafe struct LPRECT
        {
            public RECT* Value;

            // 从 RECT* 到 LPRECT 的隐式转换
            public static implicit operator LPRECT(RECT* ptr)
            {
                return new LPRECT { Value = ptr };
            }

            // 从 LPRECT 到 RECT* 的隐式转换
            public static implicit operator RECT*(LPRECT lprect)
            {
                return lprect.Value;
            }

            // 如果需要，也可以添加构造函数等
            public LPRECT(RECT* ptr)
            {
                Value = ptr;
            }
        }

        static unsafe void Main(string[] args)
        {
            GVVOID xxxx;
            var nativeType= "LPPONT";
            var name = "PONT*";
            var code = 
@$"using System;
using Windows.Win32.Foundation.Metadata;

namespace Windows.Win32.Foundation
{{
    [NativeTypedef]
    public unsafe struct {nativeType}
    {{
        public {name} Value;
    }}
}}";
            LPRECT2 lpRect2 = null;
            int s = sizeof(Windows.Win32.System.Kernel.SLIST_ENTRY);
            RECT rt;
            //using LPRECT2 = RECT*;
            GetWindowRect(PInvoke.GetDesktopWindow(), &rt);
            Console.WriteLine("Hello, World!");
        }
    }
}
