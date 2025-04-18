using System;
using System.IO;
using System.Text.RegularExpressions;
using System.Collections.Generic;

class Program
{
    static void Main(string[] args)
    {
        try
        {
            // 输入和输出文件路径
            string inputFile = @"..\..\..\..\..\SampleD3DApp\SampleD3DApp\x64\Debug\SampleD3DApp.log";
            string outputFile = "undefs.h";
            string backupFile = "undefs.bak.h";

            // 检查输入文件是否存在
            if (!File.Exists(inputFile))
            {
                Console.WriteLine("输入文件不存在: " + inputFile);
                return;
            }

            // 备份现有的 undefs.h 文件（如果存在）
            if (File.Exists(outputFile))
            {
                File.Copy(outputFile, backupFile, true); // 覆盖旧的备份文件
                Console.WriteLine($"已备份 {outputFile} 到 {backupFile}");
            }

            // 读取输入文件的所有行
            string[] lines = File.ReadAllLines(inputFile);

            // 使用 HashSet 存储提取的关键字以去重
            var extractedMacros = new HashSet<string>();

            // 正则表达式，用于匹配以 "expanded from macro '<关键字>'" 结尾的行
            string pattern = @"expanded from macro '([^']+)'$";

            foreach (string line in lines)
            {
                // 去除行首尾的空白字符
                string trimmedLine = line.Trim();

                // 使用正则表达式匹配
                Match match = Regex.Match(trimmedLine, pattern);
                if (match.Success && match.Groups.Count > 1)
                {
                    // 提取关键字（位于捕获组 1 中）
                    string macro = match.Groups[1].Value;
                    if (!string.IsNullOrWhiteSpace(macro))
                    {
                        extractedMacros.Add(macro);
                    }
                }
            }

            // 准备输出内容，每行添加 "#undef " 前缀
            var outputLines = new List<string>();
            foreach (string macro in extractedMacros)
            {
                outputLines.Add($"#undef {macro}");
            }

            // 以追加模式写入输出文件
            File.AppendAllLines(outputFile, outputLines);

            Console.WriteLine($"成功追加 {extractedMacros.Count} 个唯一关键字到 {outputFile}");
        }
        catch (Exception ex)
        {
            Console.WriteLine("发生错误: " + ex.Message);
        }
    }
}