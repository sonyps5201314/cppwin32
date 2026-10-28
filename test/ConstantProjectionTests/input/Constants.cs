using System;
using DllImport = System.Runtime.InteropServices.DllImportAttribute;
using Windows.Win32.Foundation.Metadata;

namespace Windows.Win32.Foundation.Metadata
{
    public class NativeTypeNameAttribute : Attribute
    {
        public NativeTypeNameAttribute(string value) { }
    }

    public class NativeTypedefAttribute : Attribute { }
    public class ConstAttribute : Attribute { }
    public class IncompleteArrayAttribute : Attribute { }
    [Flags] public enum Architecture { X86 = 1, X64 = 2, Arm64 = 4, All = 7 }
    public class SupportedArchitectureAttribute : Attribute
    {
        public SupportedArchitectureAttribute(Architecture value) { }
    }
}

namespace Windows.Win32.Foundation
{
    [NativeTypedef]
    public struct HRESULT { public int Value; }

    [NativeTypedef]
    public unsafe struct PWSTR { public ushort* Value; }

    [NativeTypedef]
    public struct HANDLE_PTR { public UIntPtr Value; }
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

namespace Windows.Win32.Test.Platform
{
    [SupportedArchitecture(Architecture.X86)]
    public struct FLOATOBJ { public uint First; public uint Second; }

    [NativeTypedef]
    [SupportedArchitecture(Architecture.X64 | Architecture.Arm64)]
    public struct FLOATOBJ____1 { public float Value; }

    [SupportedArchitecture(Architecture.X86)]
    public struct FLOATOBJ_XFORM
    {
        public FLOATOBJ First; public FLOATOBJ Second; public FLOATOBJ Third;
        public FLOATOBJ Fourth; public FLOATOBJ Fifth; public FLOATOBJ Sixth;
    }

    [SupportedArchitecture(Architecture.X64 | Architecture.Arm64)]
    public struct FLOATOBJ_XFORM____1
    {
        public FLOATOBJ____1 First; public FLOATOBJ____1 Second; public FLOATOBJ____1 Third;
        public FLOATOBJ____1 Fourth; public FLOATOBJ____1 Fifth; public FLOATOBJ____1 Sixth;
    }
}

namespace Windows.Win32.Test.InterfaceProjection
{
    [NativeTypedef]
    public unsafe struct DOT11_MAC_ADDRESS { public fixed byte Value[6]; }

    [NativeTypedef]
    public unsafe struct PDOT11_MAC_ADDRESS { public DOT11_MAC_ADDRESS* Value; }

    [NativeTypedef]
    public unsafe struct SpatialAudioHrtfOrientation { public fixed float Value[9]; }

    public enum NativeMode : uint { One = 1 }

    [NativeTypedef]
    public struct MODE_ALIAS { public NativeMode Value; }

    [NativeTypedef]
    public struct PixelFormat { public int Value; }

    public enum InterpolationMode : int { Invalid = -1, Default = 0 }

    [NativeTypedef]
    public struct Image { public IntPtr Value; }

    public struct EncoderParameters { public uint Count; }

    public unsafe struct ProjectionProbe
    {
        [NativeTypeName("Boolean *")] public short* Transactional;
        public bool ManagedBoolean;
        [NativeTypeName("PixelFormat *")] public uint* Format;
        [NativeTypeName("InterpolationMode")] public uint Hints;
        [NativeTypeName("EncoderParameters **")] public byte** Encoders;
        [NativeTypeName("Image **")] public byte** Frames;
        [NativeTypeName("Image *")] public byte* Frame;
        [NativeTypeName("HANDLE_PTR")] public uint PortableHandle;
        [NativeTypeName("MODE_ALIAS")] public NativeMode Mode;
        [NativeTypeName("LPVOID")] public byte* Bytes;
        [NativeTypeName("LPVOID")] public void* Address;
        [NativeTypeName("FLOAT[3][4]")] public fixed float Transform[3 * 4];
        [NativeTypeName("DOT11_MAC_ADDRESS")] public fixed byte MacAddress[6];
        [NativeTypeName("DOT11_MAC_ADDRESS[2]")] public fixed byte MacAddresses[2 * 6];
        [NativeTypeName("PDOT11_MAC_ADDRESS")] public byte** MacPointer;
        [NativeTypeName("SpatialAudioHrtfOrientation *")] public float** Orientation;
    }

    public static unsafe class Apis
    {
        [NativeTypeName("MODE_ALIAS")] public const uint MODE_ONE = 1;

        [DllImport("projection.dll")]
        public static extern int GetNameIdMap([NativeTypeName("LPCWSTR *[]")] [IncompleteArray] ushort*** names);
    }
}
