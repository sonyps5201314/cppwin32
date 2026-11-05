# SDK AST 深度自校验

2026-11-05 本地整改工作已结项归档；遗留问题、候选清单、已确认实例和稳定报告位置见 [归档说明](../../docs/archives/2026-11-05-cppwin32/README.md)。保留本工具供参考和复现，不自动继续整改生成链路。

本工具将导出头文件与 `win32metadata/generation/WinSDK/RecompiledIdlHeaders` 分别解析为 Clang AST，查找对应声明并统计布局、签名、名称写法和常量值的差异。使用本机 ClangSharp 编译产物中的 `ClangSharp.Interop.dll`、`libclang.dll`、`libClangSharp.dll`；不修改 SDK、WinMD 或导出头文件。

## 运行

```powershell
& .\tools\SdkDeepAudit\Run.ps1 `
    -Headers Z:\ms `
    -MetadataRoot L:\Open_Code\win32metadata `
    -Output Z:\sdk-deep-audit-20261105 `
    -Architectures x86,x64,arm64 `
    -CharacterSets Ansi,Unicode `
    -Jobs 4
```

脚本使用 `devenv.com /Build "Release|Any CPU"` 构建提取器，并运行故障注入测试。生成物、缓存和日志均放在 `-Output` 目录，不使用仓库的 `test\obj`，也不生成另一份正式头文件。

已有构建产物时可加 `-UseBuiltExtractor`。`-ExtractOnly` 只提取 AST；`-CompareOnly` 复用现有快照重新比较；`-ReportOnly` 重新汇总 HTML。`-FailOnDifference` 使存在差异或未验证项时返回失败，默认会保留结果供审核。

直接运行 Python 时，`run.py --configuration x64-Unicode --compare-only` 可单独复查一组配置。Python 标准库之外不需要额外包。

## 检查内容

- 记录、结构体、联合体及接口：大小、对齐、基类名称和 UUID；字段的位偏移、位域宽度、数组维度、指针层数、const/volatile、原始类型写法及展开 typedef 后的类型结构。
- 普通函数与成员函数：返回值、参数个数、参数名、参数类型及大小、调用约定、可变参数、异常说明，以及直接声明的虚函数顺序。
- typedef 和类型选择宏：名称及指向的类型；同时保留原始声明片段，避免预处理展开掩盖 SDK 宏别名的写法。
- 枚举成员、变量和常量：类型与大小、可求值的完整常量值。字符串按字面量的实际字节序列比较，包含嵌入的零字符，不使用会截断宽字符串的零结尾字符串接口。
- 对宏转换来的常量，建立独立 C++ 求值单元，验证值和表达式类型大小。无法求值的宏、位置相关宏和多定义歧义不会算作通过。
- `IID_<名称>` 可对应 SDK 类型的 `__declspec(uuid("…"))`，检查全部 16 字节及 IID 对象大小；不要求 SDK 另有同名常量声明。
- `enums.json` 的 `uses`、`addUsesTo` 和结构体 tag/typedef 关系用于精确识别允许的枚举替换。只放过相应位置的类型名称变化，仍检查宽度、对齐、指针/数组结构和限定符。

SDK 按其分区 `main.cpp` 解析，读取分区的 `settings.rsp`、附加参数和包含目录；例如 Media Foundation、DirectShow 的 C++20 设置会保留。不同架构和字符集分别取样。快照保留完整编译参数、解析器版本、诊断和输入哈希。

同名类型不能只取第一个候选：UUID、声明位置、所属记录及重载签名用于关联，无法唯一确定时列入歧义项。匿名记录优先使用父记录和使用位置关联。

## 输出与解释

- `report.html`：配置汇总及可筛选的布局差异示例。
- `summary.json`：机器可读统计，明确包含 `all_symbols_verified`。
- `reports/<配置>/differences.csv`：每项差异的两边值、文件及行号。字段偏移和位域宽度以位为单位，类型大小与对齐以字节为单位。
- `reports/<配置>/findings.csv`：全部待查声明，包括未匹配、歧义、未求值和解析有误的项目。
- `reports/<配置>/symbols.jsonl.gz`：所有已提取声明的匹配和检查结果，含参数、源码片段与快照引用。
- `reports/<配置>/parse-errors.json`：所有错误级解析诊断。
- `cache`：Clang AST 提取事实；`indexes`：本机生成的 SDK 索引缓存；`manifests`：输入指纹及文件哈希。

“有差异”不等于“已确认的 ABI 缺陷”。报告将实际布局、类型结构/身份、名称写法、宏表达式大小、整数有符号表示和 C++ `noexcept` 分开统计。比如宏本身没有独立存储对象，其表达式类型大小与人为建立的枚举对象大小不同，需要结合配置审核。跨架构统计可能重复同一个逻辑符号。

C++ 引用还有两种不同的大小概念：语言中的 `sizeof(T&)` 等于 `sizeof(T)`，但引用参数、返回值及引用成员按目标 ABI 的指针宽度处理。工具会保留引用/指针的类型身份差异，不把这类语言层面的 `sizeof` 差异混入参数和成员存储宽度；typedef 声明自身的相关差异以 `cxx_sizeof_*` 另记。

未匹配、解析有误、同名歧义、不能求值、SDK 只有不完整声明等情况均单独保留。生成器自身的辅助模板/函数及托管元数据支持声明不冒充 SDK 声明；未实例化模板的通用布局不能确定，会作为支持项或未验证项记录。

这里查的是头文件中的声明和宏。函数机器码长度、链接后的地址、实际执行行为不由这些 AST 决定；本工具也不把相同大小自动视为相同声明。ARM64 的结果来自 Clang 交叉目标布局计算，不表示执行过 ARM64 二进制。

## 校验器自身的验证

`selftest.py` 通过同一个 AST 提取器注入宽度、packing、偏移、指针层数、调用约定、回调、数组、参数名、typedef/宏写法、虚函数顺序、IID 单字节错误、宽字符串和嵌入零字符等变化。另有正确对照、枚举允许规则、枚举改变宽度/constness、同名接口跨头文件冲突、重载、解析失败和含点分区缓存路径等测试，确保异常不会静默计为通过。
