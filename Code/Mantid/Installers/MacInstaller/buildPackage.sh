#!/bin/sh
#
# Calls the scripts that set up the application in the correct directory
# structure under MantidPlot.app and then builds the Mac package from
# the Mantid.pmdoc template using packagemaker

echo Cleaning...
./clean.sh
echo Done.
echo Creating application structure...
./deploy.sh $1
echo Done.
echo Modifying properties file...
python adjustPropertiesFile.py
echo Done.
echo Building package...
/Developer/usr/bin/packagemaker --doc Mantid.pmdoc --scripts installer_hooks -i org.mantidproject.mantid.mantidplot.pkg
echo Done.
echo Creating disk image...
./dmgpack.sh Mantid mantid.pkg
echo Done.

echo Success!
