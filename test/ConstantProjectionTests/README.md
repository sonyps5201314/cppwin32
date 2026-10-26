# typedef 字面常量投影回归测试

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
- HRESULT、HTREEITEM 和整数资源标识符的已有全局枚举接口保持兼容。

算术 typedef 判定沿唯一实例 Value 字段解析，排除指针、数组、非 typedef 和循环。HRESULT、NTSTATUS、BOOL、VARIANT_BOOL、BOOLEAN、DEVPROP_BOOLEAN、HTREEITEM、SOCKET、WSAEVENT 保持历史枚举策略；这些类型在 LASTOK 中已经使用该接口，不属于此次新增 typedef 造成的回退。

生成文件、winmd、日志和可执行文件均位于测试目录的 obj 子目录，不提交。
