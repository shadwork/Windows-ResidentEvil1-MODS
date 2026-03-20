@echo off
set PATH=C:\Program Files (x86)\Microsoft Visual Studio\Installer;%PATH%
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\Common7\Tools\VsDevCmd.bat" -arch=x86
cd /d d:\Work\biorand\mod_doorskip
msbuild mod-doorskip.vcxproj /p:Configuration=Release /p:Platform=Win32 /m /t:Rebuild
