#--------------------------------
# The API library MantidQtAPI
#--------------------------------
TEMPLATE = lib

# Import the config file
include(../mantidqt.pri)

unix:system(mkdir '"$$MANTIDQTINCLUDES/MantidQtMantidWidgets"')
win32:system(mkdir '"$$MANTIDQTINCLUDES\MantidQtMantidWidgets"')

# Need to link with the API
unix:LIBS += -L$$TOPBUILDDIR/lib -lMantidQtAPI
win32:LIBS += "$$DESTDIR\MantidQtAPI.lib"

DEFINES += IN_MANTIDQT_MANTIDWIDGETS

#------------------------
# Source fies
#------------------------

HEADERDIR = inc
SRCDIR = src

SOURCES = \
  $$SRCDIR/MantidWidget.cpp \
  $$SRCDIR/pythonCalc.cpp \
  $$SRCDIR/DiagResults.cpp \
  $$SRCDIR/MWDiag.cpp \
  $$SRCDIR/MWDiagCalcs.cpp

HEADERS = \
  $$HEADERDIR/MantidWidget.h \
  $$HEADERDIR/pythonCalc.h \
  $$HEADERDIR/WidgetsDllOption.h \
  $$HEADERDIR/DiagResults.h \
  $$HEADERDIR/MWDiag.h \
  $$HEADERDIR/MWDiagCalcs.h
  
UI_DIR = $$HEADERDIR

FORMS = \
  $$HEADERDIR/MWDiag.ui

UI_HEADERS_DIR = "$$MANTIDQTINCLUDES/MantidQtMantidWidgets"
  
#-----------------------------
# Target and dependancies
#-----------------------------

unix:headercopy.commands = cd $$HEADERDIR && $(COPY) *.h '"$$MANTIDQTINCLUDES/MantidQtMantidWidgets"'
win32:headercopy.commands = cd "$$HEADERDIR" && $(COPY) *.h '"$$MANTIDQTINCLUDES\MantidQtMantidWidgets"'
PRE_TARGETDEPS = headercopy

QMAKE_EXTRA_TARGETS += headercopy
