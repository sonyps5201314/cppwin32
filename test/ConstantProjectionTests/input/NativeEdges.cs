using System;
using Windows.Win32.Foundation.Metadata;

namespace Windows.Win32.Foundation.Metadata
{
    public class NativeInheritanceAttribute : Attribute { public NativeInheritanceAttribute(string name) { } }
}

namespace Windows.Win32.Test.NameOwner
{
    public enum SharedNativeName_FLAGS { Value = 7 }
}

namespace Windows.Win32.Test.CollidingRecords
{
    [NativeRecord("_CollidingRecord", "CollidingRecord")]
    [NativeRecord("_CollidingRecord", "SharedNativeName")]
    public struct CollidingRecord { public long Value; }
}

namespace Windows.Win32.Test.NativeEdges
{
    public enum KIND_FLAGS : uint { Value = 1 }

    [NativeRecord("_KindRecord", "KindRecord")]
    [NativeMacroAlias("KIND_MACRO", "KindRecord")]
    public struct KindRecord { public int Value; }

    public struct FlagHolder
    {
        [NativeTypeName("unsigned long")] public KIND_FLAGS Flags;
        [NativeTypeName("DWORD")] public KIND_FLAGS Mask;
    }

    [NativeRecord("_MethodRecord", "MethodRecord")]
    public struct MethodRecord { public int Value; }

    [NativeTypedef]
    public struct _MethodRecord { public MethodRecord Value; }

    [NativeTypedef]
    public struct ScalarAlias { public ulong Value; }

    public struct ScalarBaseMember { public ScalarAlias Base; public int Tail; }

    public struct BaseRecord { public int Value; }

    [NativeInheritance("BaseRecord")]
    public struct DerivedRecord { public BaseRecord Base; public int Tail; }

    public struct NestedOwner
    {
        [NativeRecord("_NestedTag", "")]
        public struct NestedRecord { public int Value; }

        [NativeRecordReference("_NestedTag")]
        public NestedRecord Field;
    }

    [NativeTypedef]
    public struct NestedAlias { public NestedOwner.NestedRecord Value; }

    public struct NestedAliasHolder { public NestedAlias Field; }

    [NativeTypedef]
    public struct EarlyCallbackAlias { public OrderedCallback Value; }

    public unsafe delegate void OrderedCallback(ZCallbackArgument* argument);

    [NativeRecord("_ZCallbackArgument", "ZCallbackArgument")]
    public struct ZCallbackArgument { public int Value; }

    [SupportedArchitecture(Architecture.X64 | Architecture.Arm64)]
    public enum ARCH_ENUM { Width = 8 }

    [SupportedArchitecture(Architecture.X86)]
    public enum ARCH_ENUM____1 { Width = 4 }
}


namespace Windows.Win32.Test.ForwardDeclarations
{
    [NativeTypedef, SupportedArchitecture(Architecture.X64 | Architecture.Arm64)]
    public unsafe struct ARCH_POINTER { public PACKET* Value; }

    [NativeTypedef, SupportedArchitecture(Architecture.X86)]
    public unsafe struct ARCH_POINTER____1 { public PACKET____1* Value; }
}

namespace Windows.Win32.Test.NameOwner
{
    [NativeRecord("_CanonicalText", "CanonicalText")]
    public struct CanonicalText { public int Value; }
}

namespace Windows.Win32.Test.CollidingRecords
{
    [NativeRecord("_AlternateText", "AlternateText")]
    [NativeRecord("_AlternateText", "CanonicalText")]
    public struct AlternateText { public int Value; }
}

namespace Windows.Win32.Test.CollidingRecords
{
    public struct CanonicalReferenceHolder
    {
        [NativeRecordReference("CanonicalText")]
        public AlternateText Field;
    }
}
