接口 GUID 使用公开的 `inline constexpr IID IID_<名称>` 常量。前置声明和完整接口声明都保留元数据中的 `__declspec(uuid("…"))`，使仅包含 `.0.h` 时也能使用 `__uuidof`。`win32::guid_of<T>()` 通过编译器 UUID 读取原有值，不再依赖逐接口的 `guid_v` 模板特化。

通过 `AuditInterfaceGuids.py` 将修复前全部 `guid_v` 特化作为基准，逐项验证新 IID 的名称、作用域、头文件位置及全部 16 字节，以及前置声明和完整接口声明的 UUID。随后生成编译断言，为每个接口或 COM 类检查 IID 初始化值、`__uuidof(T)` 和 `guid_of<T>()` 一致。

`Forward.cpp` 只包含 `.0.h`，`Complete.cpp` 还包含完整接口定义；两者均在 ANSI/Unicode、x86/x64 编译。额外检查 `__uuidof(T*)`、表达式形式、接口 typedef 别名以及使用方自行声明 UUID 的类型。`-NegativeControl` 要求旧头文件因缺少 UUID 而编译失败。

```powershell
& .\test\InterfaceGuidTests\Run.ps1 `
    -Headers Z:\iid-uuid-20261105\headers `
    -Baseline Z:\ms `
    -Output Z:\iid-uuid-20261105\guid-tests `
    -NegativeControl
```

基准应使用迁移前的头文件，主输出更新后应以验收目录中的审计记录和生成的编译检查为准。输出、编译中间文件和日志均放入 `-Output` 指定的目录。
