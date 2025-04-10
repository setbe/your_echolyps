@echo off
setlocal

:: === Configuration ===
:: Path to the compiled executable
set SHADER_EMBEDDER=vs\build\shader2header.exe

:: Optionally set Vulkan SDK path if not set globally
:: set VULKAN_SDK=C:\VulkanSDK\1.3.250.0

:: === Execution ===
echo [INFO] Running: %SHADER_EMBEDDER%
pushd %~dp0
%SHADER_EMBEDDER%
if %ERRORLEVEL% neq 0 (
    echo [ERROR] shader_embedder.exe failed
    popd
    pause
    exit /b 1
)
popd
echo [OK] Shader embedding completed successfully
pause
