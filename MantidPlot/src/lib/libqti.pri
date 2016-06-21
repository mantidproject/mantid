###############################################################
##################### libqti ##################################
###############################################################

INCLUDEPATH += src/lib/include

HEADERS  += src/lib/include/CollapsiveGroupBox.h \
			src/lib/include/ColorBox.h \
			src/lib/include/ColorButton.h \
			src/lib/include/ColorMapEditor.h \
			src/lib/include/DoubleSpinBox.h \
			src/lib/include/ExtensibleFileDialog.h \
			src/lib/include/LineNumberDisplay.h \
			src/lib/include/PatternBox.h \
			src/lib/include/PenStyleBox.h \
			src/lib/include/SymbolBox.h \
			src/lib/include/SymbolDialog.h \
			src/lib/include/TextFormatButtons.h \

SOURCES  += src/lib/src/CollapsiveGroupBox.cpp \
			src/lib/src/ColorBox.cpp \
			src/lib/src/ColorButton.cpp \
			src/lib/src/ColorMapEditor.cpp \
			src/lib/src/DoubleSpinBox.cpp \
			src/lib/src/ExtensibleFileDialog.cpp \
			src/lib/src/LineNumberDisplay.cpp \
			src/lib/src/PatternBox.cpp \
			src/lib/src/PenStyleBox.cpp \
			src/lib/src/SymbolBox.cpp \
			src/lib/src/SymbolDialog.cpp \
			src/lib/src/TextFormatButtons.cpp \

###############################################################
##################### 3rdparty Qt Solutions ###################
###############################################################

INCLUDEPATH += src/lib/3rdparty/qtcolorpicker/src
HEADERS  += src/lib/3rdparty/qtcolorpicker/src/qtcolorpicker.h
SOURCES  += src/lib/3rdparty/qtcolorpicker/src/qtcolorpicker.cpp
