@echo off
if not exist bin\cygwin1.dll (
    echo Cygwin dll missing, copying
    xcopy C:\cygwin64\bin\cygwin1.dll bin\
)

set compiledir=C:\cygwin64\home\compilation\

mkdir %compiledir%
echo cd /home/compilation/ > %compiledir%\script.sh
echo mkdir bin >> %compiledir%\script.sh

for %%f in (*.c) do (
    xcopy /y %%f %compiledir%
    echo gcc %%f -o bin/%%~nf.exe >> %compiledir%\script.sh
)

for %%f in (*.h) do (
    xcopy /y %%f %compiledir%
)

C:\cygwin64\bin\bash --login -i -c "dos2unix /home/compilation/script.sh && sh /home/compilation/script.sh"

move %compiledir%\bin\* bin

rmdir /s /q %compiledir%
