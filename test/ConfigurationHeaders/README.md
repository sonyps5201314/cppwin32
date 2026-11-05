# SDK 配置头与版本选择编译检查

按 2026-11-05 的用户选择，base.h 直接包含 SDK 配置头的绝对路径。项目使用 /X，显式加入生成头、编译器标准库、UCRT 和这两个配置头所在的 shared 目录，清空普通与 external include 路径。通过 /showIncludes 只允许 sdkddkver.h、winapifamily.h 及其依赖 winpackagefamily.h；其他 SDK 平台头仍会被拒绝。AllHeaders.cpp 检查这三个配置头已经生效，NativeContractsHeader 可带入完整导出声明检查。

默认用例为 ANSI/UNICODE × x64/x86。配置用例检查 Windows 7 数值及符号版本设置、显式 WINVER/NTDDI_VERSION/_WIN32_IE 覆盖、六种应用族的分区真值和显式分区覆盖。包分区按 SDK 的 winpackagefamily.h 从服务器分区重新计算，普通桌面分区覆盖继续生效。配置真值测试在标准运行库初始化后设置应用族；完整生成声明在默认桌面配置下编译。

VersionConstants.cpp 核对 SDK 的版本宏及分区中保留的 MAXVER 投影常量。WINVER 等可配置值按 SDK 当前默认或用户覆盖处理；更新其他数值需要核对 WinMD 和 SDK 来源，不能从新输出自动重建期望。

```powershell
& .\test\ConfigurationHeaders\Run.ps1 -Headers Z:\ms `
  -NativeContracts Z:\sdk-config-absolute-20261105\AcceptedContracts.h `
  -Logs Z:\sdk-config-absolute-20261105\configuration-validation
python test/AuditSdkConfiguration.py --headers Z:\ms\win32 `
  --output Z:\sdk-config-absolute-20261105\include-audit.json
```

普通 C/C++ 标准库与运行库仍由编译器环境提供。测试只使用 SDK 配置头，不链接 Win32 导入库。
