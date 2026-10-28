# SDK 架构前置声明回归测试

先构建 cppwin32 并导出实际 SDK 头文件，再执行：

```powershell
& .\test\SdkForwardDeclarationTests\Run.ps1 -Headers 'Z:\TYPEDEF'
```

`Headers` 指向包含 `win32` 子目录的输出根。脚本使用 Visual Studio 18 的 devenv.com 重新构建并运行 x64、x86，生成的属性、日志和程序位于 obj，不提交。

两个平台都无条件包含 DeviceAndDriverInstallation.0.h，核对 27 个 PSP_* 别名以及 PINFCONTEXT 的指针类型，并确认配置管理器 PRIORITY 的限定类型为 uint32_t。原有的六组实际 SDK 接口对象别名身份断言也在两个平台执行。

旧生成器只在 PSP_* 别名前输出首个架构的 SP_* 声明，导致该完整头文件在 x86 编译失败。本测试必须包含实际的整份头文件，不能以摘取 typedef 或跳过 x86 代替。

对应的小型生成器用例在 ConstantProjectionTests 中：同名 PACKET 在 x64 为自然对齐的 16 字节结构，在 x86 为按 1 字节对齐的 8 字节结构；NativeTypedef、MetadataTypedef 和多层指针别名均须在结构定义前可用。
