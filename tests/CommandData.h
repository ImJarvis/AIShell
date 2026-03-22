#pragma once
#include <vector>
#include <string>

struct TestCaseData {
    int id;
    std::string type;
    std::string command;
    std::string intent;
    std::string expected;
};

static const std::vector<TestCaseData> c_CommandDatabase = {
    // Basic Process & Service Management
    { 1, "PS", "Get-Process", "Get-Process to list all processes using more than 1GB of memory", "Get-Process | Where-Object WS -gt 1GB" },
    { 2, "PS", "Stop-Process", "Stop-Process to forcefully terminate all instances of notepad", "Stop-Process -Name notepad -Force" },
    { 3, "PS", "Get-Service", "Get-Service to find all services that are currently stopped", "Get-Service | Where-Object Status -eq 'Stopped'" },
    { 4, "PS", "Restart-Service", "Restart-Service to restart the print spooler service", "Restart-Service -Name Spooler" },
    { 5, "CMD", "taskkill", "taskkill to kill process with PID 1234 forcefully", "taskkill /F /PID 1234" },
    { 6, "CMD", "tasklist", "tasklist to show tasks loaded with a specific DLL like ntdll.dll", "tasklist /m ntdll.dll" },

    // Filesystem, Storage & Perms
    { 7, "PS", "Get-ChildItem", "Get-ChildItem to find all hidden files in the C drive recursively silently", "Get-ChildItem -Path C:\\ -Hidden -Recurse -ErrorAction SilentlyContinue" },
    { 8, "PS", "Get-ChildItem", "Get-ChildItem to list the 10 largest files in the current directory", "Get-ChildItem | Sort-Object Length -Descending | Select-Object -First 10" },
    { 9, "CMD", "robocopy", "robocopy to mirror directory C:\\src to D:\\dest", "robocopy C:\\src D:\\dest /MIR" },
    { 10, "CMD", "icacls", "icacls to grant user John full control over folder Data", "icacls Data /grant John:(OI)(CI)F" },
    { 11, "PS", "Get-Acl", "Get-Acl to view the access control list for secret.txt", "Get-Acl -Path secret.txt" },
    { 12, "PS", "Get-PhysicalDisk", "Get-PhysicalDisk to check the health status of all physical disks", "Get-PhysicalDisk | Select-Object DeviceId, HealthStatus" },
    { 13, "PS", "Optimize-Volume", "Optimize-Volume to defragment the C drive", "Optimize-Volume -DriveLetter C -Defrag" },

    // Networking
    /*
    { 14, "PS", "Test-NetConnection", "Test-NetConnection to check if port 443 is open on example.com", "Test-NetConnection -ComputerName example.com -Port 443" },
    { 15, "PS", "Resolve-DnsName", "Resolve-DnsName to perform a reverse DNS lookup for 8.8.8.8", "Resolve-DnsName -Name 8.8.8.8" },
    { 16, "CMD", "ipconfig", "ipconfig to flush the DNS resolver cache", "ipconfig /flushdns" },
    { 17, "CMD", "netstat", "netstat to show all active TCP connections with process IDs", "netstat -ano -p TCP" },
    { 18, "PS", "Get-NetAdapter", "Get-NetAdapter to list all physical network adapters", "Get-NetAdapter -Physical" },
    { 19, "CMD", "tracert", "tracert to trace the route to google.com with a max of 15 hops", "tracert -h 15 google.com" },
    { 20, "PS", "Get-NetIPAddress", "Get-NetIPAddress to show only IPv4 addresses on the system", "Get-NetIPAddress -AddressFamily IPv4" },

    // System OS & Event Logs
    { 21, "PS", "Get-CimInstance", "Get-CimInstance to get the serial number of the Windows machine", "Get-CimInstance -ClassName Win32_BIOS | Select-Object SerialNumber" },
    { 22, "PS", "Get-CimInstance", "Get-CimInstance to find the model of the computer motherboard", "Get-CimInstance -ClassName Win32_BaseBoard | Select-Object Product" },
    { 23, "CMD", "sfc", "sfc to scan and repair corrupted Windows system files", "sfc /scannow" },
    { 24, "CMD", "dism", "dism to restore Windows image health using Windows Update", "dism /online /cleanup-image /restorehealth" },
    { 25, "CMD", "systeminfo", "systeminfo to find the original install date of Windows", "systeminfo | findstr /B /C:\"Original Install Date\"" },
    { 26, "PS", "Get-WinEvent", "Get-WinEvent to fetch the last 20 system error events", "Get-WinEvent -FilterHashtable @{LogName='System'; Level=2} -MaxEvents 20" },
    { 27, "PS", "Clear-EventLog", "Clear-EventLog to clear the Application event log", "Clear-EventLog -LogName Application" },
    { 28, "CMD", "powercfg", "powercfg to generate a detailed battery report", "powercfg /batteryreport" },
    { 29, "CMD", "powercfg", "powercfg to disable system hibernation", "powercfg /h off" },

    // Registry
    { 30, "PS", "Get-ItemProperty", "Get-ItemProperty to read the Windows build number from the registry", "Get-ItemProperty -Path 'HKLM:\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion' -Name CurrentBuild" },
    { 31, "PS", "Set-ItemProperty", "Set-ItemProperty to enable Remote Desktop via registry", "Set-ItemProperty -Path 'HKLM:\\System\\CurrentControlSet\\Control\\Terminal Server' -Name 'fDenyTSConnections' -Value 0" },
    { 32, "CMD", "reg", "reg to export the complete HKEY_CURRENT_USER tree to backup.reg", "reg export HKCU backup.reg" },

    // Local Users & Groups
    { 33, "PS", "New-LocalUser", "New-LocalUser to create a new local user named Visitor without a password", "New-LocalUser -Name Visitor -NoPassword" },
    { 34, "PS", "Add-LocalGroupMember", "Add-LocalGroupMember to add user Visitor to the Guests group", "Add-LocalGroupMember -Group Guests -Member Visitor" },
    { 35, "CMD", "net", "net to change the password for user Admin to Secret123", "net user Admin Secret123" },
    { 36, "PS", "Enable-LocalUser", "Enable-LocalUser to enable the built-in Administrator account", "Enable-LocalUser -Name Administrator" },

    // Virtualization (Hyper-V) & Advanced Windows Administration
    { 37, "PS", "Get-VM", "Get-VM to list all Hyper-V virtual machines running on this host", "Get-VM | Where-Object State -eq 'Running'" },
    { 38, "PS", "Start-VM", "Start-VM to start a virtual machine named Ubuntu", "Start-VM -Name Ubuntu" },
    { 39, "PS", "Checkpoint-VM", "Checkpoint-VM to create a snapshot of VM Ubuntu named PreUpdate", "Checkpoint-VM -Name Ubuntu -SnapshotName PreUpdate" },
    
    // Active Directory (RSAT)
    { 40, "PS", "Get-ADUser", "Get-ADUser to find all Active Directory users whose password never expires", "Get-ADUser -Filter {PasswordNeverExpires -eq $true}" },
    { 41, "PS", "Unlock-ADAccount", "Unlock-ADAccount to unlock the Active Directory account for user jdoe", "Unlock-ADAccount -Identity jdoe" },
    
    // Utility & Scripting
    { 42, "PS", "Export-Csv", "Export-Csv to export the list of running services to services.csv", "Get-Service | Export-Csv -Path services.csv -NoTypeInformation" },
    { 43, "PS", "ConvertTo-Json", "ConvertTo-Json to convert process list to JSON format", "Get-Process | ConvertTo-Json" },
    { 44, "PS", "Invoke-Command", "Invoke-Command to run a script block to update group policy on remote Server1", "Invoke-Command -ComputerName Server1 -ScriptBlock {gpupdate /force}" },
    { 45, "PS", "Enter-PSSession", "Enter-PSSession to open an interactive remote PowerShell session to DC01", "Enter-PSSession -ComputerName DC01" },
    { 46, "PS", "Get-AppxPackage", "Get-AppxPackage to list all installed Windows Store apps for the current user", "Get-AppxPackage -User $env:USERNAME" },
    { 47, "PS", "Get-Clipboard", "Get-Clipboard to read the current text content from the clipboard", "Get-Clipboard" },
    { 48, "PS", "Set-Clipboard", "Set-Clipboard to copy the string Hello to the clipboard", "Set-Clipboard -Value 'Hello'" },
    { 49, "PS", "Resolve-Path", "Resolve-Path to expand the wildcard path C:\\Windows\\S* to absolute paths", "Resolve-Path C:\\Windows\\S*" },
    { 50, "PS", "Get-ExecutionPolicy", "Get-ExecutionPolicy to check the current PowerShell execution policy for the local machine", "Get-ExecutionPolicy -Scope LocalMachine" }
*/};
