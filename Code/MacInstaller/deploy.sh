#!/bin/sh
#
# Copies the all necessary files into appropriate places in the MantidPlot.app
# subdirectory that lies below this one. This directory is then the root
# when packagemaker is called using Mantid.pmdoc

# Ensures library paths point to the correct location
update_lib_paths() {
    pluginlibs=$1
    macoslibs=$2
    qtlibs=$3
    for lib in $pluginlibs
    do
      for m in $macoslibs
      do
        install_name_tool -change @loader_path/./$m @loader_path/$4/Contents/MacOS/$m $lib
        install_name_tool -change $m @loader_path/$4/Contents/MacOS/$m $lib
      done
      for qt in $qtlibs
      do
        install_name_tool -change $qt @loader_path/$4/Contents/Frameworks/$qt $lib
      done
    done
}

# First copy in the MantidPlot executable bundle
cp -R ../Mantid/release/MantidPlot.app/Contents MantidPlot.app/

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
cp ../Mantid/release/libMantidKernel.dylib MantidPlot.app/Contents/MacOS/
cp ../Mantid/release/libMantidGeometry.dylib MantidPlot.app/Contents/MacOS/
cp ../Mantid/release/libMantidAPI.dylib MantidPlot.app/Contents/MacOS/
# Now our Qt stuff
cp ../Mantid/release/libMantidQtAPI.1.dylib MantidPlot.app/Contents/MacOS/
install_name_tool -change @executable_path/../Frameworks/libMantidQtAPI.1.dylib @executable_path/./libMantidQtAPI.1.dylib MantidPlot.app/Contents/MacOS/MantidPlot
cp ../Mantid/release/libMantidWidgets.1.dylib MantidPlot.app/Contents/MacOS/
install_name_tool -change libMantidQtAPI.1.dylib @executable_path/./libMantidQtAPI.1.dylib MantidPlot.app/Contents/MacOS/libMantidWidgets.1.dylib
install_name_tool -change @executable_path/../Frameworks/libMantidWidgets.1.dylib @executable_path/./libMantidWidgets.1.dylib MantidPlot.app/Contents/MacOS/MantidPlot
cp ../Mantid/release/libQtPropertyBrowser.1.dylib MantidPlot.app/Contents/Frameworks/
install_name_tool -change libQtPropertyBrowser.1.dylib @loader_path/../Frameworks/libQtPropertyBrowser.1.dylib MantidPlot.app/Contents/MacOS/libMantidQtAPI.1.dylib

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
cp ../Mantid/release/libMantidPythonAPI.so MantidPlot.app/Contents/MacOS/
cp ../Mantid/PythonAPI/MantidFramework.py MantidPlot.app/Contents/MacOS/
cp ../Mantid/PythonAPI/MantidHeader.py MantidPlot.app/Contents/MacOS/
cp ../Mantid/PythonAPI/MantidStartup.py MantidPlot.app/Contents/MacOS/
cp ../qtiplot/qtiplot/qtiplotrc.py MantidPlot.app/Contents/MacOS/
cp ../qtiplot/qtiplot/qtiUtil.py MantidPlot.app/Contents/MacOS/
cp ../qtiplot/qtiplot/mantidplotrc.py MantidPlot.app/Contents/MacOS/
cp ../qtiplot/qtiplot/mantidplot.py MantidPlot.app/Contents/MacOS/
# ...the sip and PyQt stuff
cp /Library/Python/2.5/site-packages/sip.so MantidPlot.app/Contents/MacOS/
cp -R /Library/Python/2.5/site-packages/PyQt4 MantidPlot.app/Contents/MacOS/

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
cp ../Mantid/release/libMantidAlgorithms.dylib MantidPlot.app/plugins/
cp ../Mantid/release/libMantidCurveFitting.dylib MantidPlot.app/plugins/
cp ../Mantid/release/libMantidDataHandling.dylib MantidPlot.app/plugins/
cp ../Mantid/release/libMantidDataObjects.dylib MantidPlot.app/plugins/
cp ../Mantid/release/libMantidNexus.dylib MantidPlot.app/plugins/
cp ../Mantid/release/libMantidICat.dylib MantidPlot.app/plugins/
install_name_tool -change /usr/local/lib/libNeXus.0.dylib @loader_path/../Contents/Frameworks/libNeXus.0.dylib MantidPlot.app/plugins/libMantidNexus.dylib
# Mantid Qt plugins
cp ../Mantid/release/libMantidQtCustomDialogs.dylib MantidPlot.app/plugins/qtplugins/mantid
cp ../Mantid/release/libMantidQtCustomInterfaces.dylib MantidPlot.app/plugins/qtplugins/mantid
# Example Python algorithm
cp ../Mantid/PythonAPI/PythonAlgorithms/Examples/Squares.py MantidPlot.app/plugins/PythonAlgs/Examples/

PLUGINLIBS='MantidPlot.app/plugins/*.dylib'
update_lib_paths "$PLUGINLIBS" "$MACOSLIBS" "$QTLIBS" ".."
PLUGINSLIBS='MantidPlot.app/plugins/qtplugins/mantid/*.dylib'
update_lib_paths "$PLUGINSLIBS" "$MACOSLIBS" "$QTLIBS" "../../.."
