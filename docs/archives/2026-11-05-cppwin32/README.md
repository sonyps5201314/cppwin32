# cppwin32 本地工作结项与剩余问题归档

归档日期：2026-11-05。

按用户决定，停止继续整改本地 cppwin32 生成链路，保存当前结果、未完成问题和审计证据。本归档不是“剩余问题已经修复”或“与 Windows SDK 等价”的验收结论，也不形成自动继续执行的修复计划。后续重新投入需要新的任务决定。

## 归档基线

| 项目 | 截止版本或指纹 |
|---|---|
| cppwin32 工具及代码基线 | `83c98e3d655d3cf0da36cd5bd5b35e92e9fa4bfd` |
| GUID/IID 与 `__uuidof` 实现 | `f05d33e6d49c052d4d50b71c23a02ae86a25017e` |
| win32metadata | `d29f1d8dec2e78b12d2f7476acbb6d3b1cad7cce` |
| ClangSharp | `7fdeebf8b77fe26c3d4e06cb9f38fd05dc7d3604` |
| Windows.Win32.winmd SHA256 | `38b01425c7d879d160fabd27c1e3c5003a1ec0a3b033840b30d6f73ca587c7bc` |
| 比较器 SHA256 | `8f763ae65351c85af21957b306e6f3eee23597c664e91f7f7f7df2c83a10532e` |
| 审计输入指纹 | `702fb980807eef0ecfcf5d72ba678aab93d0c35624f686e35030efdb52519a7e` |

正式导出位置是 `Z:\ms\win32`，共 1,308 个头文件。SDK 对照来源是 `L:\Open_Code\win32metadata\generation\WinSDK\RecompiledIdlHeaders`。六组配置覆盖 x86、x64、ARM64 和 ANSI、Unicode；每组包含 321 个 SDK 常规入口及 79 个补充宏取样单元。审计结束时，记录的 4,305 个输入文件未发生哈希变化。

原始完整报告已复制到稳定目录：

`L:\Open_Code\cppwin32\build\archives\2026-11-05-cppwin32`

其中 `report.html` 可直接打开，`reports/<配置>/differences.csv` 给出逐项值和位置，`findings.csv` 包含未匹配及未验证项，`symbols.jsonl.gz` 保存全部声明证据。该目录受现有 `build` 忽略规则管理，不纳入 Git；52 个报告、日志及描述文件共 2,894,839,477 字节，各文件哈希见本目录的 `artifact-manifest.json`。它们是独立副本，原 Z 盘报告仍保留。

## 已确认而未修复的问题

以下已经回看两边源码。表中大小为 x86 字节数，源码片段及原文件哈希已冻结在 `reviewed-source-evidence.json`，即使 Z 盘输出消失也能回查。

| 编号 | 对象 | SDK 大小 | 导出大小 | 剩余缺陷 |
|---|---|---:|---:|---|
| R-001 | `CLS_SCAN_CONTEXT` | 56 | 32 | 字段级 `__declspec(align(8))` 丢失 |
| R-002 | `CONFIGURATION_ENTRY` | 8 | 1 | SDK 的两个 BSTR 数据字段未导出，被写成带 UUID 的空记录 |
| R-003 | `MFASYNCRESULT` | 40 | 4 | SDK 继承 IMFAsyncResult 并含实例数据，导出只保留基类 |
| R-004 | `OLD_LARGE_INTEGER` / `NETLOGON_LOGON_IDENTITY_INFO` | 后者 36 | 后者 40 | 双 32 位成员结构被折叠成 long long，保留大小却改变对齐和成员身份 |

### R-001：字段级对齐不能由成员类型大小代替

SDK `shared/clfs.h:814` 的 `_CLS_SCAN_CONTEXT` 对 `cIndex`、`cContainers`、`cContainersReturned`、`eScanMode`、`pinfoContainer` 显式要求 8 字节对齐；导出 `complex_structs.h:64435` 中没有这些要求。例如 `cIndex` 的字节偏移由 16 变成 12，`cContainers` 由 24 变成 16。这不是 ULONG 宽度改变，而是声明附加布局信息未能保留。

### R-002 / R-003：记录与接口的分类仍不可靠

