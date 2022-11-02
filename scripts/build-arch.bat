@echo off
setlocal enableDelayedExpansion

rem Run from Qt command prompt with working directory set to root of repo

set BUILD_CONFIG=%1
set ARCH=%2

rem Convert to lower case for windeployqt
if /I "%BUILD_CONFIG%"=="debug" (
    set BUILD_CONFIG=debug
    set WIX_MUMS=10
) else if /I "%BUILD_CONFIG%"=="release" (
    set BUILD_CONFIG=release
    set WIX_MUMS=10
)

if /I "%ARCH%" NEQ "x86" (
    if /I "%ARCH%" NEQ "x64" (
        if /I "%ARCH%" NEQ "ARM64" (
            echo Invalid build architecture - expected 'x86', 'x64', or 'ARM64'
            echo Usage: scripts\build-arch.bat ^(release^|debug^) ^(x86^|x64^|ARM64^)
            exit /b 1
        )
    )
)


rem Convert to lower case for windeployqt
cd third-party/signaling
if /I "%BUILD_CONFIG%"=="debug" (
    call build-msvc.bat Debug
    set BUILD_CONFIG=debug
) else if /I "%BUILD_CONFIG%"=="release" (
    call build-msvc.bat Release
    set BUILD_CONFIG=release
)
cd ../..


rmdir /Q /S .\app
mkdir app
cd app
cd ../
xcopy "third-party/signaling/rtsp/header" "app" /E/H/C/I
xcopy "third-party/moonlight/app" "app" /E/H/C/I
xcopy "custom" "app" /E/H/C/I/Y 




set BUILD_ROOT=%cd%\build
set SOURCE_ROOT=%cd%
set BUILD_FOLDER=%BUILD_ROOT%\build-%ARCH%-%BUILD_CONFIG%
set DEPLOY_FOLDER=%BUILD_ROOT%\deploy-%ARCH%-%BUILD_CONFIG%
set INSTALLER_FOLDER=%BUILD_ROOT%\installer-%ARCH%-%BUILD_CONFIG%
set SYMBOLS_FOLDER=%BUILD_ROOT%\symbols-%ARCH%-%BUILD_CONFIG%
set /p VERSION=<%SOURCE_ROOT%\app\version.txt

rem Use the correct VC tools for the specified architecture
if /I "%ARCH%" EQU "x64" (
    rem x64 is a special case that doesn't match %PROCESSOR_ARCHITECTURE%
    set VC_ARCH=AMD64
) else (
    set VC_ARCH=%ARCH%
)

rem If we're not building for the current platform, use the cross compiling toolchain
if /I "%VC_ARCH%" NEQ "%PROCESSOR_ARCHITECTURE%" (
    set VC_ARCH=%PROCESSOR_ARCHITECTURE%_%VC_ARCH%
)

rem Find Visual Studio and run vcvarsall.bat
set VSWHERE="%SOURCE_ROOT%\scripts\vswhere.exe"
for /f "usebackq delims=" %%i in (`%VSWHERE% -latest -property installationPath`) do (
    call "%%i\VC\Auxiliary\Build\vcvarsall.bat" %VC_ARCH%
)
if !ERRORLEVEL! NEQ 0 goto Error

rem Find VC redistributable DLLs
for /f "usebackq delims=" %%i in (`%VSWHERE% -latest -find VC\Redist\MSVC\*\%ARCH%\Microsoft.VC*.CRT`) do set VC_REDIST_DLL_PATH=%%i

echo Cleaning output directories
rmdir /s /q %DEPLOY_FOLDER%
rmdir /s /q %BUILD_FOLDER%
rmdir /s /q %INSTALLER_FOLDER%
rmdir /s /q %SYMBOLS_FOLDER%
mkdir %BUILD_ROOT%
mkdir %DEPLOY_FOLDER%
mkdir %BUILD_FOLDER%
mkdir %INSTALLER_FOLDER%
mkdir %SYMBOLS_FOLDER%

echo Configuring the project
pushd %BUILD_FOLDER%
qmake %SOURCE_ROOT%\moonlight-qt.pro
if !ERRORLEVEL! NEQ 0 goto Error
popd

echo Compiling Moonlight in %BUILD_CONFIG% configuration
pushd %BUILD_FOLDER%
%SOURCE_ROOT%\scripts\jom.exe %BUILD_CONFIG%
if !ERRORLEVEL! NEQ 0 goto Error
popd

