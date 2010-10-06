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
# For simplicity we'll assume existence on Unix and Mac.
# On windows we have to pretend to use win32-msvc2008 when using MSVC 2010 so we'll have to do it there as well.
unix|macx|win32-msvc2008|win32-msvc2010 {
   unix|macx|win32-msvc2010 {
      DEFINES += HAVE_STDINT_H
   }
   win32-msvc2008 {
      exists("C:\Program Files\Microsoft Visual Studio 10.0\VC\include\stdint.h") | exists("C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\include\stdint.h") {
        DEFINES += HAVE_STDINT_H
    }
   }
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
INCLUDEPATH += "$$TOPBUILDDIR/../QtPropertyBrowser/src/"

unix:!macx {
	exists(/usr/include/qwt-qt4) {
	INCLUDEPATH += /usr/include/qwt-qt4/
	} else {
	INCLUDEPATH += /usr/include/qwt/
	}
} else {
	INCLUDEPATH += "$$TOPBUILDDIR/../3rdparty/qwt/src/"
}


macx {
  INCLUDEPATH += "$$THIRDPARTY/include"

  LIBS += -L$$THIRDPARTY/lib/mac/ -lPocoFoundation
}

unix {
  LIBS += -L$$MANTIDLIBPATH -lMantidKernel
  LIBS += -L$$MANTIDLIBPATH -lMantidAPI
  LIBS += -L$$MANTIDLIBPATH -lMantidGeometry
  LIBS += -L$$MANTIDLIBPATH -lQtPropertyBrowser

  CONFIG(debug, debug|release) {
  LIBS	+= -lPocoFoundationd
  LIBS  += -lPocoXMLd
  } else {
  LIBS	+= -lPocoFoundation
  LIBS  += -lPocoXML
 }

  macx {
	LIBS += -lboost_signals
	LIBS += -L../3rdparty/qwt/lib -lqwt
	} else {
		LIBS += -lboost_signals-mt
		exists ( /usr/lib64/libqwt-qt4.so ) {
			LIBS += -lqwt-qt4
		} else {
			exists ( /usr/lib/libqwt-qt4.so ) {
				LIBS += -lqwt-qt4
			} else {
				LIBS += -lqwt
			}
		}
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
    LIBS += "PocoXML.lib"
    LIBS += "QtPropertyBrowser.lib"
	LIBS += "qwt.lib"
  }
  build_pass:CONFIG(debug, debug|release) {
	# Make sure we don't link to the non-debug runtime
	QMAKE_LFLAGS_DEBUG += /NODEFAULTLIB:msvcrt.lib
    LIBS += "PocoFoundationd.lib"
    LIBS += "PocoXMLd.lib"
    LIBS += "QtPropertyBrowserd.lib"
	LIBS += "qwtd.lib"
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

