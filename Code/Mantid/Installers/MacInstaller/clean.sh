#!/bin/sh
#
# Cleans all the files out of the package directory, leaving the directory
# structure intact (except for Contents and the tree under it which is
# created when compiling qtiplot)
rm -R MantidPlot.app/Contents
rm MantidPlot.app/instrument/*
rm MantidPlot.app/plugins/*.*
rm MantidPlot.app/plugins/qtplugins/mantid/*.*
rm MantidPlot.app/plugins/PythonAlgs/Examples/*
rm -R MantidPlot.app/scripts/*