@echo off
svn update
python generateWxs.py
candle tmp.wxs  
light -out Mantid.msi tmp.wixobj "C:\Program Files\Windows Installer XML\bin\wixui.wixlib" -loc WixUI_en-us.wxl
if errorlevel 1 echo failed
