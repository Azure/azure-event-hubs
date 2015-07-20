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

rem -----------------------------------------------------------------------------
rem -- parse script arguments
rem -----------------------------------------------------------------------------

rem // default build options
set build-clean=0
set build-config=Debug
set build-platform=Win32

:args-loop
if "%1" equ "" goto args-done
if "%1" equ "-c" goto arg-build-clean
if "%1" equ "--clean" goto arg-build-clean
if "%1" equ "--config" goto arg-build-config
if "%1" equ "--platform" goto arg-build-platform
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

:args-continue
shift
goto args-loop

:args-done

rem -----------------------------------------------------------------------------
rem -- clean solutions
rem -----------------------------------------------------------------------------

if %build-clean%==1 (
	call :clean-a-solution "%build-root%\common\build\windows\common.sln"
	if not %errorlevel%==0 exit /b %errorlevel%

	call :clean-a-solution "%build-root%\eventhub_client\build\windows\eventhubclient.sln"
	if not %errorlevel%==0 exit /b %errorlevel%
)

rem -----------------------------------------------------------------------------
rem -- build solutions
rem -----------------------------------------------------------------------------

call :build-a-solution "%build-root%\common\build\windows\common.sln"
if not %errorlevel%==0 exit /b %errorlevel%

call :build-a-solution "%build-root%\eventhub_client\build\windows\eventhubclient.sln"
if not %errorlevel%==0 exit /b %errorlevel%

rem -----------------------------------------------------------------------------
rem -- run unit tests
rem -----------------------------------------------------------------------------

call :run-unit-tests "common"
if not %errorlevel%==0 exit /b %errorlevel%

call :run-unit-tests "eventhub_client"
if not %errorlevel%==0 exit /b %errorlevel%

rem -----------------------------------------------------------------------------
rem -- run end-to-end tests
rem -----------------------------------------------------------------------------

call :run-e2e-tests "eventhub_client"
if not %errorlevel%==0 exit /b %errorlevel%

rem -----------------------------------------------------------------------------
rem -- done
rem -----------------------------------------------------------------------------

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


:run-unit-tests
call :_run-tests %1 "UnitTests"
goto :eof


:run-e2e-tests
call :_run-tests %1 "e2eTests"
goto :eof


:usage
echo build.cmd [options]
echo options:
echo  -c, --clean        delete artifacts from previous build before building
echo  --config ^<value^>   [Debug] build configuration (e.g. Debug, Release)
echo  --platform ^<value^> [Win32] build platform (e.g. Win32, x64, ...)
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


:_run-tests
rem // discover tests
set test-dlls-list=
set test-dlls-path=%build-root%\%~1\build\windows\%build-platform%\%build-config%
for /f %%i in ('dir /b %test-dlls-path%\*%~2*.dll') do set test-dlls-list="%test-dlls-path%\%%i" !test-dlls-list!

if "%test-dlls-list%" equ "" (
    echo No unit tests found in %test-dlls-path%
    exit /b 1
)

rem // run tests
echo Test DLLs: %test-dlls-list%
echo.
vstest.console.exe %test-dlls-list%
if not %errorlevel%==0 exit /b %errorlevel%
goto :eof
