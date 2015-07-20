@setlocal
@echo off

rem -----------------------------------------------------------
rem  This script takes one argument, which is the directory
rem  containing the version.c file where the version string
rem  will be inserted. If no argument is given, the current
rem  directory will be used.
rem -----------------------------------------------------------
set src_dir=%cd%
if not "%1"=="" set src_dir=%~f1

rem -----------------------------------------------------------
rem  Make sure Git is in our PATH
rem -----------------------------------------------------------
where /q git
if not %errorlevel%==0 goto :eof

rem -----------------------------------------------------------
rem  Make sure we're in a Git repository
rem -----------------------------------------------------------
for /f %%i in ('git rev-parse --is-inside-work-tree') do set is_inside_work_tree=%%i
if "%is_inside_work_tree%"=="false" goto :eof

rem -----------------------------------------------------------
rem  Query Git for the version string
rem -----------------------------------------------------------
for /f %%i in ('git describe --always --dirty') do set "version_string=%%i"

rem -----------------------------------------------------------
rem  Write the version string into an existing source file
rem -----------------------------------------------------------
set version_file=%src_dir%\version.c
ren "%version_file%" version.c.old
for /f "usebackq tokens=*" %%i in ("%version_file%.old") do (
  if "%%i"=="#define EHC_VERSION unknown" (
    echo #define EHC_VERSION %version_string% >> %version_file%
  ) else (
    echo %%i >> %version_file%
  )
)

del /q %version_file%.old
