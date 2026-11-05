# typedef 常量、架构及接口类型投影回归测试

运行前使用 devenv.com 构建 cppwin32 的 Debug|x64，并准备同级 win32metadata 的 net10.0 发射器，然后执行：

```powershell
& .\test\ConstantProjectionTests\Run.ps1
```

也可以通过 `-Emitter` 指定 ClangSharpSourceToWinmd.exe 路径。脚本从最小 C# 输入生成 winmd，再导出头文件，最后使用 Visual Studio 18 的 devenv.com 构建和运行 x64、x86 两套原生断言。

验证内容：

- 普通及多层算术 typedef 的常量位于其所属命名空间，值和类型正确。
- 指针宽度整数 typedef 和浮点 typedef 可以用于 constexpr。
- 字符串保持宽字符、只读指针和完整值，不进入整数枚举。
- 未使用 typedef 的普通常量保持原有输出。
- 架构受限的 NativeTypedef 在前置声明处也保留条件；FLOATOBJ 在 x86 为8字节结构体，在 x64 为 float，FLOATOBJ_XFORM 分别为48和24字节。
- HRESULT、HTREEITEM 和整数资源标识符的已有全局枚举接口保持兼容。
- 原生 Boolean 保留16位有符号别名，托管 bool 仍为 bool；条件 BYTE/DWORD 占位声明恢复图像类型及原有指针层数，标量枚举通过明确的成员映射恢复。

- 枚举别名可以用于所属命名空间的 constexpr 常量，值使用明确的枚举转换。
- HANDLE_PTR 使用平台指针宽度；LPVOID 保持 void*，固定 FLOAT[3][4] 保持连续的 48 字节数组。
- DOT11_MAC_ADDRESS、SpatialAudioHrtfOrientation 保留数组范围，PDOT11_MAC_ADDRESS 为指向数组的指针；外层数组不重复累计元素数。
- GetNameIdMap 的字符串别名保留显式指针及数组退化，精确函数指针类型为 wchar_t const***。
- NativeTypedef、MetadataTypedef 及多层指针别名在使用前声明所有架构的同名结构；PACKET 的 x64、x86 大小和对齐分别验证，实际 SDK 另由 SdkForwardDeclarationTests 检查。
- 类型宏直接输出 `#define`，不使用 `#ifndef` 跳过已有定义。x64/x86 均验证同值重复定义编译通过，不同值重复定义由生成头文件触发 C4005；测试将该警告提升为错误。
- NativeRecord/NativeMacroAlias 分别重建真实 tag、typedef 和对象式类型宏；覆盖宏链、匿名 record、无 typedef 的裸 tag、宏/tag 同名保护及纯托管重命名。NativeRecordReference 单独保留实际引用的 typedef 名称。

算术 typedef 判定沿唯一实例 Value 字段解析，排除指针、数组、非 typedef 和循环。HRESULT、NTSTATUS、BOOL、VARIANT_BOOL、BOOLEAN、DEVPROP_BOOLEAN、HTREEITEM、SOCKET、WSAEVENT 保持历史枚举策略；这些类型在 LASTOK 中已经使用该接口，不属于此次新增 typedef 造成的回退。

生成文件、winmd、日志和可执行文件均位于测试目录的 obj 子目录，不提交。

位域还验证：同一存储字段中的不同 typedef 按成员分别恢复，`UINT8`、`BYTE`、`UINT64` 保留 SDK 拼写，`int`/`unsigned int` 保留符号性；没有新属性的旧 WinMD 仍使用原有存储类型输出。两个平台都检查结构大小及位域声明类型。

基础整数使用 C++ 关键字，SDK 的真实 typedef 继续保留。NativePrimitiveType 单独恢复 long 等原始拼写；x64/x86 检查函数、委托、字段、const 指针、普通宏常量，以及 void 返回值带属性时的参数位置。

复杂声明还检查内联 cdecl/stdcall 回调、嵌套回调、数组指针及返回函数指针的函数，结合真实类型断言验证 x64/x86。命名回调读取实际 UnmanagedFunctionPointer，导出函数读取 ImplMap 调用约定。

声明冲突还检查原生别名与其他分区类型重名、宏与合成枚举重名、结构体标签的多余 typedef、嵌套标签和嵌套类型别名、回调参数的声明顺序，以及名为 Base 的整数成员和真正的结构体基类。备用别名冲突不会将唯一结构体整体移入命名空间；有独立主类型的同名备用别名不重复输出，字段声明和结构依赖排序均使用最终输出名称对应的类型。跨架构指针 typedef 验证前置声明不会因其他架构的生成路径而遗漏。

人为定义的枚举在 enums.json 中正式使用有意义的名称，例如 SEC_WINNT_AUTH_IDENTITY_FLAGS、SERVICE_NOTIFY_FLAGS；C++ 保留其枚举类型及全部值，不临时添加 _ENUM，也不将枚举使用位置降回整数。命名原生结构的完整定义使用 `typedef struct tag { ... } name;`，在定义所在文件保留真实 typedef；前置声明头继续提供依赖所需的提前声明。

