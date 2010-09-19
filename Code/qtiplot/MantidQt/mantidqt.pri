#------------------------------
# This sets some variables that
# are used in the sub-projects
#-----------------------------

#------------------------
# Compile mode
#------------------------
CONFIG += qt warn_on exceptions debug_and_release

win32:DEFINES += QT_DLL QT_THREAD_SUPPORT _WINDOWS WIN32 BOOST_ALL_DYN_LINK POCO_DLL
DEFINES += BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG

# Mantid requires a macro to tell it if stdint.h exists but qmake has no simple function
# to check system header paths and worse still no way of accessing what they are!
# For simplicity we'll assume existence on Unix and Mac
unix|macx {
   DEFINES += HAVE_STDINT_H
}

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
MANTIDLIBPATH = "$$MANTIDPATH/release"
build_pass:CONFIG(debug, debug|release) {
  MANTIDLIBPATH = "$$MANTIDPATH/debug"
}

win32:LIBPATH += $$MANTIDLIBPATH

THIRDPARTY = "$$TOPBUILDDIR/../../Third_Party"

MANTIDQTINCLUDES = $$TOPBUILDDIR/includes

!exists(\"$$MANTIDQTINCLUDES\") {
  system(mkdir \"$$MANTIDQTINCLUDES\")
}

TMPDIR = $$TOPBUILDDIR/qtbuild/MantidQt

# Qt qmake variables

DEPENDPATH += "$$MANTIDPATH/Kernel/inc/"
DEPENDPATH += "$$MANTIDPATH/Geometry/inc/"
DEPENDPATH += "$$MANTIDPATH/API/inc/"
INCLUDEPATH += "$$MANTIDPATH/Kernel/inc/"
INCLUDEPATH += "$$MANTIDPATH/Geometry/inc/"
INCLUDEPATH += "$$MANTIDPATH/API/inc/"
INCLUDEPATH += "$$MANTIDQTINCLUDES"

macx {
  INCLUDEPATH += "$$THIRDPARTY/include"

  LIBS += -L$$THIRDPARTY/lib/mac/ -lPocoFoundation
}

unix {
  LIBS += -L$$MANTIDLIBPATH -lMantidKernel
  LIBS += -L$$MANTIDLIBPATH -lMantidAPI
  LIBS += -L$$MANTIDLIBPATH -lMantidGeometry

  CONFIG(debug, debug|release) {
  LIBS	+= -lPocoFoundationd
  LIBS  += -lPocoXMLd
  } else {
  LIBS	+= -lPocoFoundation
  LIBS  += -lPocoXML
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
    DEFINES += HAVE_STDINT_H
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
    LIBS += "PocoXML.lib"
  }
  build_pass:CONFIG(debug, debug|release) {
    LIBS += "PocoFoundationd.lib"
    LIBS += "PocoXMLd.lib"
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

# This automatically switches to $$MANTIDPATH/debug for a debug build
DESTDIR = "$$MANTIDLIBPATH"

# This makes release the default build on running nmake. Must be here - after the config dependent parts above
CONFIG += release

