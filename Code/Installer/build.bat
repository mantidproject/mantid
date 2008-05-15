@echo off
svn update
candle Mantid.wxs  
light -out Mantid.msi Mantid.wixobj "C:\Program Files\Windows Installer XML\bin\wixui.wixlib" -loc WixUI_en-us.wxl
if errorlevel 1 echo failed