条件类型别名通过 NativeConditionalAlias 保留 SDK 条件、宏或 typedef 类别，以及两支目标。使用 `-CharacterSet Ansi`、`-CharacterSet Unicode` 分别验证 x64/x86：包括双分支宏、条件 typedef、指针别名、仅在 UNICODE 下选择 W 的单分支宏，以及按值字段对两个目标完整定义的依赖。base.h 保留投影配置常量，并按绝对路径引用 sdkddkver.h、winapifamily.h 提供版本及分区定义。

单分支宏还验证：真实 ANSI typedef 在函数参数和返回值、回调、接口、按值字段、指针字段和 typedef 定义中保留原名，同时保持原来的 ANSI 类型。相关原始声明完成后再定义选择宏，UNICODE 使用方仍选择 W。没有独立 WinMD 类型的指针 typedef 从真实 NativeAlias 来源补齐；union 前置声明保留 union 类别。真实 SDK 检查包含四个 ToolHelp API 以及 PMODULEENTRY32、PPROCESSENTRY32 在两个字符集中的可用性。

输出按记录归属组织：完整定义旁保留记录和指针别名；条件类型关系就近分组；遮蔽原有类型或标签的宏放到公开分区声明末尾。测试要求不存在 native_declarations.h、native_aliases.h，也不通过 push_macro/pop_macro 修补自身顺序。IncludeOrder.cpp 反向包含消费者和提供者，imports/SdkConfiguration.cpp 提前包含真实 SDK 配置头。宏冲突负例继续检查 C4005，真实类型宏不套 #ifndef。

`test/AuditForwardDeclarations.py` 逐个 `.0.h` 检查相同命名空间、条件和有效架构下的重复前置声明、typedef 及命名回调；不会把不同架构或 ANSI/Unicode 分支视为重复。合成测试和真实 SDK 前置声明测试都会运行此检查。`test/AuditForwardDeclarationsTests.py` 验证检查器能识别架构交集，同时保留互斥分支和不同命名空间。结构体完整定义旁保留的别名仍由布局检查验证。

imports/SdkConfiguration.cpp 验证 SDK 配置头提前包含的兼容性。test/ConfigurationHeaders/Run.ps1 验证版本、分区和用户覆盖；test/AuditSdkConfiguration.py 检查两个 SDK 配置头只在 base.h 以指定绝对路径引用。

导出时增加 `-contracts <文件路径>` 会生成复杂原生声明、所有条件别名和单分支原名的编译检查，并生成同名 .aliases.tsv 归属清单。将检查头通过本脚本 `-NativeContracts <文件路径>` 传入，可在 x64/x86 符号工程中同时编译。test/AuditDeclarationLayout.py 将归属清单与实际完整定义旁的别名核对，并检查旧辅助头、旧 include 及宏保护序列是否归零。默认发射器为已构建的 Release/net10.0/ClangSharpSourceToWinmd.exe；可用 `-Emitter` 指定其他路径。

`test/CompareDeclarations.py` 全量比较 API、回调、接口、字段、typedef、枚举成员和常量的声明原文。`--baseline 批次名=win32目录` 可重复指定多个批次，输出包含逐项差异、文件位置和 SHA256；不以 ABI 等价忽略类型拼写。`--review 批次名=精确变化清单` 会对照已审核的旧值和新值，发现任何额外变化立即失败。该脚本针对生成器的单行声明，完整头文件 diff 和原生编译仍需保留。

已审核的条件宏改动前基准是 `Z:\sdk-declarations-20261102\headers\win32`，可执行以下检查。清单中包含条件别名补充、用户确认的 FLAGS 枚举投影和先前 RPC 版本标记；不能随新输出自动重写清单。

```powershell
python test/CompareDeclarations.py --current Z:\TYPEDEF\win32 `
  --baseline preunicode=Z:\sdk-declarations-20261102\headers\win32 `
  --review preunicode=test/sdk-spelling-preunicode-review.json.gz `
  --output Z:\sdk-spelling-check
```

`imports/ImportCollisions.sln` 使用 `devenv.com` 构建独立元数据，保留两个 DLL 中同名方法的导入记录。首次出现的声明保留原名，其他 DLL 的同名声明加上 DLL 文件名主干和下划线，并产生 `CPPWIN32001` 编译警告。输出保持 `extern "C"` 和原调用约定。

`imports/ImportSymbols.sln` 将声明地址编译到静态库对象中，使用 `dumpbin /symbols` 核对 x64 的普通 C 名称及 x86 的 stdcall 名称，排除 C++ 函数修饰名。测试不链接或调用更名后的 DLL 函数。可使用 `-SdkHeaders Z:\ms` 同时检查真实 SDK 的 `GetDeviceID`、`tbs_GetDeviceID` 和全局 `_LSA_UNICODE_STRING`；完整 SDK 的工程检查另见 `test/SampleD3DApp/Run.ps1`。
