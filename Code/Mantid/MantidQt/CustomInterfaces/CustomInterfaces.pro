#--------------------------------
# The CustomInterface library - 
# MantidQtInterfaces
#--------------------------------
TEMPLATE = lib

# Import the global config file
include(../mantidqt.pri)

#!exists(\"$$MANTIDQTINCLUDES/MantidQtCustomInterfaces\") {
#  system(mkdir \"$$MANTIDQTINCLUDES/MantidQtCustomInterfaces\")
#}


# Link with the libraries it is CustomInterfaces uses
unix:LIBS += -L$$TOPBUILDDIR/lib -lMantidQtAPI -lMantidWidgets
win32:LIBS += "$$DESTDIR\MantidQtAPI.lib" "$$DESTDIR\MantidWidgets.lib"

#------------------------
# Source fies
#------------------------

HEADERDIR = inc/MantidQtCustomInterfaces
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
  $$SRCDIR/IndirectDataAnalysis.cpp \
  $$SRCDIR/MuonAnalysis.cpp \
  $$SRCDIR/IO_MuonGrouping.cpp \
  $$SRCDIR/IndirectDiffractionReduction.cpp


HEADERS = \
  $$HEADERDIR/SANSRunWindow.h \
  $$HEADERDIR/SANSUtilityDialogs.h \
  $$HEADERDIR/SANSAddFiles.h \
  $$HEADERDIR/Homer.h \
  $$HEADERDIR/deltaECalc.h \
  $$HEADERDIR/Background.h \
  $$HEADERDIR/ConvertToEnergy.h \
  $$HEADERDIR/Indirect.h \
  $$HEADERDIR/IndirectDataAnalysis.h \
  $$HEADERDIR/MuonAnalysis.h \
  $$HEADERDIR/IO_MuonGrouping.h \
  $$HEADERDIR/IndirectDiffractionReduction.h
  
UI_DIR = $$HEADERDIR

FORMS = \
  $$HEADERDIR/SANSRunWindow.ui \
  $$HEADERDIR/ConvertToEnergy.ui \
  $$HEADERDIR/IndirectDataAnalysis.ui \
  $$HEADERDIR/MuonAnalysis.ui \
  $$HEADERDIR/IndirectDiffractionReduction.ui

UI_HEADERS_DIR = "$$HEADERDIR"

TARGET = MantidQtCustomInterfaces

unix{
  HEADERDIR = $$TOPBUILDDIR/CustomInterfaces/inc
  SRCDIR = $$TOPBUILDDIR/CustomInterfaces/src
#  headercopy.commands = cd $$HEADERDIR && $(COPY) *.h '"$$MANTIDQTINCLUDES/MantidQtCustomInterfaces"'
}

win32{
  HEADERDIR = $$TOPBUILDDIR\CustomInterfaces\inc
  SRCDIR = $$TOPBUILDDIR\CustomInterfaces\src
#  headercopy.commands = cd $$HEADERDIR && $(COPY) *.h '"$$MANTIDQTINCLUDES\MantidQtCustomInterfaces"'
}

#PRE_TARGETDEPS = headercopy

#QMAKE_EXTRA_TARGETS += headercopy
