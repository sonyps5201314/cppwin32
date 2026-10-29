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
