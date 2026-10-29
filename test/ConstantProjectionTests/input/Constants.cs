using System;
using DllImport = System.Runtime.InteropServices.DllImportAttribute;
using Windows.Win32.Foundation.Metadata;

namespace Windows.Win32.Foundation.Metadata
{
    [AttributeUsage(AttributeTargets.Assembly, AllowMultiple = true)] public class NativeAliasAttribute : Attribute { public NativeAliasAttribute(string name, string declaration, string canonical) { } }
    public class NativeDeclaratorAttribute : Attribute { public NativeDeclaratorAttribute(string declaration, string dependencies) { } }
    public class NativePrimitiveTypeAttribute : Attribute { public NativePrimitiveTypeAttribute(string name) { } }
    public class NativeTypeNameAttribute : Attribute
    {
        public NativeTypeNameAttribute(string value) { }
    }

    public class NativeTypedefAttribute : Attribute { }
    public class MetadataTypedefAttribute : Attribute { }
    [AttributeUsage(AttributeTargets.Struct, AllowMultiple = true)] public class NativeRecordAttribute : Attribute { public NativeRecordAttribute(string tag, string alias) { } }
    [AttributeUsage(AttributeTargets.Struct, AllowMultiple = true)] public class NativeMacroAliasAttribute : Attribute { public NativeMacroAliasAttribute(string name, string target) { } }
    public class NativeRecordReferenceAttribute : Attribute { public NativeRecordReferenceAttribute(string name) { } }
    [AttributeUsage(AttributeTargets.Field, AllowMultiple = true)] public class NativeBitfieldAttribute : Attribute { public NativeBitfieldAttribute(string name, long offset, long length) { } }
    [AttributeUsage(AttributeTargets.Field, AllowMultiple = true)] public class NativeBitfieldTypeAttribute : Attribute { public NativeBitfieldTypeAttribute(string name, string typeNamespace, string typeName) { } }
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

namespace Windows.Win32.Test.ForwardDeclarations
{
    [NativeTypedef]
    public unsafe struct PPACKET { public PACKET* Value; }

    [MetadataTypedef]
    public unsafe struct MPACKET { public PACKET* Value; }

    [NativeTypedef]
    public unsafe struct PPPACKET { public PPACKET* Value; }

    [SupportedArchitecture(Architecture.X64 | Architecture.Arm64)]
    public struct PACKET { public ulong Address; public uint Tag; }

    [SupportedArchitecture(Architecture.X86)]
    [System.Runtime.InteropServices.StructLayout(System.Runtime.InteropServices.LayoutKind.Sequential, Pack = 1)]
    public struct PACKET____1 { public uint Address; public uint Tag; }
}

namespace Windows.Win32.Test.NativeOrigins
{
    [NativeRecord("_SDK_RECORD", "SDK_RECORD_STRUCT")]
    [NativeRecord("_SDK_RECORD", "SDK_RECORD_ALIAS")]
    [NativeMacroAlias("SDK_RECORD", "SDK_RECORD_STRUCT")]
    [NativeMacroAlias("SDK_RECORD_CHAIN", "SDK_RECORD")]
    public struct SDK_RECORD { public int Value; }

    [NativeRecord("_SDK_OTHER", "SDK_OTHER")]
    [NativeRecord("_SDK_OTHER", "SDK_OTHER_ALIAS")]
    public struct ManagedOther { public int Value; }

    public unsafe struct OriginHolder
    {
        [NativeTypeName("SDK_RECORD_STRUCT *")] public SDK_RECORD* Record;
        [NativeTypeName("SDK_OTHER *")] public ManagedOther* Other;
        [NativeTypeName("SDK_OTHER_ALIAS *")] public ManagedOther* NamedAlias;
    }
}

namespace Windows.Win32.Test.NativeOrigins
{
    [NativeRecord("COLLISION", "COLLISION_STRUCT")]
    [NativeMacroAlias("COLLISION", "COLLISION_STRUCT")]
    public struct COLLISION { public int Value; }

