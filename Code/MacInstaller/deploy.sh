#!/bin/sh
#
# Copies the all necessary files into appropriate places in the MantidPlot.app
# subdirectory that lies below this one. This directory is then the root
# when packagemaker is called using Mantid.pmdoc

# First copy in the MantidPlot executable bundle
cp -R ../qtiplot/qtiplot/MantidPlot.app/Contents MantidPlot.app/

# Copy in the colormaps. Use rsync to exclude .svn directories.
rsync -aC ../qtiplot/colormaps MantidPlot.app/Contents/Resources/

# Copy in the instrument definition files
cp ../../Test/Instrument/*.xml MantidPlot.app/instrument/

# Copy in the scripts (rsync excludes .svn directories)
rsync -aC ../Mantid/PythonAPI/scripts MantidPlot.app/

# Now run the Qt macdeployqt tool to copy the Qt libraries and set the 
# executable to point to them
macdeployqt MantidPlot.app 

# Now the Mantid shared libraries
# First the core Mantid libraries
cp ../Mantid/Bin/Shared/libMantidKernel.dylib MantidPlot.app/Contents/MacOS/
cp ../Mantid/Bin/Shared/libMantidGeometry.dylib MantidPlot.app/Contents/MacOS/
cp ../Mantid/Bin/Shared/libMantidAPI.dylib MantidPlot.app/Contents/MacOS/
# Now our Qt stuff
cp ../qtiplot/MantidQt/lib/libMantidQtAPI.1.dylib MantidPlot.app/Contents/MacOS/
install_name_tool -change @executable_path/../Frameworks/libMantidQtAPI.1.dylib @executable_path/./libMantidQtAPI.1.dylib MantidPlot.app/Contents/MacOS/MantidPlot
cp ../qtiplot/MantidQt/lib/libMantidWidgets.1.dylib MantidPlot.app/Contents/MacOS/
cp ../qtiplot/QtPropertyBrowser/lib/libQtPropertyBrowser.1.dylib MantidPlot.app/Contents/Frameworks/

# Now lots of dependencies
# ...third party directory. Specific required versions only.
cp ../Third_Party/lib/mac/lib*.*.dylib MantidPlot.app/Contents/MacOS/
cp ../Third_Party/lib/mac/libboost* MantidPlot.app/Contents/MacOS/
# The NeXus library
cp /usr/local/lib/libNeXus.0.dylib MantidPlot.app/Contents/Frameworks/
# ...other stuff needed for qtiplot
cp ../qtiplot/3rdparty/qwt/lib/libqwt.5.dylib MantidPlot.app/Contents/Frameworks/
cp /Library/Frameworks/libqscintilla2.5.dylib MantidPlot.app/Contents/Frameworks/

# Need to make sure all libraries point to local Qt, not system-wide one
cd MantidPlot.app/Contents/Frameworks/
QTLIBS=$(ls Qt*.framework/Versions/*/*)
DYLIBS=$(ls *.dylib)
for lib in $DYLIBS
do
  for qt in $QTLIBS
  do
    install_name_tool -change $qt @loader_path/./$qt $lib
  done
done
cd ../MacOS/
MACOSLIBS=$(ls *.dylib)
for lib2 in $MACOSLIBS
do
  for qt in $QTLIBS
  do
    install_name_tool -change $qt @loader_path/../Frameworks/$qt $lib2
  done
done
cd ../../..

# The stuff that needs to go in the executables directory
# ...the Mantid python library and associated files
cp ../Mantid/Bin/Shared/libMantidPythonAPI.so MantidPlot.app/Contents/MacOS/
cp ../Mantid/PythonAPI/MantidFramework.py MantidPlot.app/Contents/MacOS/
cp ../Mantid/PythonAPI/MantidHeader.py MantidPlot.app/Contents/MacOS/
cp ../Mantid/PythonAPI/MantidStartup.py MantidPlot.app/Contents/MacOS/
cp ../qtiplot/qtiplot/qtiplotrc.py MantidPlot.app/Contents/MacOS/
cp ../qtiplot/qtiplot/qtiUtil.py MantidPlot.app/Contents/MacOS/
cp ../qtiplot/qtiplot/mantidplotrc.py MantidPlot.app/Contents/MacOS/
cp ../qtiplot/qtiplot/mantidplot.py MantidPlot.app/Contents/MacOS/
# ...the sip and PyQt stuff
cp /Library/Python/2.5/site-packages/sip.so MantidPlot.app/Contents/MacOS/
mkdir MantidPlot.app/Contents/MacOS/PyQt4
cp /Library/Python/2.5/site-packages/PyQt4/Qt*.so MantidPlot.app/Contents/MacOS/PyQt4/
cp /Library/Python/2.5/site-packages/PyQt4/__init__.py MantidPlot.app/Contents/MacOS/PyQt4/

# Need to point PyQt libraries to local ones
cd MantidPlot.app/Contents/MacOS/PyQt4
PYQTLIBS=$(ls Qt*.so)
for so in $PYQTLIBS
do
  for qt in $QTLIBS
  do
    install_name_tool -change $qt @loader_path/../../Frameworks/$qt $so
  done
done
cd ../../../..

# Populate the plugins directory
cp ../Mantid/Bin/Shared/libMantidAlgorithms.dylib MantidPlot.app/plugins/
cp ../Mantid/Bin/Shared/libMantidCurveFitting.dylib MantidPlot.app/plugins/
cp ../Mantid/Bin/Shared/libMantidDataHandling.dylib MantidPlot.app/plugins/
cp ../Mantid/Bin/Shared/libMantidDataObjects.dylib MantidPlot.app/plugins/
cp ../Mantid/Bin/Shared/libMantidNexus.dylib MantidPlot.app/plugins/
install_name_tool -change /usr/local/lib/libNeXus.0.dylib @loader_path/../Contents/Frameworks/libNeXus.0.dylib MantidPlot.app/plugins/libMantidNexus.dylib
cp ../qtiplot/MantidQt/lib/libMantidQtCustomDialogs.dylib MantidPlot.app/plugins/
cp ../qtiplot/MantidQt/lib/libMantidQtCustomInterfaces.dylib MantidPlot.app/plugins/
cp ../Mantid/PythonAPI/PythonAlgorithms/Squares.py MantidPlot.app/plugins/PythonAlgs/Examples/

PLUGINLIBS='MantidPlot.app/plugins/*.dylib'
for lib in $PLUGINLIBS
do
  for m in $MACOSLIBS
  do
    install_name_tool -change @loader_path/./$m @loader_path/../Contents/MacOS/$m $lib
    install_name_tool -change $m @loader_path/../Contents/MacOS/$m $lib
  done
  for qt in $QTLIBS
  do
    install_name_tool -change $qt @loader_path/../Contents/Frameworks/$qt $lib
  done
done

