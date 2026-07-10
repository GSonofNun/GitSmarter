# GitSmarter IPC Test Script
# Tests the named pipe IPC interface for screenshots and input injection

param(
    [string]$Command = "info",
    [int]$X = 0,
    [int]$Y = 0,
    [string]$Button = "left",
    [int]$VK = 0,
    [string]$Char = "",
    [string]$OutputFile = "screenshot.png"
)

$pipeName = "GitSmarterIPC"

function Send-IpcCommand {
    param([string]$JsonCommand)

    try {
        $pipe = New-Object System.IO.Pipes.NamedPipeClientStream(".", $pipeName, [System.IO.Pipes.PipeDirection]::InOut)
        $pipe.Connect(5000)

        $writer = New-Object System.IO.StreamWriter($pipe)
        $reader = New-Object System.IO.StreamReader($pipe)

        # Send command
        $writer.WriteLine($JsonCommand)
        $writer.Flush()

        # Read response
        $response = $reader.ReadLine()

        # For screenshots, also read binary data
        if ($JsonCommand -match '"type":\s*"screenshot"') {
            $responseObj = $response | ConvertFrom-Json
            if ($responseObj.success -and $responseObj.data_size -gt 0) {
                $binaryData = New-Object byte[] $responseObj.data_size
                $totalRead = 0
                while ($totalRead -lt $responseObj.data_size) {
                    $read = $pipe.Read($binaryData, $totalRead, $responseObj.data_size - $totalRead)
                    if ($read -eq 0) { break }
                    $totalRead += $read
                }
                [System.IO.File]::WriteAllBytes($OutputFile, $binaryData)
                Write-Host "Screenshot saved to: $OutputFile ($totalRead bytes)"
            }
        }

        $pipe.Close()
        return $response
    }
    catch {
        Write-Host "Error: $_"
        return $null
    }
}

switch ($Command) {
    "info" {
        $json = '{"type":"get_info"}'
        $response = Send-IpcCommand $json
        if ($response) {
            $obj = $response | ConvertFrom-Json
            Write-Host "Window Info:"
            Write-Host "  Size: $($obj.width) x $($obj.height)"
            Write-Host "  Title: $($obj.title)"
            Write-Host "  Focused: $($obj.focused)"
            Write-Host "  Visible: $($obj.visible)"
        }
    }
    "screenshot" {
        $json = '{"type":"screenshot","format":"png"}'
        $response = Send-IpcCommand $json
        if ($response) {
            Write-Host "Response: $response"
        }
    }
    "click" {
        $json = "{`"type`":`"mouse_click`",`"x`":$X,`"y`":$Y,`"button`":`"$Button`"}"
        $response = Send-IpcCommand $json
        Write-Host "Response: $response"
    }
    "move" {
        $json = "{`"type`":`"mouse_move`",`"x`":$X,`"y`":$Y}"
        $response = Send-IpcCommand $json
        Write-Host "Response: $response"
    }
    "key" {
        $json = "{`"type`":`"key_down`",`"vk`":$VK,`"ctrl`":false,`"shift`":false,`"alt`":false}"
        $response = Send-IpcCommand $json
        Write-Host "Response: $response"
    }
    "char" {
        $json = "{`"type`":`"char`",`"char`":`"$Char`"}"
        $response = Send-IpcCommand $json
        Write-Host "Response: $response"
    }
    default {
        Write-Host "Usage: test_ipc.ps1 -Command <info|screenshot|click|move|key|char> [options]"
        Write-Host ""
        Write-Host "Commands:"
        Write-Host "  info        - Get window information"
        Write-Host "  screenshot  - Capture screenshot (saves to -OutputFile)"
        Write-Host "  click       - Send mouse click at -X,-Y with -Button"
        Write-Host "  move        - Move mouse to -X,-Y"
        Write-Host "  key         - Press key with virtual key code -VK"
        Write-Host "  char        - Type character -Char"
        Write-Host ""
        Write-Host "Examples:"
        Write-Host "  .\test_ipc.ps1 -Command info"
        Write-Host "  .\test_ipc.ps1 -Command screenshot -OutputFile capture.png"
        Write-Host "  .\test_ipc.ps1 -Command click -X 100 -Y 200 -Button left"
        Write-Host "  .\test_ipc.ps1 -Command key -VK 13  # Enter key"
        Write-Host "  .\test_ipc.ps1 -Command char -Char A"
    }
}