    [NativeRecord("", "ANONYMOUS_STRUCT")]
    [NativeMacroAlias("ANONYMOUS", "ANONYMOUS_STRUCT")]
    public struct ANONYMOUS { public int Value; }
}

namespace Windows.Win32.Test.NativeOrigins
{
    [NativeRecord("BareTag", "")]
    [NativeMacroAlias("PUBLIC_BARE_RECORD", "BareTag")]
    public struct PUBLIC_BARE_RECORD { public int Value; }
}

namespace Windows.Win32.Test.NativeOrigins
{
    [NativeRecord("ActualBareTag", "")]
    public struct ManagedTagOnly { public int Value; }
}

namespace Windows.Win32.Test.Bitfields
{
    public struct BitfieldAliases
    {
        [NativeBitfield("First", 0, 4), NativeBitfield("Second", 4, 4)] public byte _bitfield;
        [NativeTypeName("UINT8 : 4")] public byte First { get => 0; }
        [NativeTypeName("BYTE : 4")] public byte Second { get => 0; }
    }
    public struct WideBitfields
    {
        [NativeBitfield("StartupSuspend", 0, 1), NativeBitfield("Reserved", 1, 63)] public ulong _bitfield;
        [NativeTypeName("UINT64 : 1")] public ulong StartupSuspend { get => 0; }
        [NativeTypeName("UINT64 : 63")] public ulong Reserved { get => 0; }
    }
    public struct SignedBitfields
    {
        [NativeBitfield("Signed", 0, 4), NativeBitfield("Unsigned", 4, 28)] public uint _bitfield;
        [NativeTypeName("int : 4")] public int Signed { get => 0; }
        [NativeTypeName("unsigned int : 28")] public uint Unsigned { get => 0; }
    }
    public struct LegacyBitfields
    {
        [NativeBitfield("Value", 0, 32)] public uint _bitfield;
    }
}

namespace Windows.Win32.Test.Primitives
{
    public unsafe struct PrimitiveProbe
    {
        [NativeTypeName("long")] public int NativeLong;
        [NativeTypeName("unsigned long")] public uint NativeUnsignedLong;
        [NativeTypeName("const unsigned short *")] public ushort* Words;
        public int PlainInt;
    }
    public static unsafe class Apis
    {
        [DllImport("synthetic.dll", CallingConvention = System.Runtime.InteropServices.CallingConvention.Cdecl)]
        [return: NativeTypeName("long")]
        public static extern int PrimitiveMethod([NativeTypeName("long *")] int* value, int plain);
        [DllImport("synthetic.dll")]
        [return: NativeTypeName("VOID")]
        public static extern void PrimitiveVoid([NativeTypeName("long *")] int* first, int second);
        public const uint PLAIN_UNSIGNED = 1001;
        public const int PLAIN_SIGNED = -1;
    }
    [return: NativeTypeName("unsigned long")]
    public unsafe delegate uint PrimitiveCallback([NativeTypeName("long *")] int* value);
}

namespace Windows.Win32.Test.NativeCallbacks
{
    [System.Runtime.InteropServices.UnmanagedFunctionPointer(System.Runtime.InteropServices.CallingConvention.Cdecl)]
    public delegate int CdeclCallback(int value);
    public struct InlineCallbacks
    {
        [NativeTypeName("int (*)(int) __attribute__((cdecl))")] public IntPtr First;
        [NativeTypeName("int (*)(int) __attribute__((stdcall))")] public IntPtr Second;
        [NativeTypeName("int (*)(void (*)(int) __attribute__((stdcall))) __attribute__((cdecl))")] public IntPtr Nested;
        [NativeTypeName("int (*)[4]")] public IntPtr Array;
    }
    public static class Apis
    {
        [DllImport("synthetic.dll")]
        public static extern void Register([NativeTypeName("int (*)(int) __attribute__((stdcall))")] IntPtr callback);
        [DllImport("synthetic.dll")]
        [return: NativeTypeName("int (*)(int) __attribute__((stdcall))")]
        public static extern IntPtr GetCallback();
    }
}
