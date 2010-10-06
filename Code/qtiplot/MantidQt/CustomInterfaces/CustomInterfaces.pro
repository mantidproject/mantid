#--------------------------------
# The CustomInterface library - 
# MantidQtInterfaces
#--------------------------------
TEMPLATE = lib

# Import the global config file
include(../mantidqt.pri)

!exists(\"$$MANTIDQTINCLUDES/MantidQtCustomInterfaces\") {
  system(mkdir \"$$MANTIDQTINCLUDES/MantidQtCustomInterfaces\")
}


### Include qwt

unix:!macx {
	exists(/usr/include/qwt-qt4) {
	INCLUDEPATH += /usr/include/qwt-qt4/
	} else {
	INCLUDEPATH += /usr/include/qwt/
	}
} else {
	INCLUDEPATH += ../../3rdparty/qwt/src
}

### Link qwt

# POSIX
unix {
	
	macx {
		LIBS += -L../../3rdparty/qwt/lib -lqwt
	} else {
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
#WINDOWS

win32 {

	# LIBPATH

	CONFIG(build64) {
		LIBPATH += ../../../Third_Party/lib/win64
	} else {
		LIBPATH += ../../../Third_Party/lib/win32
	}

	build_pass:CONFIG(debug, debug|release) {
		LIBS += qwtd.lib
	} else {
		LIBS += qwt.lib
	}
}


# Link with the libraries it is CustomInterfaces uses
unix:LIBS += -L$$TOPBUILDDIR/lib -lMantidQtAPI -lMantidWidgets
win32:LIBS += "$$DESTDIR\MantidQtAPI.lib" "$$DESTDIR\MantidWidgets.lib"

#------------------------
# Source fies
#------------------------

HEADERDIR = inc
SRCDIR = src

SOURCES = \
  $$SRCDIR/SANSRunWindow.cpp \
  $$SRCDIR/SANSUtilityDialogs.cpp \
  $$SRCDIR/SANSAddFiles.cpp \
  $$SRCDIR/Homer.cpp \
  $$SRCDIR/deltaECalc.cpp \
  $$SRCDIR/Background.cpp \
  $$SRCDIR/ConvertToEnergy.cpp \
  $$SRCDIR/Indirect.cpp \
  $$SRCDIR/indirectAnalysis.cpp \
  $$SRCDIR/MuonAnalysis.cpp \
  $$SRCDIR/IO_MuonGrouping.cpp


HEADERS = \
  $$HEADERDIR/SANSRunWindow.h \
  $$HEADERDIR/SANSUtilityDialogs.h \
  $$HEADERDIR/SANSAddFiles.h \
  $$HEADERDIR/Homer.h \
  $$HEADERDIR/deltaECalc.h \
  $$HEADERDIR/Background.h \
  $$HEADERDIR/ConvertToEnergy.h \
  $$HEADERDIR/Indirect.h \
  $$HEADERDIR/indirectAnalysis.h \
  $$HEADERDIR/MuonAnalysis.h \
  $$HEADERDIR/IO_MuonGrouping.h
  
UI_DIR = $$HEADERDIR

FORMS = \
  $$HEADERDIR/SANSRunWindow.ui \
  $$HEADERDIR/ConvertToEnergy.ui \
  $$HEADERDIR/indirectAnalysis.ui \
  $$HEADERDIR/MuonAnalysis.ui

UI_HEADERS_DIR = "$$MANTIDQTINCLUDES/MantidQtCustomInterfaces"

TARGET = MantidQtCustomInterfaces

unix:headercopy.commands = cd $$HEADERDIR && $(COPY) *.h '"$$MANTIDQTINCLUDES/MantidQtCustomInterfaces"'
win32:headercopy.commands = cd $$HEADERDIR && $(COPY) *.h '"$$MANTIDQTINCLUDES\MantidQtCustomInterfaces"'
PRE_TARGETDEPS = headercopy

QMAKE_EXTRA_TARGETS += headercopy
