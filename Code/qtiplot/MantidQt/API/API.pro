#--------------------------------
# The API library MantidQtAPI
#--------------------------------
TEMPLATE = lib

# Import the config file
include(../mantidqt.pri)

unix:system(mkdir $$MANTIDQTINCLUDES/MantidQtAPI)
win32:system(mkdir $$MANTIDQTINCLUDES\MantidQtAPI)

DEFINES += IN_MANTIDQT_API

#------------------------
# Source fies
#------------------------

HEADERDIR = inc
SRCDIR = src

SOURCES = \
  $$SRCDIR/DialogFactory.cpp \
  $$SRCDIR/DialogManager.cpp \
  $$SRCDIR/AlgorithmInputHistory.cpp \
  $$SRCDIR/AlgorithmDialog.cpp \
  $$SRCDIR/GenericDialog.cpp \
  
HEADERS = \
  $$HEADERDIR/DllOption.h \
  $$HEADERDIR/DialogFactory.h \
  $$HEADERDIR/DialogManager.h \
  $$HEADERDIR/AlgorithmInputHistory.h \
  $$HEADERDIR/AlgorithmDialog.h \
  $$HEADERDIR/GenericDialog.h \
  
  
#-----------------------------
# Target and dependancies
#-----------------------------
TARGET = MantidQtAPI

unix:headercopy.commands = cd $$HEADERDIR && $(COPY) *.h $$MANTIDQTINCLUDES/MantidQtAPI
win32:headercopy.commands = cd $$HEADERDIR && $(COPY) *.h $$MANTIDQTINCLUDES\MantidQtAPI
PRE_TARGETDEPS = headercopy

QMAKE_EXTRA_TARGETS += headercopy
