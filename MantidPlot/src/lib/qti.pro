QMAKE_PROJECT_DEPTH = 0
linux-g++-64: libsuff=64

TARGET            = qti
TEMPLATE          = lib

VERSION      	  = 0.0.1

CONFIG           += thread
CONFIG           += warn_on 
CONFIG           += release
CONFIG           += staticlib 

MOC_DIR           = ./tmp/
OBJECTS_DIR       = ./tmp/
DESTDIR           = ./

INCLUDEPATH      += ./include/
INCLUDEPATH      += ../../../3rdparty/qwt/src

##################### Linux (Mac OS X) ######################################

# statically link against Qwt library in 3rdparty
unix:LIBS         += ../3rdparty/qwt/lib/libqwt.a

# dynamically link against dependencies if they are installed system-wide
#unix:LIBS        += -lqwt

##################### Windows ###############################################
win32:LIBS        += ../3rdparty/qwt/lib/libqwt.a

HEADERS  += include/ColorBox.h \
			include/ColorButton.h \
            include/ColorMapEditor.h \
			include/DoubleSpinBox.h \
			include/ExtensibleFileDialog.h \
			include/LineNumberDisplay.h \
            include/PatternBox.h \
			include/PenStyleBox.h \
			include/SymbolBox.h \

SOURCES  += src/ColorBox.cpp \
			src/ColorButton.cpp \
            src/ColorMapEditor.cpp \
			src/DoubleSpinBox.cpp \
			src/ExtensibleFileDialog.cpp \
			src/LineNumberDisplay.cpp \
            src/PatternBox.cpp \
			src/PenStyleBox.cpp \
			src/SymbolBox.cpp \
