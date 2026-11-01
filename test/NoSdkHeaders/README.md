# 不使用 Windows SDK 平台头的编译检查

项目使用 /X，显式加入生成的头文件、编译器标准库和 UCRT 运行库目录，清空普通与 external include 路径。通过 /showIncludes 拒绝 shared、um、winrt、cppwinrt 下的 Windows SDK 平台头。AllHeaders.cpp 还检查 SDK 配置头的 include guard 没有出现，NativeContractsHeader 可带入完整导出声明检查。

默认用例为 ANSI/UNICODE × x64/x86。配置用例检查 Windows 7 数值及符号版本设置、显式 WINVER/NTDDI_VERSION/_WIN32_IE 覆盖、六种应用族的分区真值和显式分区覆盖。配置真值测试在标准运行库初始化后设置应用族；完整生成声明在默认桌面配置下编译。

VersionConstants.cpp 的 119 个数值来自修复前已验收输出，先取消同名预处理宏，再断言原 C++ 常量仍存在且数值保持。更新这些数值需要核对 WinMD 和 SDK 来源，不能从新输出自动重建期望。

```powershell
& .\test\NoSdkHeaders\Run.ps1 -Headers Z:\ms `
  -NativeContracts Z:\sdk-independent-20261104\AcceptedContracts.h `
  -Logs Z:\sdk-independent-20261104\no-sdk-validation
python test/AuditSdkIndependence.py --headers Z:\ms\win32 `
  --output Z:\sdk-independent-20261104\include-audit.json
```

普通 C/C++ 标准库与运行库仍由编译器环境提供。测试不使用 Windows SDK 的 Win32 平台声明头，也不链接 Win32 导入库。
