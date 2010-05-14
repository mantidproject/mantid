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
  $$SRCDIR/Homer.cpp \
  $$SRCDIR/deltaECalc.cpp \
  $$SRCDIR/Background.cpp

HEADERS = \
  $$HEADERDIR/SANSRunWindow.h \
  $$HEADERDIR/SANSUtilityDialogs.h \
  $$HEADERDIR/Homer.h \
  $$HEADERDIR/deltaECalc.h \
  $$HEADERDIR/Background.h 
  
UI_DIR = $$HEADERDIR

FORMS = \
  $$HEADERDIR/SANSRunWindow.ui \
  $$HEADERDIR/Homer.ui \

UI_HEADERS_DIR = "$$MANTIDQTINCLUDES/MantidQtCustomInterfaces"

TARGET = MantidQtCustomInterfaces

unix:headercopy.commands = cd $$HEADERDIR && $(COPY) *.h '"$$MANTIDQTINCLUDES/MantidQtCustomInterfaces"'
win32:headercopy.commands = cd $$HEADERDIR && $(COPY) *.h '"$$MANTIDQTINCLUDES\MantidQtCustomInterfaces"'
PRE_TARGETDEPS = headercopy

QMAKE_EXTRA_TARGETS += headercopy
