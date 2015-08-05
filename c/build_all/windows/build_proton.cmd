@setlocal EnableExtensions EnableDelayedExpansion
@echo off

set proton-repo=https://github.com/dcristoloveanu/qpid-proton.git
set proton-branch=0.9-IoTClient

rem -----------------------------------------------------------------------------
rem -- use PROTON_PATH if specified, otherwise <CurrentDrive>:\proton\qpid-proton
rem -----------------------------------------------------------------------------

set build-root=%~d0\proton\qpid-proton
if defined PROTON_PATH set build-root=%PROTON_PATH%\qpid-proton
echo Build root is %build-root%

rem -----------------------------------------------------------------------------
rem -- Make sure they know this is a third party component
rem -----------------------------------------------------------------------------

@Echo AzureIoT Eventhub Client has a dependency on apache qpid-proton-c
@Echo https://github.com/apache/qpid-proton/blob/master/LICENSE

set /P installComp=Do you want to install the component (y/n)? 
if "%installComp%"=="N" GOTO End
if "%installComp%"=="n" GOTO End


rem -----------------------------------------------------------------------------
rem -- sync the Proton source code
rem -----------------------------------------------------------------------------

rmdir %build-root% /s /q
git clone -b %proton-branch% %proton-repo% %build-root%

if not %errorlevel%==0 exit /b %errorlevel%

rem -----------------------------------------------------------------------------
rem -- build
rem -----------------------------------------------------------------------------

if not exist %build-root%\build (
    md %build-root%\build
    if not %errorlevel%==0 exit /b %errorlevel%
)

pushd %build-root%\build

cmake .. -G "Visual Studio 12"
if not %errorlevel%==0 exit /b %errorlevel%

msbuild proton.sln
if not %errorlevel%==0 exit /b %errorlevel%

:End
