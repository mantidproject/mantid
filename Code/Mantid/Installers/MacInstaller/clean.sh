#!/bin/sh
#
# Cleans all the files out of the package directory, leaving the directory
# structure intact (except for Contents and the tree under it which is
# created when compiling qtiplot)
rm -Rf MantidPlot.app/Contents/MacOS/*
rm -Rf MantidPlot.app/Contents/Frameworks/*
rm -Rf MantidPlot.app/Contents/Frameworks
rm -Rf MantidPlot.app/Contents/Resources/colormaps
rm -Rf MantidPlot.app/Contents/Resources/qt.conf
rm MantidPlot.app/instrument/*
rm MantidPlot.app/plugins/*.*
rm MantidPlot.app/plugins/qtplugins/mantid/*.*
rm -Rf MantidPlot.app/plugins/PythonAlgs/*
rm -R MantidPlot.app/scripts/*
