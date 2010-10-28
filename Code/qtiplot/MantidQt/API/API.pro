#--------------------------------
# The API library MantidQtAPI
#--------------------------------
TEMPLATE = lib

# Import the config file
include(../mantidqt.pri)

!exists(\"$$MANTIDQTINCLUDES/MantidQtAPI\") {
  system(mkdir \"$$MANTIDQTINCLUDES/MantidQtAPI\")
}

DEFINES += IN_MANTIDQT_API

#------------------------
# Source fies
#------------------------

HEADERDIR = inc
SRCDIR = src

SOURCES = \
  $$SRCDIR/InterfaceFactory.cpp \
  $$SRCDIR/InterfaceManager.cpp \
  $$SRCDIR/AlgorithmInputHistory.cpp \
  $$SRCDIR/AlgorithmDialog.cpp \
  $$SRCDIR/GenericDialog.cpp \
  $$SRCDIR/UserSubWindow.cpp \
  $$SRCDIR/MantidQtDialog.cpp \
  $$SRCDIR/ManageUserDirectories.cpp

  
HEADERS = \
  $$HEADERDIR/DllOption.h \
  $$HEADERDIR/InterfaceFactory.h \
  $$HEADERDIR/InterfaceManager.h \
  $$HEADERDIR/AlgorithmInputHistory.h \
  $$HEADERDIR/AlgorithmDialog.h \
  $$HEADERDIR/GenericDialog.h \
  $$HEADERDIR/UserSubWindow.h \
  $$HEADERDIR/MantidQtDialog.h \
  $$HEADERDIR/FileDialogHandler.h \
  $$HEADERDIR/ManageUserDirectories.h

UI_DIR = $$HEADERDIR

FORMS = \
  $$HEADERDIR/ManageUserDirectories.ui
  
UI_HEADERS_DIR = "$$MANTIDQTINCLUDES/MantidQtAPI"
#-----------------------------
# Target and dependancies
#-----------------------------
TARGET = MantidQtAPI


unix{
  HEADERDIR = $$TOPBUILDDIR/API/inc
  SRCDIR = $$TOPBUILDDIR/API/src
  headercopy.commands = cd $$HEADERDIR && $(COPY) *.h '$$MANTIDQTINCLUDES/MantidQtAPI'
}

win32{
  HEADERDIR = $$TOPBUILDDIR\API\inc
  SRCDIR = $$TOPBUILDDIR\API\src
  headercopy.commands = cd "$$HEADERDIR" && $(COPY) *.h '"$$MANTIDQTINCLUDES\\MantidQtAPI"'
}

PRE_TARGETDEPS = headercopy

QMAKE_EXTRA_TARGETS += headercopy

