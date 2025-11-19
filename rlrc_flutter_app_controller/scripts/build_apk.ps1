# ==============================================================================
# Flutter Android APK 构建向导 (PowerShell 版) - 修正版
# ==============================================================================

# --- 配置项 (唯一需要您修改的地方) ---
$outputDir = "D:\BaiduSyncdisk\软件\LightController"

# ==============================================================================
# --- 脚本辅助函数 (通常无需修改) ---
# ==============================================================================

function Show-Menu {
    param (
        [string]$Title,
        [System.Collections.IDictionary]$Options,
        [string]$DefaultKey
    )

    Write-Host "`n"
    Write-Host "--- $Title ---"

    foreach ($key in $Options.Keys) {
        $optionText = $Options[$key]
        if ($key -eq $DefaultKey) {
            Write-Host " [$key] $optionText (默认)"
        } else {
            Write-Host " [$key] $optionText"
        }
    }

    while ($true) {
        $input = Read-Host "--> 请输入您的选择 (直接回车使用默认值 '$DefaultKey')"
        if ([string]::IsNullOrWhiteSpace($input)) {
            return $DefaultKey
        }
        if ($Options.ContainsKey($input)) {
            return $input
        }
        Write-Host "[错误] 无效输入，请重新选择。"
    }
}

# ==============================================================================
# --- 脚本主流程 ---
# ==============================================================================

$ErrorActionPreference = "Stop"
Clear-Host
Write-Host "欢迎使用 Flutter APK 构建向导！"
Write-Host "----------------------------------"

if (-not (Test-Path -Path $outputDir -PathType Container)) {
    throw "[错误] 输出目录不存在！请检查脚本中的路径配置或手动创建该目录: '$outputDir'"
}

$pubspec = Get-Content .\pubspec.yaml
$appName = ($pubspec | Select-String -Pattern "^name:").ToString().Split(':')[1].Trim()
$version = ($pubspec | Select-String -Pattern "^version:").ToString().Split(':')[1].Trim()

# --- 新增功能：询问是否需要自动锁屏 ---
$lockScreenOptions = @{
    "n" = "否 (No)"
    "y" = "是 (Yes)"
}
$shouldLockScreen = Show-Menu -Title "构建成功后是否自动锁定屏幕？" -Options $lockScreenOptions -DefaultKey "n"
# ------------------------------------

$buildTypeOptions = @{
    "release" = "Release (用于发布的已签名、优化版)"
    "debug"   = "Debug (用于测试的未签名版)"
}
$buildType = Show-Menu -Title "请选择构建类型" -Options $buildTypeOptions -DefaultKey "release"

$archOptions = @{
    "arm64" = "arm64-v8a (推荐)"
    "arm32" = "armeabi-v7a (兼容旧设备)"
    "fat"   = "Fat APK (通用包，体积大)"
}
$archChoice = Show-Menu -Title "请选择 CPU 架构" -Options $archOptions -DefaultKey "arm64"

$flutterArchParam = ""
$archSuffix = ""
if ($archChoice -eq "arm64") {
    $flutterArchParam = "--target-platform android-arm64"
    $archSuffix = "arm64-v8a"
} elseif ($archChoice -eq "arm32") {
    $flutterArchParam = "--target-platform android-arm"
    $archSuffix = "armeabi-v7a"
} else {
    $archSuffix = "fat"
}

$fileName = "$appName-v$version-$archSuffix-$buildType.apk"
$outputPath = Join-Path -Path $outputDir -ChildPath $fileName

Write-Host "`n"
Write-Host "================== 构建配置清单 =================="
Write-Host "  应用名      : $appName"
Write-Host "  版本号      : $version"
Write-Host "  构建类型    : $buildType"
Write-Host "  CPU 架构    : $archSuffix"
Write-Host "  自动锁屏    : $($lockScreenOptions[$shouldLockScreen])" # 显示锁屏选项
Write-Host "  最终输出路径: $outputPath"
Write-Host "=================================================="

$confirmation = Read-Host "`n--> 确认以上信息并开始构建吗？(Y/N)"

if ($confirmation.ToUpper() -ne 'Y') {
    Write-Host "操作已取消。"
    exit
}

Write-Host "`n--> 开始构建 APK，请稍候..."
# 1. 执行 Flutter 构建命令 (不带 --output)
$build_command = "flutter build apk --$buildType $flutterArchParam"
Write-Host "执行命令: $build_command"
Invoke-Expression $build_command

Write-Host "[成功] Flutter 构建过程完成。"

# 2. 移动并重命名文件
# 确定 Flutter 默认生成的 APK 源文件路径
$sourceApkName = "app-$buildType.apk"
$sourceApkPath = ".\build\app\outputs\flutter-apk\$sourceApkName"

Write-Host "--> 正在将 APK 从 $sourceApkPath 移动到目标位置..."

# 检查源文件是否存在
if (Test-Path $sourceApkPath) {
    # 使用 Move-Item 命令来移动并重命名，-Force 参数会覆盖同名旧文件
    Move-Item -Path $sourceApkPath -Destination $outputPath -Force
    Write-Host "`n🎉 构建成功!"
    Write-Host "[OK] APK 已保存至: $outputPath"

    # --- 新增功能：根据选择执行锁屏 ---
    if ($shouldLockScreen -eq "y") {
        Write-Host "--> 3秒后将自动锁定屏幕..."
        Start-Sleep -Seconds 3
        # 调用 Windows 系统 DLL 来执行锁屏
        rundll32.exe user32.dll, LockWorkStation
    }
    # ------------------------------------

} else {
    # 如果源文件不存在，说明 Flutter 构建可能失败了
    throw "[错误] 未找到生成的 APK 文件！请检查上面的 Flutter 构建日志是否存在错误。"
}