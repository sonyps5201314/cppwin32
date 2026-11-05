# 原始 SDK 与导出声明的首批人工复核

这里只记录已经回看两边源码的候选，不把全部自动差异直接视为产品缺陷。本轮实现校验器，不在这里顺带修改发射器或头文件。

| 项目 | x86 SDK 大小 | 当前导出大小 | 已确认的声明差异 |
|---|---:|---:|---|
| `CLS_SCAN_CONTEXT` | 56 | 32 | SDK 多个字段有 `__declspec(align(8))`，导出字段缺少这些要求 |
| `CONFIGURATION_ENTRY` | 8 | 1 | SDK 有两个 BSTR 字段，导出为带 UUID 的空 `novtable` 记录 |
| `MFASYNCRESULT` | 40 | 4 | SDK 继承 IMFAsyncResult 且有数据字段，导出只剩基类 |
| `NETLOGON_LOGON_IDENTITY_INFO` | 36 | 40 | SDK 的 OLD_LARGE_INTEGER 是两个 32 位成员的结构体，导出改成 long long，改变了对齐 |

`CLS_SCAN_CONTEXT` 的 SDK 来源是 `generation/WinSDK/RecompiledIdlHeaders/shared/clfs.h:814`，导出来源是 `Z:/ms/win32/impl/complex_structs.h:64435`。例如 `cIndex` 的字节偏移由 SDK 的 16 变成 12，`cContainers` 由 24 变成 16。这是字段级对齐丢失，不是 ULONG 宽度发生了变化。

`CONFIGURATION_ENTRY` 的来源为 `um/ftpext.h:198`，包含 `bstrKey`、`bstrValue`；导出为 `complex_interfaces.h:22021` 的空记录。UUID 属性本身不能证明一个记录是没有数据字段的 COM 接口。

`MFASYNCRESULT` 的来源为 `um/mfapi.h:380`，除了基类还有 `overlapped`、`pCallback`、`hrStatusResult`、`dwBytesTransferred`、`hEvent`；导出 `complex_interfaces.h:71315` 中没有这些字段。继承接口的记录也可能是有实例数据的结构体。

`OLD_LARGE_INTEGER` 的来源为 `um/subauth.h:47`，由 `ULONG LowPart`、`LONG HighPart` 构成；导出 `complex_structs.h:82363` 为 `typedef long long OLD_LARGE_INTEGER;`。两者大小相同，对齐要求却不同，导致 NETLOGON_LOGON_IDENTITY_INFO 及引用它的其他记录发生偏移或大小变化。

后续修正宜先处理真实布局：保留字段级对齐、避免把带 UUID 或接口基类的普通结构误作纯接口、避免仅按宽度折叠具有不同对齐和成员身份的结构体。名称写法、枚举替换和 C++ noexcept 应使用报告中的独立类别审核。
