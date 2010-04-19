#------------------------------
# This sets some variables that
# are used in the sub-projects
#-----------------------------

#------------------------
# Compile mode
#------------------------
CONFIG += qt warn_on exceptions

QMAKESPEC=win32-msvc2005

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
MANTIDPATH = "$$TOPBUILDDIR/../../Mantid"
MANTIDLIBPATH = "$$MANTIDPATH/Bin/Shared"
win32 {
  LIBPATH += $$MANTIDLIBPATH
  build_pass:CONFIG(release, debug|release) {
    # Put both Scons and Visual Studio output directories on the search path
    LIBPATH += "$$MANTIDPATH/release"
  }
  build_pass:CONFIG(debug, debug|release) {
    MANTIDLIBPATH = "$$MANTIDPATH/debug"
    LIBPATH += $$MANTIDLIBPATH
  }
}

THIRDPARTY = "$$TOPBUILDDIR/../../Third_Party"

unix:MANTIDQTINCLUDES = $$TOPBUILDDIR/includes
win32:MANTIDQTINCLUDES = "$$TOPBUILDDIR\includes"

TMPDIR = $$TOPBUILDDIR/qtbuild/MantidQt

# Qt qmake variables

DEPENDPATH += "$$MANTIDPATH/Kernel/inc/"
DEPENDPATH += "$$MANTIDPATH/Geometry/inc/"
DEPENDPATH += "$$MANTIDPATH/API/inc/"
INCLUDEPATH += "$$MANTIDPATH/Kernel/inc/"
INCLUDEPATH += "$$MANTIDPATH/Geometry/inc/"
INCLUDEPATH += "$$MANTIDPATH/API/inc/"
INCLUDEPATH += "$$MANTIDQTINCLUDES"

unix {
  LIBS += -L$$MANTIDLIBPATH -lMantidKernel
  LIBS += -L$$MANTIDLIBPATH -lMantidAPI
  LIBS += -L$$MANTIDLIBPATH -lMantidGeometry

  CONFIG(debug, debug|release) {
  LIBS	+= -lPocoFoundationd
  } else {
  LIBS	+= -lPocoFoundation
 }

  macx{
   LIBS += -lboost_signals
 } else{
   LIBS += -lboost_signals-mt
 }
}

win32 {
  INCLUDEPATH += "$$THIRDPARTY/include"

CONFIG(build64)  {
    THIRDPARTYLIB = "$$THIRDPARTY/lib/win64"
    message(SETTING FOR x64)
  } else {
    THIRDPARTYLIB = "$$THIRDPARTY/lib/win32"
    message(SETTING FOR x86)
  }
  LIBPATH     += $$THIRDPARTYLIB 
  LIBS += "MantidKernel.lib"
  LIBS += "MantidGeometry.lib"
  LIBS += "MantidAPI.lib"
  build_pass:CONFIG(release, debug|release) {
    LIBS += "PocoFoundation.lib"
    LIBS += "libboost_signals-vc80-mt-1_34_1.lib"
  }
  build_pass:CONFIG(debug, debug|release) {
    LIBS += "PocoFoundationd.lib"
    LIBS += "libboost_signals-vc80-mt-gd-1_34_1.lib"
  }
}

CONFIG(debug, debug|release) {
  MOC_DIR        = "$$TMPDIR/debug"
  OBJECTS_DIR    = "$$TMPDIR/debug"
} else {
  MOC_DIR        = "$$TMPDIR"
  OBJECTS_DIR    = "$$TMPDIR"
}
SIP_DIR = "$$TMPDIR"

win32:build_pass:CONFIG(debug, debug|release) {
  # Put alongside Mantid libraries
  DESTDIR = "$$MANTIDLIBPATH"
} else {
  # Put in local output directory
  DESTDIR = "$$TOPBUILDDIR/lib"
}

# This makes release the default build on running nmake. Must be here - after the config dependent parts above
CONFIG += release