SDK `um/ftpext.h:198` 的 `CONFIGURATION_ENTRY` 虽带 UUID，仍有 `bstrKey` 和 `bstrValue`；导出 `complex_interfaces.h:22021` 为空。SDK `um/mfapi.h:380` 的 `tagMFASYNCRESULT` 除接口基类外还有 OVERLAPPED、回调、状态、传输计数和事件句柄；导出 `complex_interfaces.h:71315` 中没有这些字段。

UUID、接口基类、虚函数与实例字段是不同维度。当前链路仍存在把普通数据记录或具有实例数据的派生记录归到纯接口输出路径的情况。这里只确认最终产物缺失字段，尚未完成从 ClangSharp、C#、WinMD 到 C++ 各阶段的全部责任定位。

### R-004：只保留宽度会丢失对齐和声明身份

SDK `um/subauth.h:47` 的 `OLD_LARGE_INTEGER` 是 `{ ULONG LowPart; LONG HighPart; }`，导出 `complex_structs.h:82363` 是 `typedef long long OLD_LARGE_INTEGER;`。两者均为 8 字节，但 x86 对齐不同，影响 `NETLOGON_LOGON_IDENTITY_INFO` 及引用它的其他记录。这说明仅按大小、算术类型或包装结构形式折叠类型，不能证明布局和 SDK 声明身份保持不变。

## 尚未完成归因的候选清单

当前报告没有全面归因，也没有算出“剩余独立产品缺陷总数”。以下是按配置的声明记录数，一条声明可能有多个差异，同一个根因也可能影响多个声明和配置。

| 配置 | 声明记录 | 标记为不同 | 记录大小差异 | 字段偏移差异 | 未匹配 | 同名歧义 |
|---|---:|---:|---:|---:|---:|---:|
| x86 / ANSI | 394,189 | 142,449 | 14 | 18 | 13,321 | 6,812 |
| x86 / Unicode | 394,223 | 139,934 | 18 | 29 | 13,415 | 6,706 |
| x64 / ANSI | 396,319 | 141,516 | 15 | 20 | 13,827 | 6,666 |
| x64 / Unicode | 396,353 | 139,003 | 19 | 31 | 13,921 | 6,560 |
| ARM64 / ANSI | 395,961 | 141,506 | 15 | 20 | 13,598 | 6,664 |
| ARM64 / Unicode | 395,995 | 138,993 | 19 | 31 | 13,692 | 6,558 |

完整分类与计数保存在 `audit-summary.json`。仍需审核的范围包括：

- **其他布局候选**：记录大小和对齐、成员大小和对齐、位域宽度、字段偏移；除上文实例外，尚未逐项确认为产品缺陷。
- **类型身份与名称表示**：typedef、结构体标签、宏名称、限定符、指针/引用、数组和回调表示；宽度相同不能自动满足保留 SDK 写法的目标，也不能把全部严格差异当成 ABI 破坏。
- **函数和接口候选**：返回值/参数、参数名、调用约定、可变参数、基类和直接声明的虚函数顺序。没有完成全部同名、重载及跨头文件上下文的归因。
- **常量候选**：实际值、宏表达式大小、有符号数表示、宏的多定义与求值失败。宏表达式的 sizeof 并不等于原 SDK 中存在同样大小的存储对象。
- **C++ 表面差异**：每组约 4.1 万条声明有异常说明差异，主要涉及生成的 noexcept。是否保留、如何纳入允许规则，没有在本轮裁定。
- **未覆盖证明的项目**：未匹配、歧义、不完整类型、未实例化模板、不能求值和部分候选不能验证等情况，均未算作通过。

修复 R-001 至 R-004 不意味着这些候选将全部消失。不能从当前计数推断还存在十几万处独立错误，也不能声称仅有上述四个实例。

## 已保存的逐项待查索引

`remaining-declarations.csv.gz` 是全部六组 `findings.csv` 的压缩合并索引。原六组共 1,052,261 行待查记录，按状态、类别、名称、差异类别以及双侧文件/行号合并为 180,921 行，并为每行记录所属配置。

