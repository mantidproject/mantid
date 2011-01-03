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

BUILDPATH=../../Framework/release
THIRDPARTY=../../../Third_Party
PYTHONAPI=../../Framework/PythonAPI

# First copy in the MantidPlot executable bundle
cp -R $BUILDPATH/MantidPlot.app/Contents MantidPlot.app/

# Copy in the colormaps. Use rsync to exclude .svn directories.
rsync -aC ../colormaps MantidPlot.app/Contents/Resources/

# Copy in the instrument definition files
cp ../../Instrument/*.xml MantidPlot.app/instrument/

# Copy in the scripts (rsync excludes .svn directories)
rsync -aC ../../Scripts MantidPlot.app/scripts

# Now run the Qt macdeployqt tool to copy the Qt libraries and set the 
# executable to point to them
macdeployqt MantidPlot.app 

# Now the Mantid shared libraries
# First the core Mantid libraries
cp $BUILDPATH/libMantidKernel.dylib MantidPlot.app/Contents/MacOS/
cp $BUILDPATH/libMantidGeometry.dylib MantidPlot.app/Contents/MacOS/
cp $BUILDPATH/libMantidAPI.dylib MantidPlot.app/Contents/MacOS/
# Now our Qt stuff
cp $BUILDPATH/libMantidQtAPI.1.dylib MantidPlot.app/Contents/MacOS/
install_name_tool -change @executable_path/../Frameworks/libMantidQtAPI.1.dylib @executable_path/./libMantidQtAPI.1.dylib MantidPlot.app/Contents/MacOS/MantidPlot
cp $BUILDPATH/libMantidWidgets.1.dylib MantidPlot.app/Contents/MacOS/
install_name_tool -change libMantidQtAPI.1.dylib @executable_path/./libMantidQtAPI.1.dylib MantidPlot.app/Contents/MacOS/libMantidWidgets.1.dylib
install_name_tool -change @executable_path/../Frameworks/libMantidWidgets.1.dylib @executable_path/./libMantidWidgets.1.dylib MantidPlot.app/Contents/MacOS/MantidPlot
cp $BUILDPATH/libQtPropertyBrowser.1.dylib MantidPlot.app/Contents/Frameworks/
install_name_tool -change libQtPropertyBrowser.1.dylib @loader_path/../Frameworks/libQtPropertyBrowser.1.dylib MantidPlot.app/Contents/MacOS/libMantidQtAPI.1.dylib
install_name_tool -change libQtPropertyBrowser.1.dylib @loader_path/../Frameworks/libQtPropertyBrowser.1.dylib MantidPlot.app/Contents/MacOS/libMantidWidgets.1.dylib
install_name_tool -change libqwt.5.dylib @loader_path/../Frameworks/libqwt.5.dylib MantidPlot.app/Contents/MacOS/libMantidQtAPI.1.dylib
install_name_tool -change libqwt.5.dylib @loader_path/../Frameworks/libqwt.5.dylib MantidPlot.app/Contents/MacOS/libMantidWidgets.1.dylib

# Now lots of dependencies
# ...third party directory. Specific required versions only.
cp $THIRDPARTY/lib/mac/lib*.*.dylib MantidPlot.app/Contents/MacOS/
cp $THIRDPARTY/lib/mac/libboost* MantidPlot.app/Contents/MacOS/
# The NeXus library
cp /usr/local/lib/libNeXus.0.dylib MantidPlot.app/Contents/Frameworks/
# ...other stuff needed for qtiplot
cp $THIRDPARTY/lib/mac/libqwt.5.dylib MantidPlot.app/Contents/Frameworks/
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
cp $BUILDPATH/libMantidPythonAPI.so MantidPlot.app/Contents/MacOS/
cp $PYTHONAPI/MantidFramework.py MantidPlot.app/Contents/MacOS/
cp $PYTHONAPI/MantidStartup.py MantidPlot.app/Contents/MacOS/
cp $PYTHONAPI/setup.py MantidPlot.app/Contents/MacOS/
cp $PYTHONAPI/__init__.py MantidPlot.app/Contents/MacOS/
cp ../../MantidPlot/qtiplotrc.py MantidPlot.app/Contents/MacOS/
cp ../../MantidPlot/qtiUtil.py MantidPlot.app/Contents/MacOS/
cp ../../MantidPlot/mantidplotrc.py MantidPlot.app/Contents/MacOS/
cp ../../MantidPlot/mantidplot.py MantidPlot.app/Contents/MacOS/
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
cp $BUILDPATH/libMantidAlgorithms.dylib MantidPlot.app/plugins/
cp $BUILDPATH/libMantidCurveFitting.dylib MantidPlot.app/plugins/
cp $BUILDPATH/libMantidDataHandling.dylib MantidPlot.app/plugins/
cp $BUILDPATH/libMantidDataObjects.dylib MantidPlot.app/plugins/
cp $BUILDPATH/libMantidNexus.dylib MantidPlot.app/plugins/
cp $BUILDPATH/libMantidICat.dylib MantidPlot.app/plugins/
install_name_tool -change /usr/local/lib/libNeXus.0.dylib @loader_path/../Contents/Frameworks/libNeXus.0.dylib MantidPlot.app/plugins/libMantidNexus.dylib
# Mantid Qt plugins
cp $BUILDPATH/libMantidQtCustomDialogs.dylib MantidPlot.app/plugins/qtplugins/mantid
cp $BUILDPATH/libMantidQtCustomInterfaces.dylib MantidPlot.app/plugins/qtplugins/mantid
# Example Python algorithm
cp $PYTHONAPI/PythonAlgorithms/Examples/Squares.py MantidPlot.app/plugins/PythonAlgs/Examples/

PLUGINLIBS='MantidPlot.app/plugins/*.dylib'
update_lib_paths "$PLUGINLIBS" "$MACOSLIBS" "$QTLIBS" ".."
PLUGINSLIBS='MantidPlot.app/plugins/qtplugins/mantid/*.dylib'
update_lib_paths "$PLUGINSLIBS" "$MACOSLIBS" "$QTLIBS" "../../.."
install_name_tool -change libQtPropertyBrowser.1.dylib @loader_path/../../../Contents/Frameworks/libQtPropertyBrowser.1.dylib MantidPlot.app/plugins/qtplugins/mantid/libMantidQtCustomInterfaces.dylib
install_name_tool -change libQtPropertyBrowser.1.dylib @loader_path/../../../Contents/Frameworks/libQtPropertyBrowser.1.dylib MantidPlot.app/plugins/qtplugins/mantid/libMantidQtCustomDialogs.dylib
install_name_tool -change libqwt.5.dylib @loader_path/../../../Contents/Frameworks/libqwt.5.dylib MantidPlot.app/plugins/qtplugins/mantid/libMantidQtCustomInterfaces.dylib
install_name_tool -change libqwt.5.dylib @loader_path/../../../Contents/Frameworks/libqwt.5.dylib MantidPlot.app/plugins/qtplugins/mantid/libMantidQtCustomDialogs.dylib
