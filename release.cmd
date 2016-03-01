git checkout master
if not %errorlevel%==0 exit /b %errorlevel%

git pull -t origin master 
if not %errorlevel%==0 exit /b %errorlevel%

git checkout develop 
if not %errorlevel%==0 exit /b %errorlevel%

git pull -t origin develop 
if not %errorlevel%==0 exit /b %errorlevel%

git tag -a %1 -m "Version released with tag %1"
if not %errorlevel%==0 exit /b %errorlevel%

git checkout master 
if not %errorlevel%==0 exit /b %errorlevel%

git merge --ff-only develop 
if not %errorlevel%==0 exit /b %errorlevel%

git push --tags origin master 
if not %errorlevel%==0 exit /b %errorlevel%
