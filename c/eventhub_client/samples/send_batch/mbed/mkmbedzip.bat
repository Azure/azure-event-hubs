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


REM  Found 7z zip utility
:ZIP_UTIL_FOUND
SET outdir=mbed_out
REM  Delete any residual files from previous batch file runs
RMDIR /S/Q %outdir%
DEL /F /Q *.zip
REM  Create a directory for temporary file storage
MKDIR %outdir%
REM  Copy common code
COPY ..\..\..\..\common\src\*.c %outdir%
COPY ..\..\..\..\common\inc\*.h %outdir%
REM  Copy mbed-specific common code
COPY /Y ..\..\..\..\common\adapters\*mbed.c %outdir%
COPY /Y ..\..\..\..\common\adapters\*mbed.cpp %outdir%
REM  Copy EventHub-Client code
COPY ..\..\..\src\*.c %outdir%
COPY ..\..\..\inc\*.h %outdir%
REM  Copy sendtelemetryasync sample code
COPY .\main.cpp %outdir%
COPY ..\*.c %outdir%
COPY ..\*.h %outdir%
REM Insert version into source code
CALL insert_version.cmd %outdir%
REM  Delete all AMQP-related files
DEL /F /Q %outdir%\*amqp*.*

REM  Put all files into a zip file
PUSHD %outdir%
7z a -r ..\send_batch.zip *.*
POPD

RMDIR /S/Q %outdir%

:END

REM  Return to the directory we started in
POPD
