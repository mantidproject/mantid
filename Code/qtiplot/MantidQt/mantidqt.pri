#------------------------------
# This sets some variables that
# are used in the sub-projects
#-----------------------------

#------------------------
# Compile mode
#------------------------
CONFIG += qt warn_on exceptions

# Release mode
CONFIG += release

# Debug mode
#CONFIG += debug

win32:DEFINES += QT_DLL QT_THREAD_SUPPORT  _WINDOWS WIN32

#-------------------------------
# Paths, libraries and resources
#-------------------------------
unix {
   CWD = $$system(pwd)
}
win32 {
   CWD = $$system(cd)
}

TOPBUILDDIR = $$CWD

# Icons
RESOURCES = "$$TOPBUILDDIR/../../../Images/images.qrc"

# My variables
MANTIDPATH = $$TOPBUILDDIR/../../Mantid
MANTIDLIBPATH = $$MANTIDPATH/Bin/Shared
THIRDPARTY = $$TOPBUILDDIR/../../Third_Party

unix:MANTIDQTINCLUDES = $$TOPBUILDDIR/includes
win32:MANTIDQTINCLUDES = $$TOPBUILDDIR\includes

TMPDIR = $$TOPBUILDDIR/qtbuild/MantidQt

# Qt qmake variables

INCLUDEPATH += "$$MANTIDPATH/includes"
INCLUDEPATH += "$$MANTIDQTINCLUDES"

unix {
  LIBS += -L$$MANTIDLIBPATH -lMantidKernel
  LIBS += -L$$MANTIDLIBPATH -lMantidAPI
}
win32 {
  INCLUDEPATH += "$$THIRDPARTY/include"
  LIBS += "$$MANTIDLIBPATH/MantidKernel.lib"
  LIBS += "$$MANTIDLIBPATH/MantidGeometry.lib"
  LIBS += "$$MANTIDLIBPATH/MantidAPI.lib"
  LIBS += "$$THIRDPARTY/lib/win32/PocoFoundation.lib"
}

MOC_DIR = "$$TMPDIR"
OBJECTS_DIR = "$$TMPDIR"
SIP_DIR = "$$TMPDIR"

DESTDIR = "$$TOPBUILDDIR/lib"


