REM  Microsoft Azure IoT Device Libraries
REM  Copyright (c) Microsoft Corporation
REM  All rights reserved. 
REM  MIT License
REM  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
REM  documentation files (the Software), to deal in the Software without restriction, including without limitation 
REM  the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
REM  and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

REM  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

REM  THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED 
REM  TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
REM  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
REM  CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
REM  IN THE SOFTWARE.

REM  Store the current path and change to the batch file directory
PUSHD %~dp0

REM  Limit the scope of environment variable changes to local only.
SETLOCAL

call ..\..\..\..\build_all\mbed\set_credentials.cmd
call mkmbedzip.bat

rmdir hg /s /q

mkdir hg
PUSHD hg

REM  Check if 7z is already in the system variable PATH
WHERE 7z.exe
IF %ERRORLEVEL% EQU 0 GOTO ZIP_UTIL_FOUND

REM  Try adding the default path to 7z if it's not already in the system variable PATH
PATH %ProgramFiles%\7-Zip;%PATH%
WHERE 7z.exe
IF %ERRORLEVEL% EQU 0 GOTO ZIP_UTIL_FOUND

REM  Abort execution if 7z was not found
@ECHO *******************************************
@ECHO ** ATTENTION:  7z utility was not found. **
@ECHO *******************************************
GOTO END

:ZIP_UTIL_FOUND

REM  Check if hg is already in the system variable PATH
WHERE hg.exe
IF %ERRORLEVEL% EQU 0 GOTO HG_FOUND

REM  Try adding the default path to hg if it's not already in the system variable PATH
PATH %ProgramFiles%\TortoiseHG;%PATH%
WHERE hg.exe
IF %ERRORLEVEL% EQU 0 GOTO HG_FOUND

REM  Abort execution if hg was not found
@ECHO *******************************************
@ECHO ** ATTENTION:  hg not found. **
@ECHO *******************************************
GOTO END

:HG_FOUND

set HGUSER=%MBED_HG_USER%

REM  Clone the repo
hg clone --insecure https://%MBED_USER%:%MBED_PWD%@developer.mbed.org/users/AzureIoTClient/code/eventhubclient_send_batch_sample_bld/

PUSHD eventhubclient_send_batch_sample_bld\send_batch
if not %errorlevel%==0 exit /b %errorlevel%

del /q *.*
if not %errorlevel%==0 exit /b %errorlevel%

call 7z x ..\..\..\send_batch.zip *.* -y
if not %errorlevel%==0 exit /b %errorlevel%

hg addremove
if not %errorlevel%==0 exit /b %errorlevel%

hg commit -m "Automatic build commit"

hg push --insecure https://%MBED_USER%:%MBED_PWD%@developer.mbed.org/users/AzureIoTClient/code/eventhubclient_send_batch_sample_bld/

POPD
POPD

SET COMPILEMBED_LOC=..\..\..\..\tools\compilembed
msbuild %COMPILEMBED_LOC%\compilembed.sln

%COMPILEMBED_LOC%\bin\debug\compilembed.exe -un %MBED_USER% -pwd %MBED_PWD% -r http://developer.mbed.org/users/AzureIoTClient/code/eventhubclient_send_batch_sample_bld/ -plat FRDM-K64F
if not %errorlevel%==0 exit /b %errorlevel%

:END

REM  Return to the directory we started in
POPD
