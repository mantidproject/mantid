@echo off
svn update > build_number.txt
python generateWxs.py
if errorlevel 1 goto failed
candle tmp.wxs  
if errorlevel 1 goto failed
light -out Mantid.msi tmp.wixobj "C:\Program Files\Windows Installer XML\bin\wixui.wixlib" -loc WixUI_en-us.wxl
if errorlevel 1 goto failed
exit(0)
:failed
echo failed