使用完整 WinMD 导出的头文件检查示例工程：

```powershell
& .\test\SampleD3DApp\Run.ps1
```

脚本使用 `devenv.com` 重新构建生成器，将当前 `Windows.Win32.winmd` 导出到 `Z:\ms`，再重新构建示例的 ANSI/UNICODE、Debug/Release、x64/x86 八种组合。可以通过 `-Winmd`、`-Output` 指定输入和输出，通过 `-Configurations`、`-CharacterSets` 指定构建矩阵。

构建会沿用工作区中的工程配置、源文件和预编译头。脚本通过编译器参数将本次导出目录放在包含路径前面，完成后恢复环境变量，并核对工程文件没有变化。它只编译、链接示例，不运行示例中的窗口操作。

日志和输入哈希写入 `test/SampleD3DApp/obj/full-sdk`。该检查验证工程实际包含的完整生成头文件，不替代单个头文件独立包含检查，也不验证图形界面运行效果。

不同 DLL 的同名 API 保持 C 声明：按生成顺序保留首次名称，后续名称增加 DLL 文件名主干前缀。当前 SDK 的 TPM 声明为 `tbs_GetDeviceID`，编译时会显示 `CPPWIN32001` 警告，并注明最先出现的 DirectSound 声明。
