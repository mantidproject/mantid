# --------------------------------
# The CustomDialogs library -
# MantidQtCustomDialogs
# --------------------------------
TEMPLATE = lib

# Import the global config file
include(../mantidqt.pri)
CONFIG += qt
QT += opengl


#!exists(\"$$MANTIDQTINCLUDES/MantidQtCustomDialogs\") {
#  system(mkdir \"$$MANTIDQTINCLUDES/MantidQtCustomDialogs\")
#}

# Need to link with the API
unix:LIBS += -L$$TOPBUILDDIR/lib -lMantidQtAPI -lMantidWidgets
win32:LIBS += "$$DESTDIR\MantidQtAPI.lib" "$$DESTDIR\MantidWidgets.lib"

# ------------------------
# Source fies
# ------------------------
HEADERDIR = inc/MantidQtCustomDialogs
SRCDIR = src

SOURCES = $$SRCDIR/LoadRawDialog.cpp \
    $$SRCDIR/LOQScriptInputDialog.cpp \
    $$SRCDIR/CreateSampleShapeDialog.cpp \
    $$SRCDIR/SampleShapeHelpers.cpp \
    $$SRCDIR/MantidGLWidget.cpp \
    $$SRCDIR/PlotAsymmetryByLogValueDialog.cpp \
    $$SRCDIR/LoadDAEDialog.cpp \
    $$SRCDIR/LoadAsciiDialog.cpp \
    $$SRCDIR/LoadDialog.cpp
        
HEADERS = $$HEADERDIR/LoadRawDialog.h \
    $$HEADERDIR/LOQScriptInputDialog.h \
    $$HEADERDIR/CreateSampleShapeDialog.h \
    $$HEADERDIR/SampleShapeHelpers.h \
    $$HEADERDIR/MantidGLWidget.h \
    $$HEADERDIR/PlotAsymmetryByLogValueDialog.h \
    $$HEADERDIR/LoadDAEDialog.h \
    $$HEADERDIR/LoadAsciiDialog.h \
    $$HEADERDIR/LoadDialog.h
    
UI_DIR = $$HEADERDIR

FORMS = $$HEADERDIR/LOQScriptInputDialog.ui \
    $$HEADERDIR/CreateSampleShapeDialog.ui \
    $$HEADERDIR/PlotAsymmetryByLogValueDialog.ui 
    
UI_HEADERS_DIR = "$$HEADERDIR"

TARGET = MantidQtCustomDialogs
unix{
  HEADERDIR = $$TOPBUILDDIR/CustomDialogs/inc
  SRCDIR = $$TOPBUILDDIR/CustomDialogs/src
#  headercopy.commands = cd \
#    $$HEADERDIR \
#    && \
#    $(COPY) \
#    *.h \
#    '"$$MANTIDQTINCLUDES/MantidQtCustomDialogs"'
}

win32{
  HEADERDIR = $$TOPBUILDDIR\CustomDialogs\inc
  SRCDIR = $$TOPBUILDDIR\CustomDialogs\src
#  headercopy.commands = cd \
#    $$HEADERDIR \
#    && \
#    $(COPY) \
#    *.h \
#    '"$$MANTIDQTINCLUDES\MantidQtCustomDialogs"'
}

#PRE_TARGETDEPS = headercopy
#QMAKE_EXTRA_TARGETS += headercopy
