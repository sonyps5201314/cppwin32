using Windows.Win32.Foundation.Metadata;
using System;
using System.Runtime.InteropServices;

[assembly: NativeConditionalAlias("defined(UNICODE)", "macro", "CHOICE", "CHOICE_W", "_CHOICE_W")]
[assembly: NativeConditionalAlias("!defined(UNICODE)", "macro", "CHOICE", "CHOICE_A", "_CHOICE_A")]
[assembly: NativeConditionalAlias("defined(UNICODE)", "macro", "PCHOICE", "PCHOICE_W", "_CHOICE_W")]
[assembly: NativeConditionalAlias("!defined(UNICODE)", "macro", "PCHOICE", "PCHOICE_A", "_CHOICE_A")]
[assembly: NativeConditionalAlias("defined(UNICODE)", "typedef", "CHOICE_TYPE", "CHOICE_W", "_CHOICE_W")]
[assembly: NativeConditionalAlias("!defined(UNICODE)", "typedef", "CHOICE_TYPE", "CHOICE_A", "_CHOICE_A")]

[assembly: NativeConditionalAlias("defined(UNICODE)", "macro", "LEGACY", "LEGACYW", "_LEGACYW")]
[assembly: NativeConditionalAlias("defined(UNICODE)", "macro", "PLEGACY", "PLEGACYW", "_LEGACYW")]
[assembly: NativeConditionalAlias("defined(UNICODE)", "macro", "QLEGACY", "QLEGACYW", "_LEGACYW")]
[assembly: NativeAlias("QLEGACY", "LEGACY *", "struct _LEGACY *")]
[assembly: NativeAlias("QLEGACYW", "LEGACYW *", "struct _LEGACYW *")]
[assembly: NativeConditionalAlias("defined(UNICODE)", "macro", "PUNION", "PUNIONW", "_UNIONW")]
[assembly: NativeAlias("PUNION", "union _UNION *", "_UNION *")]
[assembly: NativeAlias("PUNIONW", "union _UNIONW *", "_UNIONW *")]
[assembly: NativeAlias("PALIAS", "PZRECORD", "struct _ZRECORD *")]
[assembly: NativeConditionalAlias("defined(UNICODE)", "typedef", "SELECTED_ALIAS", "PALIAS", "_ZRECORD")]

namespace Windows.Win32.Test.UnicodeAliases
{
    [NativeRecord("_CHOICE_A", "CHOICE_A")]
    [NativeMacroAlias("CHOICE", "CHOICE_A")]
    public struct CHOICE_A { public int Value; }

    public struct ChoiceHolder
    {
        [NativeRecordReference("CHOICE_TYPE")]
        public CHOICE_A Value;
    }

    [NativeRecord("_CHOICE_W", "CHOICE_W")]
    public struct CHOICE_W { public long Value; }

    [NativeTypedef]
    public unsafe struct PCHOICE_A { public CHOICE_A* Value; }

    [NativeTypedef]
    public unsafe struct PCHOICE_W { public CHOICE_W* Value; }
}

namespace Windows.Win32.Test.OtherLegacy
{
    using Windows.Win32.Test.UnicodeAliases;
    public static unsafe class Apis
    {
        [DllImport("kernel32", ExactSpelling = true)]
        public static extern PLEGACY OtherLegacy(PLEGACY value);
        [DllImport("kernel32", ExactSpelling = true)]
        public static extern global::Windows.Win32.Test.NativeOrigins.COLLISION* TagConsumer(global::Windows.Win32.Test.NativeOrigins.COLLISION* value);
    }
}

namespace Windows.Win32.Test.ForwardAliasOrder
{
    [NativeRecord("_ZRECORD", "ZRECORD")]
    public struct ZRECORD { public int Value; }
    [NativeTypedef]
    public unsafe struct PZRECORD { public ZRECORD* Value; }
}

namespace Windows.Win32.Test.UnicodeAliases
{
    [NativeRecord("_UNION", "UNION"), StructLayout(LayoutKind.Explicit)]
    public struct UNION { [FieldOffset(0)] public int Value; [FieldOffset(0)] public float Floating; }
    [NativeRecord("_UNIONW", "UNIONW"), StructLayout(LayoutKind.Explicit)]
    public struct UNIONW { [FieldOffset(0)] public long Value; [FieldOffset(0)] public double Floating; }

    [NativeRecord("_LEGACY", "LEGACY")]
    public struct LEGACY { public int Value; }
    [NativeRecord("_LEGACYW", "LEGACYW")]
    public struct LEGACYW { public long Value; }
    [NativeTypedef] public unsafe struct PLEGACY { public LEGACY* Value; }
    [NativeTypedef] public unsafe struct PLEGACYW { public LEGACYW* Value; }

    public unsafe struct LegacyHolder
    {
        public LEGACY Value;
        public LEGACY* Pointer;
        public PLEGACY Alias;
    }

    [UnmanagedFunctionPointer(CallingConvention.Winapi)]
    public unsafe delegate PLEGACY LegacyCallback(PLEGACY alias, LEGACY* pointer);

    [System.Runtime.InteropServices.Guid("E2FBCAB3-AD1F-4195-9525-4E931C8E59A1")]
    public unsafe struct ILegacyConsumer
    {
        public void** lpVtbl;
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate PLEGACY _Get(ILegacyConsumer* pThis, PLEGACY alias, LEGACY* pointer);
        public PLEGACY Get(PLEGACY alias, LEGACY* pointer)
        {
            fixed (ILegacyConsumer* pThis = &this)
                return Marshal.GetDelegateForFunctionPointer<_Get>((IntPtr)(lpVtbl[0]))(pThis, alias, pointer);
        }
    }

    public static unsafe class Apis
    {
        [DllImport("kernel32", ExactSpelling = true)]
        public static extern int LegacyFirst(PLEGACY value);

        [DllImport("kernel32", ExactSpelling = true)]
        public static extern PLEGACY LegacyReturn(LEGACY* value);
    }
}