这个数字是归档索引行数，不是根因或确认缺陷数。`backlog-counts.json` 保留每个配置及状态的原计数，归档时已反向展开验证，确保没有因跨配置合并丢失记录。SDK 文件路径使用相对于 RecompiledIdlHeaders 的路径，导出文件路径相对于 win32 目录。

索引包含定位信息和差异类别；具体两边的值、原始声明、SDK 候选及求值证据，在稳定完整报告目录中保存。`header-hashes.json` 冻结 1,308 个被审计头文件的哈希，`input-manifest.json.gz` 冻结全部审计输入哈希。

## 已接受的变更与例外

下列工作已经完成或是明确选择，不应在阅读剩余问题时误当成待回退项：

- `.0.h` 的前置声明/typedef 去重和记录附近的别名组织已有对应提交与检查。
- 接口 GUID 已输出为公开 IID 常量，接口 UUID 支持 __uuidof；SDK 类型上的 UUID 可作为 IID 的源证据，按完整 16 字节比较。
- `enums.json` 的字段、参数及返回值替换按具体 use/addUsesTo 规则识别，允许名称变化但不免除宽度、限定符、指针/数组结构与偏移检查。
- 宏生成的常量允许对应 SDK 宏，不能求值或存在歧义时保留未验证状态。
- GetDeviceID 的后续 DLL 重名声明采用 DLL 名称前缀，是用户要求的本地处理；应单独审查，不应擅自改回 C++ 转发包装。
- base.h 使用两个 SDK 配置头绝对路径，是用户在 2026-11-05 的明确本地选择；较早的 SDK 独立配置方案不是当前状态。

## 架构与验收边界

1. **跨阶段信息损失尚未被证明全部解决。** Clang AST、C# 包装、WinMD 类型和 C++ 声明并不具备一一对应的天然关系；记录类别、字段属性、真实 typedef/宏和条件信息仍可能在其中某个阶段丢失或变形。
2. **原 SDK 的组织与条件上下文不能由逻辑命名空间完整代替。** 同名类型、头文件组合、A/W 选择、架构和 API family 会影响可见声明。当前检查使用已记录的分区和桌面 API 上下文，不是所有 SDK 版本/宏组合的穷举。
3. **全局依赖和大文件输出增加审核成本。** 即使编译通过，也不能因此证明全部原 SDK 身份、声明组织和布局正确。
4. **校验器本身仍是严格检查与关联工具，不是完整等价证明器。** 类型身份、名称写法、noexcept、宏表达式类型及整数表示分别记录；匹配规则也可能留下误报、漏关联或需要人工处理的情况。
5. **C++ sizeof 与 ABI 存储不能混用。** 引用的语言 sizeof 和参数/成员指针宽度已在工具中分开，相关差异另记为 cxx_sizeof 项。
6. **没有完整二进制层证明。** 头文件 AST 不给出函数机器码长度或链接后地址；ARM64 数据来自 Clang 交叉目标布局，没有执行 ARM64 二进制。

审计主解析单元为零错误、26 个故障注入测试通过、报告 CRC 和行数通过，只证明本轮工具运行及文件完整性符合记录。`all_symbols_verified` 明确为 false。

## 保留、排除与复现

- 自校验器源码、运行脚本和说明仍在 `tools/SdkDeepAudit`，没有删除研究代码。
- `verification.json`、`input-verification.json`、`profile-corrections.json` 保留解析器/规则指纹、输入未变、各配置计数及 C++20 分区设置修正的证据。
- 本次没有修改生成器、ClangSharp、win32metadata、WinMD 或 Z:\ms 中的输出，也没有生成 Z:\TYPEDEF。
- 用户原有的 SampleD3DApp.vcxproj 修改和 ClangSharp launchSettings.json 修改保留，未纳入本次归档提交；工作树清单见 artifact-manifest.json。
- 没有新建备份分支、重写历史或推送。完整报告使用复制方式保存，未迁移或删除原 Z 盘目录。

如将来重新评估，可使用 `tools/SdkDeepAudit/Run.ps1` 针对指定头文件和元数据仓库重新构建、提取和比较；已有完整报告的总览页面可以直接打开。新的运行不应覆盖本归档目录，并应建立新的输入指纹和结果目录。这里保留复现方法，不自动恢复已停止的整改工作。
