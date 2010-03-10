#!/bin/sh
#
# Calls the scripts that set up the application in the correct directory
# structure under MantidPlot.app and then builds the Mac package from
# the Mantid.pmdoc template using packagemaker

./clean.sh
./deploy.sh
python adjustPropertiesFile.py
/Developer/usr/bin/packagemaker --doc Mantid.pmdoc -i org.mantidproject.mantid.mantidplot.pkg
