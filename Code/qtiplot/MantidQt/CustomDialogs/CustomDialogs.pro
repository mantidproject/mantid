#--------------------------------
# The CustomDialogs library - 
# MantidQtCustomDialogs
#--------------------------------
TEMPLATE = lib

# Import the global config file
include(../mantidqt.pri)

CONFIG += qt
QT += opengl

unix:system(mkdir '"$$MANTIDQTINCLUDES/MantidQtCustomDialogs"')
win32:system(mkdir '"$$MANTIDQTINCLUDES\MantidQtCustomDialogs"')

# Need to link with the API
unix:LIBS += -L$$TOPBUILDDIR/lib -lMantidQtAPI
win32:LIBS += "$$DESTDIR\MantidQtAPI.lib"

#------------------------
# Source fies
#------------------------

HEADERDIR = inc
SRCDIR = src

SOURCES = \
  $$SRCDIR/LoadRawDialog.cpp \
  $$SRCDIR/LOQScriptInputDialog.cpp \
  $$SRCDIR/CreateSampleShapeDialog.cpp \
  $$SRCDIR/SampleShapeHelpers.cpp \
  $$SRCDIR/MantidGLWidget.cpp \
  $$SRCDIR/DiagScriptInputDialog.cpp

HEADERS = \
  $$HEADERDIR/LoadRawDialog.h \
  $$HEADERDIR/LOQScriptInputDialog.h \
  $$HEADERDIR/CreateSampleShapeDialog.h \
  $$HEADERDIR/SampleShapeHelpers.h \
  $$HEADERDIR/MantidGLWidget.h \
  $$HEADERDIR/DiagScriptInputDialog.h

UI_DIR = $$HEADERDIR

FORMS = \
  $$HEADERDIR/LOQScriptInputDialog.ui \
  $$HEADERDIR/CreateSampleShapeDialog.ui \
  $$HEADERDIR/DiagScriptInputDialog.ui

UI_HEADERS_DIR = "$$MANTIDQTINCLUDES/MantidQtCustomDialogs"

TARGET = MantidQtCustomDialogs

unix:headercopy.commands = cd $$HEADERDIR && $(COPY) *.h '"$$MANTIDQTINCLUDES/MantidQtCustomDialogs"'
win32:headercopy.commands = cd $$HEADERDIR && $(COPY) *.h '"$$MANTIDQTINCLUDES\MantidQtCustomDialogs"'
PRE_TARGETDEPS = headercopy

QMAKE_EXTRA_TARGETS += headercopy
