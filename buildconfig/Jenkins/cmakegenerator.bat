@echo off

:: SDK 8.1 is backwards compatible with Windows 7. It allows us to target Windows 7
:: when building on newer versions of Windows. This value must be supplied
:: externally and cannot be supplied in the cmake configuration
set SDK_VERS=8.1
set CM_GENERATOR=Visual Studio 16 2019
set CM_ARCH=x64
