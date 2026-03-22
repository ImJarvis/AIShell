# AI Accuracy & Profiling Report

| Status | Intent | Expected | Output | RAG (ms) | LLM (ms) |
|---|---|---|---|---|---|
| ❌ FAIL | Get-Process to list all processes using more than 1GB of memory | `Get-Process | Where-Object WS -gt 1GB` | `more +1024 "C:\Windows\System32\processlist.txt"` | 8.16 | 26623.63 |
| ✅ PASS | Stop-Process to forcefully terminate all instances of notepad | `Stop-Process -Name notepad -Force` | `Stop-Process -Name notepad -Force` | 2.10 | 8700.94 |
| ✅ PASS | Get-Service to find all services that are currently stopped | `Get-Service | Where-Object Status -eq 'Stopped'` | `Get-Service / Where-Object {$_.Status -eq "Stopped"}` | 0.62 | 25446.02 |
| ✅ PASS | Restart-Service to restart the print spooler service | `Restart-Service -Name Spooler` | `Restart-Service -Name Spooler` | 2.37 | 15108.93 |
| ✅ PASS | taskkill to kill process with PID 1234 forcefully | `taskkill /F /PID 1234` | `taskkill /PID 1234 /F` | 1.22 | 23939.70 |
| ✅ PASS | tasklist to show tasks loaded with a specific DLL like ntdll.dll | `tasklist /m ntdll.dll` | `tasklist /m ntdll.dll` | 2.77 | 20078.40 |
| ✅ PASS | Get-ChildItem to find all hidden files in the C drive recursively silently | `Get-ChildItem -Path C:\ -Hidden -Recurse -ErrorAction SilentlyContinue` | `Get-ChildItem -Path C:\ -Recurse -Hidden` | 1.16 | 30739.50 |
| ✅ PASS | Get-ChildItem to list the 10 largest files in the current directory | `Get-ChildItem | Sort-Object Length -Descending | Select-Object -First 10` | `Get-ChildItem -File / Sort-Object Length -Descending / Select-Object -First 10` | 0.79 | 20319.36 |
| ✅ PASS | robocopy to mirror directory C:\src to D:\dest | `robocopy C:\src D:\dest /MIR` | `robocopy C:\src D:\dest /MIR` | 2.48 | 28726.01 |
| ✅ PASS | icacls to grant user John full control over folder Data | `icacls Data /grant John:(OI)(CI)F` | `icacls "Data" /grant John:F` | 1.62 | 24891.81 |
| ✅ PASS | Get-Acl to view the access control list for secret.txt | `Get-Acl -Path secret.txt` | `Get-Acl -Path "C:\path\to\secret.txt"` | 0.86 | 29835.50 |
| ✅ PASS | Get-PhysicalDisk to check the health status of all physical disks | `Get-PhysicalDisk | Select-Object DeviceId, HealthStatus` | `Get-PhysicalDisk / Select-Object HealthStatus` | 2.60 | 11077.73 |
| ❌ FAIL | Optimize-Volume to defragment the C drive | `Optimize-Volume -DriveLetter C -Defrag` | `defrag C: /u` | 3.63 | 9494.65 |

## Summary
**Accuracy**: 84.62%
**Passed**: 11
**Failed**: 2
**Average RAG Time**: 2.3 ms
**Average LLM Time**: 21152.5 ms