echo Saving PDBs
for /r "%BUILD_FOLDER%" %%f in (*.pdb) do (
    copy "%%f" %SYMBOLS_FOLDER%
    if !ERRORLEVEL! NEQ 0 goto Error
)
copy %SOURCE_ROOT%\third-party\moonlight\libs\windows\lib\%ARCH%\*.pdb %SYMBOLS_FOLDER%
if !ERRORLEVEL! NEQ 0 goto Error
7z a %SYMBOLS_FOLDER%\MoonlightDebuggingSymbols-%ARCH%-%VERSION%.zip %SYMBOLS_FOLDER%\*.pdb
if !ERRORLEVEL! NEQ 0 goto Error

if "%ML_SYMBOL_STORE%" NEQ "" (
    echo Publishing PDBs to symbol store: %ML_SYMBOL_STORE%
    symstore add /f %SYMBOLS_FOLDER%\*.pdb /s %ML_SYMBOL_STORE% /t Moonlight
    if !ERRORLEVEL! NEQ 0 goto Error
) else (
    if "%MUST_DEPLOY_SYMBOLS%"=="1" (
        echo "A symbol server must be specified in ML_SYMBOL_STORE for signed release builds"
        exit /b 1
    )
)

echo Copying DLL dependencies
copy %SOURCE_ROOT%\third-party\moonlight\libs\windows\lib\%ARCH%\*.dll %DEPLOY_FOLDER%
copy \vcpkg\installed\x64-windows\tools\protobuf\*.dll %DEPLOY_FOLDER%
if !ERRORLEVEL! NEQ 0 goto Error

if /I "%BUILD_CONFIG%"=="debug" (
    copy \vcpkg\installed\x64-windows\debug\bin\*.dll %DEPLOY_FOLDER%
) else if /I "%BUILD_CONFIG%"=="release" (
    copy \vcpkg\installed\x64-windows\bin\*.dll %DEPLOY_FOLDER%
)

echo Copying AntiHooking.dll
copy %BUILD_FOLDER%\third-party\moonlight\AntiHooking\%BUILD_CONFIG%\AntiHooking.dll %DEPLOY_FOLDER%
if !ERRORLEVEL! NEQ 0 goto Error

echo Copying GC mapping list
copy %SOURCE_ROOT%\app\SDL_GameControllerDB\gamecontrollerdb.txt %DEPLOY_FOLDER%
if !ERRORLEVEL! NEQ 0 goto Error

echo Deploying Qt dependencies
windeployqt.exe --dir %DEPLOY_FOLDER% --%BUILD_CONFIG% --qmldir %SOURCE_ROOT%\app\gui --no-opengl-sw --no-compiler-runtime --no-qmltooling --no-virtualkeyboard --no-sql %BUILD_FOLDER%\app\%BUILD_CONFIG%\Moonlight.exe
if !ERRORLEVEL! NEQ 0 goto Error

echo Deleting unused styles
rem Qt 5.x directories
rmdir /s /q %DEPLOY_FOLDER%\QtQuick\Controls.2\Fusion
rmdir /s /q %DEPLOY_FOLDER%\QtQuick\Controls.2\Imagine
rmdir /s /q %DEPLOY_FOLDER%\QtQuick\Controls.2\Universal
rem Qt 6.x directories
rmdir /s /q %DEPLOY_FOLDER%\QtQuick\Controls\Fusion
rmdir /s /q %DEPLOY_FOLDER%\QtQuick\Controls\Imagine
rmdir /s /q %DEPLOY_FOLDER%\QtQuick\Controls\Universal

echo Generating QML cache
forfiles /p %DEPLOY_FOLDER% /m *.qml /s /c "cmd /c qmlcachegen.exe @path"
if !ERRORLEVEL! NEQ 0 goto Error

echo Deleting original QML files
forfiles /p %DEPLOY_FOLDER% /m *.qml /s /c "cmd /c del @path"
if !ERRORLEVEL! NEQ 0 goto Error

echo Copying application binary to deployment directory
copy %BUILD_FOLDER%\app\%BUILD_CONFIG%\Moonlight.exe %DEPLOY_FOLDER%
if !ERRORLEVEL! NEQ 0 goto Error

@REM rmdir /s /q %SOURCE_ROOT%\app
exit /b !ERRORLEVEL!

:Error
echo Build failed!
rmdir /s /q %SOURCE_ROOT%\app
exit /b !ERRORLEVEL!
