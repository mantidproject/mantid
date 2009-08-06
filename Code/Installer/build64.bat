@echo off
svn update > build_number.txt
python generateWxs64.py
if errorlevel 1 goto failed
candle tmp.wxs  
if errorlevel 1 goto failed
light -out Mantid64.msi tmp.wixobj "C:\Program Files (x86)\Windows Installer XML\bin"\wixui.wixlib -loc WixUI_en-us.wxl
if errorlevel 1 goto failed
python msiSize.py
exit(0)
:failed
echo failed