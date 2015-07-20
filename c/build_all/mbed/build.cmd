
@setlocal EnableExtensions EnableDelayedExpansion
@echo off

set current-path=%~dp0
rem // remove trailing slash
set current-path=%current-path:~0,-1%

set repo-build-root=%current-path%\..\..
rem // resolve to fully qualified path
for %%i in ("%repo-build-root%") do set repo-build-root=%%~fi

rem -----------------------------------------------------------------------------
rem -- build eventhub client samples
rem -----------------------------------------------------------------------------

call %repo-build-root%\eventhub_client\build\mbed\build.cmd
REM if not %errorlevel%==0 exit /b %errorlevel%

goto :eof

