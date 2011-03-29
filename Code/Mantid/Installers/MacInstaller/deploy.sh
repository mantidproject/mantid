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

if [ $# -ne 1 ]
then
  echo "Usage: deploy.sh [PATH_TO_BUILD_DIRECTORY]"
  exit
fi

# Build directory can now be anywhere - must be supplied as argument
BUILDPATH=$1
THIRDPARTY=../../../Third_Party/lib/mac64
PYTHONAPI=../../Framework/PythonAPI

# First copy in the MantidPlot executable bundle
cp -R $BUILDPATH/MantidPlot MantidPlot.app/Contents/MacOS/

# Copy in the colormaps. Use rsync to exclude .svn directories.
rsync -aC ../colormaps MantidPlot.app/Contents/Resources/

# Copy in the instrument definition files
cp ../../Instrument/*.xml MantidPlot.app/instrument/

# Copy in the scripts (rsync excludes .svn directories)
rsync -aC ../../Scripts/* MantidPlot.app/scripts
# Remove stuff that really doesn't belong in svn Scripts directory anyway
rm MantidPlot.app/scripts/CMakeLists.txt
rm -R MantidPlot.app/scripts/test

# Now run the Qt macdeployqt tool to copy the Qt libraries and set the 
# executable to point to them
macdeployqt MantidPlot.app 

# Now the Mantid shared libraries
# First the core Mantid libraries
cp $BUILDPATH/libMantidKernel.dylib MantidPlot.app/Contents/MacOS/
cp $BUILDPATH/libMantidGeometry.dylib MantidPlot.app/Contents/MacOS/
cp $BUILDPATH/libMantidAPI.dylib MantidPlot.app/Contents/MacOS/
# Now our Qt stuff
cp $BUILDPATH/libMantidQtAPI.dylib MantidPlot.app/Contents/MacOS/
install_name_tool -change @executable_path/../Frameworks/libMantidQtAPI.dylib @executable_path/./libMantidQtAPI.dylib MantidPlot.app/Contents/MacOS/MantidPlot
cp $BUILDPATH/libMantidWidgets.dylib MantidPlot.app/Contents/MacOS/
install_name_tool -change libMantidQtAPI.dylib @executable_path/./libMantidQtAPI.dylib MantidPlot.app/Contents/MacOS/libMantidWidgets.dylib
install_name_tool -change @executable_path/../Frameworks/libMantidWidgets.dylib @executable_path/./libMantidWidgets.dylib MantidPlot.app/Contents/MacOS/MantidPlot
cp $BUILDPATH/libQtPropertyBrowser.dylib MantidPlot.app/Contents/Frameworks/
install_name_tool -change libQtPropertyBrowser.dylib @loader_path/../Frameworks/libQtPropertyBrowser.dylib MantidPlot.app/Contents/MacOS/libMantidQtAPI.dylib
install_name_tool -change libQtPropertyBrowser.dylib @loader_path/../Frameworks/libQtPropertyBrowser.dylib MantidPlot.app/Contents/MacOS/libMantidWidgets.dylib
install_name_tool -change @loader_path/./libqwt.5.dylib @loader_path/../Frameworks/libqwt.5.dylib MantidPlot.app/Contents/MacOS/libMantidQtAPI.dylib
install_name_tool -change @loader_path/./libqwt.5.dylib @loader_path/../Frameworks/libqwt.5.dylib MantidPlot.app/Contents/MacOS/libMantidWidgets.dylib

# Now lots of dependencies
# ...third party directory. Specific required versions only.
cp $THIRDPARTY/lib*.*.dylib MantidPlot.app/Contents/MacOS/
cp $THIRDPARTY/libboost* MantidPlot.app/Contents/MacOS/
# Remove a couple that we don't want in the MacOS directory
rm MantidPlot.app/Contents/MacOS/libqwt.5.*
rm MantidPlot.app/Contents/MacOS/libqwtplot3d.0.*.dylib
rm MantidPlot.app/Contents/MacOS/libNeXus.0.dylib
# The NeXus library
cp /usr/local/lib/libNeXus.0.dylib MantidPlot.app/Contents/Frameworks/
# ...other stuff needed for qtiplot
cp $THIRDPARTY/libqwt.5.dylib MantidPlot.app/Contents/Frameworks/
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
cp /Library/Python/2.6/site-packages/sip.so MantidPlot.app/Contents/MacOS/
cp -R /Library/Python/2.6/site-packages/PyQt4 MantidPlot.app/Contents/MacOS/

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
rsync -aC $PYTHONAPI/PythonAlgorithms/* MantidPlot.app/plugins/PythonAlgs/

PLUGINLIBS='MantidPlot.app/plugins/*.dylib'
update_lib_paths "$PLUGINLIBS" "$MACOSLIBS" "$QTLIBS" ".."
PLUGINSLIBS='MantidPlot.app/plugins/qtplugins/mantid/*.dylib'
update_lib_paths "$PLUGINSLIBS" "$MACOSLIBS" "$QTLIBS" "../../.."
install_name_tool -change libQtPropertyBrowser.dylib @loader_path/../../../Contents/Frameworks/libQtPropertyBrowser.dylib MantidPlot.app/plugins/qtplugins/mantid/libMantidQtCustomInterfaces.dylib
# install_name_tool -change libQtPropertyBrowser.1.dylib @loader_path/../../../Contents/Frameworks/libQtPropertyBrowser.1.dylib MantidPlot.app/plugins/qtplugins/mantid/libMantidQtCustomDialogs.dylib
install_name_tool -change @loader_path/./libqwt.5.dylib @loader_path/../../../Contents/Frameworks/libqwt.5.dylib MantidPlot.app/plugins/qtplugins/mantid/libMantidQtCustomInterfaces.dylib
install_name_tool -change @loader_path/./libqwt.5.dylib @loader_path/../../../Contents/Frameworks/libqwt.5.dylib MantidPlot.app/plugins/qtplugins/mantid/libMantidQtCustomDialogs.dylib
install_name_tool -change libiomp5.dylib @loader_path/../../../Contents/Frameworks/libiomp5.dylib MantidPlot.app/plugins/qtplugins/mantid/libMantidQtCustomInterfaces.dylib
install_name_tool -change libiomp5.dylib @loader_path/../../../Contents/Frameworks/libiomp5.dylib MantidPlot.app/plugins/qtplugins/mantid/libMantidQtCustomDialogs.dylib

# Additions to fix up for cmake snow leopard build
install_name_tool -change @loader_path/./libqwt.5.dylib @loader_path/../Frameworks/libqwt.5.dylib MantidPlot.app/Contents/MacOS/MantidPlot
cp /opt/intel/lib/libiomp5.dylib MantidPlot.app/Contents/Frameworks/

cd MantidPlot.app/Contents/MacOS
MANTIDLIBS=$(ls libMantid*.*)
for l in $MANTIDLIBS
do
  echo $l
  for m in $MANTIDLIBS
  do
    install_name_tool -change $l @loader_path/./$l $m
    install_name_tool -change @executable_path/../Frameworks/$l @loader_path/./$l $m
  done
  install_name_tool -change @executable_path/../Frameworks/$l @loader_path/./$l MantidPlot
  install_name_tool -change libiomp5.dylib @loader_path/../Frameworks/libiomp5.dylib $l
done
install_name_tool -change libiomp5.dylib @loader_path/../Frameworks/libiomp5.dylib libMantidPythonAPI.so

cd ../../plugins
MTDPLUGINS=$(ls libMantid*.dylib)
for n in $MTDPLUGINS
do
  install_name_tool -change libMantidDataObjects.dylib @loader_path/./libMantidDataObjects.dylib $n
  install_name_tool -change libiomp5.dylib @loader_path/../Contents/Frameworks/libiomp5.dylib $n
done
