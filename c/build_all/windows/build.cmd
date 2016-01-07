@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

@setlocal EnableExtensions EnableDelayedExpansion
@echo off

set current-path=%~dp0
rem // remove trailing slash
set current-path=%current-path:~0,-1%

set build-root=%current-path%\..\..
rem // resolve to fully qualified path
for %%i in ("%build-root%") do set build-root=%%~fi

rem -----------------------------------------------------------------------------
rem -- check prerequisites
rem -----------------------------------------------------------------------------

rem // some of our projects expect PROTON_PATH to be defined
if not defined PROTON_PATH (
    set PROTON_PATH=%~d0\proton
)

if not exist %PROTON_PATH% (
    echo ERROR: PROTON_PATH must point to the root of your QPID Proton installation, but
    echo "%PROTON_PATH%" does not exist. Exiting...
    exit /b 1
)

rem ensure nuget.exe exists
where /q nuget.exe
if not !errorlevel! == 0 (
@Echo Azure EventHubs SDK needs to download nuget.exe from https://www.nuget.org/nuget.exe 
@Echo https://www.nuget.org 
choice /C yn /M "Do you want to download and run nuget.exe?" 
if not !errorlevel!==1 goto :eof
rem if nuget.exe is not found, then ask user
Powershell.exe wget -outf nuget.exe https://nuget.org/nuget.exe
	if not exist .\nuget.exe (
		echo nuget does not exist
		exit /b 1
	)
)
rem -----------------------------------------------------------------------------
rem -- parse script arguments
rem -----------------------------------------------------------------------------

rem // default build options
set build-clean=0
set build-config=Debug
set build-platform=Win32
set CMAKE_run_e2e_tests=OFF
set CMAKE_run_longhaul_tests=OFF
set CMAKE_skip_unittests=OFF

:args-loop
if "%1" equ "" goto args-done
if "%1" equ "-c" goto arg-build-clean
if "%1" equ "--clean" goto arg-build-clean
if "%1" equ "--config" goto arg-build-config
if "%1" equ "--platform" goto arg-build-platform
if "%1" equ "--run-e2e-tests" goto arg-run-e2e-tests
if "%1" equ "--run-longhaul-tests" goto arg-longhaul-tests
if "%1" equ "--skip-unittests" goto arg-skip-unittests
call :usage && exit /b 1

:arg-build-clean
set build-clean=1
goto args-continue

:arg-build-config
shift
if "%1" equ "" call :usage && exit /b 1
set build-config=%1
goto args-continue

:arg-build-platform
shift
if "%1" equ "" call :usage && exit /b 1
set build-platform=%1
goto args-continue

:arg-run-e2e-tests
set CMAKE_run_e2e_tests=ON
goto args-continue

:arg-longhaul-tests
set CMAKE_run_longhaul_tests=ON
goto args-continue

:arg-skip-unittests
set CMAKE_skip_unittests=ON
goto args-continue

:args-continue
shift
goto args-loop

:args-done

rem -----------------------------------------------------------------------------
rem -- restore packages for solutions
rem -----------------------------------------------------------------------------

rem -----------------------------------------------------------------------------
rem -- clean solutions
rem -----------------------------------------------------------------------------

rem -----------------------------------------------------------------------------
rem -- build solutions
rem -----------------------------------------------------------------------------

rem -----------------------------------------------------------------------------
rem -- build with CMAKE and run tests
rem -----------------------------------------------------------------------------

rmdir /s/q %USERPROFILE%\cmake
rem no error checking

mkdir %USERPROFILE%\cmake
rem no error checking

pushd %USERPROFILE%\cmake
cmake -Drun_longhaul_tests:BOOL=%CMAKE_run_longhaul_tests% -Drun_e2e_tests:BOOL=%CMAKE_run_e2e_tests% %build-root% -Dskip_unittests:BOOL=%CMAKE_skip_unittests% %build-root%
if not %errorlevel%==0 exit /b %errorlevel%

msbuild /m eventhub_client.sln
if not %errorlevel%==0 exit /b %errorlevel%

ctest -C "debug" -V
if not %errorlevel%==0 exit /b %errorlevel%

popd
goto :eof


rem -----------------------------------------------------------------------------
rem -- subroutines
rem -----------------------------------------------------------------------------

:clean-a-solution
call :_run-msbuild "Clean" %1 %2 %3
goto :eof


:build-a-solution
call :_run-msbuild "Build" %1 %2 %3
goto :eof

:usage
echo build.cmd [options]
echo options:
echo  -c, --clean           delete artifacts from previous build before building
echo  --config ^<value^>      [Debug] build configuration (e.g. Debug, Release)
echo  --platform ^<value^>    [Win32] build platform (e.g. Win32, x64, ...)
echo  --run-e2e-tests       run end-to-end tests
echo  --run-longhaul-tests  run long-haul tests
goto :eof


rem -----------------------------------------------------------------------------
rem -- helper subroutines
rem -----------------------------------------------------------------------------

:_run-msbuild
rem // optionally override configuration|platform
setlocal EnableExtensions
set build-target=
if "%~1" neq "Build" set "build-target=/t:%~1"
if "%~3" neq "" set build-config=%~3
if "%~4" neq "" set build-platform=%~4

msbuild /m %build-target% "/p:Configuration=%build-config%;Platform=%build-platform%" %2
if not %errorlevel%==0 exit /b %errorlevel%
goto :eof