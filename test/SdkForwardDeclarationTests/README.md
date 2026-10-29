# SDK 架构前置声明回归测试

先构建 cppwin32 并导出实际 SDK 头文件，再执行：

```powershell
& .\test\SdkForwardDeclarationTests\Run.ps1 -Headers 'Z:\TYPEDEF'
```

`Headers` 指向包含 `win32` 子目录的输出根。脚本使用 Visual Studio 18 的 devenv.com 重新构建并运行 x64、x86，生成的属性、日志和程序位于 obj，不提交。

两个平台都无条件包含 DeviceAndDriverInstallation.0.h，核对 27 个 PSP_* 别名以及 PINFCONTEXT 的指针类型，并确认配置管理器 PRIORITY 的限定类型为 uint32_t。原有的六组实际 SDK 接口对象别名身份断言也在两个平台执行。

蓝牙回调另核对 `PFN_DEVICE_CALLBACK` 的 BOOL 返回值、LPVOID 参数和 const BLUETOOTH_DEVICE_INFO* 参数。脚本同时检查头文件中的实际类型拼写，避免同义别名在 C++ 类型断言中等价而漏掉名称回退。

类型宏必须直接输出 `#define`，不能使用 `#ifndef` 屏蔽已有冲突定义。

三个蓝牙数据类型必须分别具有真实 `_BLUETOOTH_*` tag、`*_STRUCT` typedef 和 `#define BLUETOOTH_* ...`；不能由反向 typedef 代替宏。脚本从实际 complex_structs.h 抽取 SYSTEMTIME 及三个蓝牙结构的定义，单独编译并检查大小、偏移。全局 complex_structs.h 有既有的空命名空间 include 问题，因此该测试不宣称整份全 SDK 结构头独立编译通过。

旧生成器只在 PSP_* 别名前输出首个架构的 SP_* 声明，导致该完整头文件在 x86 编译失败。本测试必须包含实际的整份头文件，不能以摘取 typedef 或跳过 x86 代替。

对应的小型生成器用例在 ConstantProjectionTests 中：同名 PACKET 在 x64 为自然对齐的 16 字节结构，在 x86 为按 1 字节对齐的 8 字节结构；NativeTypedef、MetadataTypedef 和多层指针别名均须在结构定义前可用。

LDAP 检查从实际导出提取 INT typedef 和四个 ldap_sasl_bind 声明，结合真实 Ldap.0.h 编译返回类型及 int* MessageNumber 的断言；同时检查两个数字宏常量的 unsigned int 拼写。

AllJoyn 验证从实际 complex_interfaces.h 抽取 IUnknown、IInspectable、IWindowsDevicesAllJoynBusObjectInterop 及其真实依赖，编译 AddPropertyGetHandler/SetHandler 的成员函数指针断言，callback 必须是完整 stdcall 函数指针。
