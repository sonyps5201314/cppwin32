using System;
using Windows.Win32.Foundation.Metadata;

namespace Windows.Win32.Foundation.Metadata
{
    public class NativeTypeNameAttribute : Attribute
    {
        public NativeTypeNameAttribute(string value) { }
    }

    public class NativeTypedefAttribute : Attribute { }
}

namespace Windows.Win32.Foundation
{
    [NativeTypedef]
    public struct HRESULT { public int Value; }

    [NativeTypedef]
    public unsafe struct PWSTR { public ushort* Value; }
}

namespace Windows.Win32.UI.Controls
{
    [NativeTypedef]
    public struct HTREEITEM { public IntPtr Value; }
}

namespace Windows.Win32.Test.Constants
{
    [NativeTypedef]
    public struct DWORD { public uint Value; }

    [NativeTypedef]
    public struct COUNT { public DWORD Value; }

    [NativeTypedef]
    public struct POINTER_INTEGER { public UIntPtr Value; }

    [NativeTypedef]
    public struct FLOAT_VALUE { public float Value; }

    public static class Apis
    {
        [NativeTypeName("DWORD")]
        public const uint MAX_VALUE = 0xffffffff;

        [NativeTypeName("COUNT")]
        public const uint CHAIN_VALUE = 42;

        [NativeTypeName("POINTER_INTEGER")]
        public const uint POINTER_INTEGER_VALUE = 12;

        [NativeTypeName("FLOAT_VALUE")]
        public const float FLOAT_CONSTANT = 1.5f;

        [NativeTypeName("PWSTR")]
        public const string TEXT = "Constant recovery";

        [NativeTypeName("HRESULT")]
        public const int LEGACY_FAILURE = -1;

        [NativeTypeName("HTREEITEM")]
        public const int LEGACY_TREE = -1;

        [NativeTypeName("PWSTR")]
        public const uint LEGACY_RESOURCE = 1;

        public const uint UNTYPED = 7;
    }
}
