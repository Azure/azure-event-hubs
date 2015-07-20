
@setlocal EnableExtensions EnableDelayedExpansion
@echo off

set current-path=%~dp0
rem // remove trailing slash
set current-path=%current-path:~0,-1%

set build-root=%current-path%\..\..
rem // resolve to fully qualified path
for %%i in ("%build-root%") do set build-root=%%~fi

rem -----------------------------------------------------------------------------
rem -- build eventhub samples
rem -----------------------------------------------------------------------------

REM call %build-root%\samples\send\mbed\buildsample.bat
REM if not %errorlevel%==0 exit /b %errorlevel%

REM call %build-root%\samples\send_batch\mbed\buildsample.bat
REM if not %errorlevel%==0 exit /b %errorlevel%

goto :eof

