#--------------------------------
# The CustomDialogs library - 
# MantidQtCustomDialogs
#--------------------------------
TEMPLATE = lib

# Import the global config file
include(../mantidqt.pri)

unix:system(mkdir '"$$MANTIDQTINCLUDES/MantidQtCustomDialogs"')
win32:system(mkdir '"$$MANTIDQTINCLUDES\MantidQtCustomDialogs"')

# Need to link with the API
unix:LIBS += -L$$TOPBUILDDIR/lib -lMantidQtAPI
win32:LIBS += "$$TOPBUILDDIR\lib\MantidQtAPI.lib"

#------------------------
# Source fies
#------------------------

HEADERDIR = inc
SRCDIR = src

SOURCES = \
  $$SRCDIR/LoadRawDialog.cpp \
  $$SRCDIR/LOQScriptInputDialog.cpp

HEADERS = \
  $$HEADERDIR/LoadRawDialog.h \
  $$HEADERDIR/LOQScriptInputDialog.h
  
UI_DIR = $$HEADERDIR

FORMS = $$HEADERDIR/LOQScriptInputDialog.ui

TARGET = MantidQtCustomDialogs

unix:headercopy.commands = cd $$HEADERDIR && $(COPY) *.h '"$$MANTIDQTINCLUDES/MantidQtCustomDialogs"'
win32:headercopy.commands = cd $$HEADERDIR && $(COPY) *.h '"$$MANTIDQTINCLUDES\MantidQtCustomDialogs"'
PRE_TARGETDEPS = headercopy

QMAKE_EXTRA_TARGETS += headercopy
