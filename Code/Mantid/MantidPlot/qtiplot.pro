#############################################################################
###################### USER-SERVICEABLE PART ################################
#############################################################################
unix {
QMAKE_CXXFLAGS_RELEASE += -g
QMAKE_LFLAGS_RELEASE += -g
 macx{
 QMAKESPEC=macx-g++
 }
}

# building without muParser doesn't work yet
SCRIPTING_LANGS += muParser
SCRIPTING_LANGS += Python

# a console displaying output of scripts; particularly useful on Windows
# where running QtiPlot from a terminal is inconvenient
DEFINES         += SCRIPTING_CONSOLE
# a dialog for selecting the scripting language on a per-project basis
DEFINES         += SCRIPTING_DIALOG
DEFINES         +=  BOOST_DATE_TIME_POSIX_TIME_STD_CONFIG
win32:DEFINES   += _WIN32_WINNT=0x0400 _WIN32 GSL_DLL POCO_DLL BOOST_ALL_DYN_LINK NOMINMAX POCO_NO_UNWINDOWS

# Mantid requires a macro to tell it if stdint.h exists but qmake has no simple function
# to check system header paths and worse still no way of accessing what they are!
# For simplicity we'll assume existence on Unix and Mac.
# On windows we have to pretend to use win32-msvc2008 when using MSVC 2010 so we'll have to do it there as well.
unix|macx|win32-msvc2008|win32-msvc2010 {
   unix|macx|win32-msvc2010 {
      DEFINES += HAVE_STDINT_H
   }
   win32-msvc2008 {
      exists("C:\Program Files\Microsoft Visual Studio 10.0\VC\include\stdint.h") | exists("C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\include\stdint.h") {
        DEFINES += HAVE_STDINT_H
    }
   }
}

RESOURCES        = ../Images/images.qrc
RESOURCES        += ../Images/ICatSearchBackGround.qrc

# comment the following lines if you haven't subscribed for a QtiPlot binaries maintenance contract
#RESTRICTED_MODULES += EMF

######################################################################################
# Uncomment the following line if you want to perform a custom installation using 
# the *.path variables defined bellow. 
######################################################################################
#CONFIG          += CustomInstall

CONFIG += debug_and_release
win32:build_pass:CONFIG(debug, debug|release) {
  # Make sure we don't link to the non-debug runtime
  QMAKE_LFLAGS_DEBUG += /NODEFAULTLIB:msvcrt.lib
}

MANTIDPATH = ../Framework
MANTIDLIBPATH = $$MANTIDPATH/release
build_pass:CONFIG(debug, debug|release) {
  MANTIDLIBPATH = $$MANTIDPATH/debug
}

# Put executable alongside the mantid framework libraries
# Automatically switches destination to Mantid/debug if a debug build
DESTDIR = $$MANTIDLIBPATH

mac:CXXFLAGS+=-headerpad_max_install_names

##################### 3rd PARTY HEADER FILES SECTION ########################
#!!! Warning: You must modify these paths according to your computer settings
#############################################################################
# Global 

DEPENDPATH  += ../Framework/Kernel/inc/
DEPENDPATH  += ../Framework/Geometry/inc/
DEPENDPATH  += ../Framework/API/inc/
DEPENDPATH  += ../MantidQt/MantidWidgets/inc
DEPENDPATH  += ../MantidQt/API/inc
DEPENDPATH  += src
INCLUDEPATH  += ../Framework/Kernel/inc/
INCLUDEPATH  += ../Framework/Geometry/inc/
INCLUDEPATH  += ../Framework/API/inc/
INCLUDEPATH  += ../MantidQt/API/inc
INCLUDEPATH  += ../MantidQt/MantidWidgets/inc
INCLUDEPATH  += ../3rdparty/liborigin
INCLUDEPATH  += ../QtPropertyBrowser/src
INCLUDEPATH  += src
INCLUDEPATH  += ../tmp/qtiplot

unix:!macx {

  INCLUDEPATH       += /usr/include/
  INCLUDEPATH       += /usr/include/muParser/
  exists(/usr/include/qwt-qt4) {
    INCLUDEPATH       += /usr/include/qwt-qt4/
  } else {
    INCLUDEPATH       += /usr/include/qwt/
  }
  exists(/usr/include/qwtplot3d-qt4) {
    INCLUDEPATH       += /usr/include/qwtplot3d-qt4/
  } else {
    INCLUDEPATH       += /usr/include/qwtplot3d/
  }
} else{
  INCLUDEPATH       += ../../Third_Party/include/
  INCLUDEPATH       += ../../Third_Party/include/muparser
  INCLUDEPATH       += ../../Third_Party/include
  INCLUDEPATH       += ../../Third_Party/include/zlib123
  INCLUDEPATH       += ../../Third_Party/include/qwtplot3d
  INCLUDEPATH       += ../../Third_Party/include/qwt
}

##################### 3rd PARTY LIBRARIES SECTION ###########################
#!!! Warning: You must modify these paths according to your computer settings
#############################################################################

##################### Linux (Mac OS X) ######################################

# dynamically link against dependencies if they are installed system-wide
unix {
  LIBS         += -lqscintilla2
  LIBS         += -lmuparser

macx {
  LIBS += -L../../Third_Party/lib/mac
  LIBS += -L../3rdparty/qwt/lib -lqwt
  LIBS += -lqwtplot3d
  LIBS += -lboost_signals
}else{
  # MG: By default, ld populates the symbol table with only those referenced within the linked libraries.
  # Given that Mantid uses dlopen it may not necessarily reference all symbols at link time. We therefore need to 
  # ensure that the symbol table is populated with all symbols defined within the linked libraries.
  QMAKE_LFLAGS += -Wl,--export-dynamic

  # Some systems have qwt and qwtplot3d-qt4 and others have -qt4 suffixes on both
  # and some in /usr/lib64 others /usr/lib
  # some systems also have a /usr/lib64 but are not 64 bit
  exists(/usr/lib64/libqwt-qt4.so){
    LIBS += -lqwt-qt4
  }else{
    exists(/usr/lib/libqwt-qt4.so){
     LIBS += -lqwt-qt4
    }else{
     LIBS += -lqwt
    }
  }
  exists(/usr/lib64/libqwtplot3d-qt4.so){
    LIBS += -lqwtplot3d-qt4
  } else{
    exists(/usr/lib/libqwtplot3d-qt4.so){
     LIBS += -lqwtplot3d-qt4
    }else{
     LIBS += -lqwtplot3d
    }
  }
  LIBS += -lboost_signals-mt

  LIBS += -Wl,-rpath,/opt/Mantid/bin
  LIBS += -Wl,-rpath,/opt/Mantid/plugins
  LIBS += -Wl,-rpath,/opt/OpenCASCADE/lib
  LIBS += -Wl,-rpath,/opt/OpenCASCADE/lib64

}

  LIBS += -lgsl -lgslcblas

  LIBS += -L$$MANTIDLIBPATH -lMantidAPI
  LIBS += -lMantidGeometry
  LIBS += -lMantidKernel

  LIBS += -lMantidQtAPI
  LIBS += -lMantidWidgets
  LIBS += -lboost_date_time

CONFIG(debug, debug|release) {
  LIBS	+= -lPocoUtild
  LIBS	+= -lPocoFoundationd
  LIBS += -lQtPropertyBrowserd
} else {
  LIBS	+= -lPocoUtil
  LIBS	+= -lPocoFoundation
  LIBS += -lQtPropertyBrowser
 }
}

##################### Windows ###############################################
win32 {
  PYTHONPREFIX=$$system(python -c "\"import sys; print sys.prefix\"")
  PYTHONLIBPATH=$$join(PYTHONPREFIX,,,\\libs)
  message(Python lib path $${PYTHONLIBPATH})
  
  LIBPATH += $${PYTHONLIBPATH}
  CONFIG(build64)  {
    THIRD_PARTY = ../../Third_Party/lib/win64
    message(SETTING FOR x64)
  } else {
    THIRD_PARTY = ../../Third_Party/lib/win32
    message(SETTING FOR x86)
  }
  
  LIBPATH += $${THIRD_PARTY}
  LIBS += zlib.lib
  build_pass:CONFIG(debug, debug|release) {
    LIBS += -lqscintilla2d
    LIBS += muparser_d.lib
    LIBS += PocoUtild.lib
    LIBS += PocoFoundationd.lib
    LIBS += gsl_d.lib
    LIBS += cblas_d.lib
    LIBS += QtPropertyBrowserd.lib
    LIBS += qwtd.lib
    LIBS += qwtplot3dd.lib
    
    # Add a console for stdout/stderr to be displayed
    CONFIG += console
  } else {
    LIBS += -lqscintilla2
    LIBS += muparser.lib
    LIBS += PocoUtil.lib
    LIBS += PocoFoundation.lib
    LIBS += gsl.lib
    LIBS += cblas.lib
    LIBS += QtPropertyBrowser.lib
    LIBS += qwt.lib
    LIBS += qwtplot3d.lib
  }
  
  build_pass:CONFIG(debug, debug|release) {
    # Just looks at the place where Visual Studio will put a debug build
    LIBPATH += ../Framework/debug
  } else {
    # Look in the right place for both Scons and Visual Studio builds
    LIBPATH += ../Framework/release
  } 

  LIBS += MantidAPI.lib
  LIBS += MantidGeometry.lib
  LIBS += MantidKernel.lib
  LIBS += MantidQtAPI.lib
  LIBS += MantidWidgets.lib
}

QMAKE_POST_LINK += $${QMAKE_COPY} qtiplotrc.py \"$$DESTDIR\" &
QMAKE_POST_LINK += $${QMAKE_COPY} qtiUtil.py \"$$DESTDIR\" &
QMAKE_POST_LINK += $${QMAKE_COPY} mantidplot.py \"$$DESTDIR\" &
QMAKE_POST_LINK += $${QMAKE_COPY} mantidplotrc.py \"$$DESTDIR\"

# This makes release the default build on running nmake. Must be here - after the config dependent parts above
CONFIG += release

#############################################################################
###################### END OF USER-SERVICEABLE PART #########################
#############################################################################

#############################################################################
###################### BASIC PROJECT PROPERTIES #############################
#############################################################################

QMAKE_PROJECT_DEPTH = 0

TARGET         = MantidPlot
TEMPLATE       = app
CONFIG        += qt warn_on exceptions opengl thread
CONFIG        += assistant

DEFINES       += QT_PLUGIN
contains(CONFIG, CustomInstall){
	INSTALLS        += target
	INSTALLS        += translations
	INSTALLS        += manual
	INSTALLS        += documentation
	unix:INSTALLS        += man

	unix: INSTALLBASE = /usr
	win32: INSTALLBASE = C:/QtiPlot

	unix: target.path = $$INSTALLBASE/bin
	unix: translations.path = $$INSTALLBASE/share/qtiplot/translations
	unix: manual.path = $$INSTALLBASE/share/doc/qtiplot/manual
	unix: documentation.path = $$INSTALLBASE/share/doc/qtiplot
	unix: man.path = $$INSTALLBASE/share/man/man1/

	win32: target.path = $$INSTALLBASE
	win32: translations.path = $$INSTALLBASE/translations
	win32: manual.path = $$INSTALLBASE/manual
	win32: documentation.path = $$INSTALLBASE/doc

	DEFINES       += TRANSLATIONS_PATH="\\\"$$replace(translations.path," ","\ ")\\\"
	DEFINES       += MANUAL_PATH="\\\"$$replace(manual.path," ","\ ")\\\"
	}
	
win32:DEFINES += QT_DLL QT_THREAD_SUPPORT _WINDOWS WIN32 _USE_MATH_DEFINES=true QSCINTILLA_DLL
QT            += opengl qt3support network svg xml

SIP_DIR        = ../tmp/qtiplot

CONFIG(debug, debug|release) {
  MOC_DIR        = ../tmp/qtiplot/debug
  OBJECTS_DIR    = ../tmp/qtiplot/debug
} else {
  MOC_DIR        = ../tmp/qtiplot/release
  OBJECTS_DIR    = ../tmp/qtiplot/release
}

#############################################################################
###################### PROJECT FILES SECTION ################################
#############################################################################

###################### ICONS ################################################

win32:RC_FILE = icons/qtiplot.rc
mac:RC_FILE   = icons/qtiplot.icns

###################### TRANSLATIONS #########################################

TRANSLATIONS    = translations/qtiplot_de.ts \
                  translations/qtiplot_es.ts \
                  translations/qtiplot_fr.ts \
                  translations/qtiplot_ru.ts \
                  translations/qtiplot_ja.ts \
                  translations/qtiplot_sv.ts

#system(lupdate -verbose qtiplot.pro)
#system(lrelease -verbose qtiplot.pro)

translations.files += translations/qtiplot_de.qm \
                  translations/qtiplot_es.qm \
                  translations/qtiplot_fr.qm \
                  translations/qtiplot_ru.qm \
                  translations/qtiplot_ja.qm \
                  translations/qtiplot_sv.qm

###################### DOCUMENTATION ########################################

manual.files += ../manual/html \
				../manual/qtiplot-manual-en.pdf

documentation.files += ../README.html \
                       ../gpl_licence.txt

unix: man.files += ../qtiplot.1

###################### HEADERS ##############################################

HEADERS  += src/ApplicationWindow.h \
            src/globals.h\
            src/Graph.h \
            src/Graph3D.h \
            src/Table.h \
            src/CurvesDialog.h \
            src/SetColValuesDialog.h \
            src/PlotDialog.h \
            src/Plot3DDialog.h \
            src/PlotWizard.h \
            src/ExportDialog.h \
            src/AxesDialog.h \
            src/PolynomFitDialog.h \
            src/ExpDecayDialog.h \
            src/FunctionDialog.h \
            src/FitDialog.h \
            src/SurfaceDialog.h \
            src/TableDialog.h \
            src/TextDialog.h \
            src/LineDialog.h \
            src/ScalePicker.h \
            src/TitlePicker.h \
            src/CanvasPicker.h \
            src/PlotCurve.h \
            src/QwtErrorPlotCurve.h \
            src/QwtPieCurve.h \
            src/ErrDialog.h \
            src/LegendWidget.h \
            src/ArrowMarker.h \
            src/ImageMarker.h \
            src/ImageDialog.h \
            src/fit_gsl.h \
            src/nrutil.h\
            src/pixmaps.h\
            src/MultiLayer.h\
            src/LayerDialog.h \
            src/IntDialog.h \
            src/SortDialog.h\
            src/Bar.h \
            src/Cone3D.h \
            src/ConfigDialog.h \
            src/QwtBarCurve.h \
            src/BoxCurve.h \
            src/QwtHistogram.h \
            src/VectorCurve.h \
            src/ScaleDraw.h \
            src/Matrix.h \
            src/MatrixDialog.h \
            src/MatrixSizeDialog.h \
            src/MatrixValuesDialog.h \
            src/DataSetDialog.h \
            src/MyParser.h \
            src/ColorBox.h \
            src/SymbolBox.h \
            src/PatternBox.h \
            src/importOPJ.h\
            src/SymbolDialog.h \
            src/Plot.h \
            src/ColorButton.h \
            src/AssociationsDialog.h \
            src/RenameWindowDialog.h \
			src/MdiSubWindow.h \
            src/InterpolationDialog.h\
            src/ImportASCIIDialog.h \
            src/ImageExportDialog.h\
            src/SmoothCurveDialog.h\
            src/FilterDialog.h\
            src/FFTDialog.h\
            src/Note.h\
            src/Folder.h\
            src/FindDialog.h\
            src/ScriptingEnv.h\
            src/Scripted.h\
            src/Script.h\
            src/ScriptEdit.h\
            src/ScriptEditor.h\
            src/FunctionCurve.h\
            src/Fit.h\
            src/MultiPeakFit.h\
            src/ExponentialFit.h\
            src/PolynomialFit.h\
            src/NonLinearFit.h\
            src/PluginFit.h\
            src/SigmoidalFit.h\
			src/LogisticFit.h\
            src/customevents.h\
            src/ScriptingLangDialog.h\
            src/TextFormatButtons.h\
            src/TableStatistics.h\
            src/Spectrogram.h\
            src/ColorMapEditor.h\
			src/ColorMapDialog.h\
            src/SelectionMoveResizer.h\
            src/Filter.h\
            src/Differentiation.h\
            src/Integration.h\
            src/Interpolation.h\
            src/SmoothFilter.h\
            src/FFTFilter.h\
            src/FFT.h\
            src/Convolution.h\
            src/Correlation.h\
            src/PlotToolInterface.h\
            src/ScreenPickerTool.h\
            src/DataPickerTool.h\
            src/RangeSelectorTool.h\
            src/TranslateCurveTool.h\
            src/MultiPeakFitTool.h\
            src/CurveRangeDialog.h\
            src/LineProfileTool.h\
            src/PlotEnrichement.h\
            src/ExtensibleFileDialog.h\
            src/OpenProjectDialog.h\
            src/Grid.h\
            src/MatrixModel.h\
            src/FitModelHandler.h \
            src/TextEditor.h \
            src/CustomActionDialog.h \
            src/DoubleSpinBox.h\
            src/MatrixCommand.h  \
            src/UserFunction.h  \
            src/ContourLinesEditor.h\
            src/PenStyleBox.h\
            src/ScriptingWindow.h\
            src/ScriptManagerWidget.h\
            src/Mantid/MantidApplication.h \
	    src/Mantid/LoadRawDlg.h \
	    src/Mantid/ExecuteAlgorithm.h \
	    src/Mantid/ImportWorkspaceDlg.h \
	    src/Mantid/AbstractMantidLog.h \
	    src/Mantid/MantidLog.h \
	    src/Mantid/MantidUI.h \
	    src/Mantid/MantidMatrix.h \
	    src/Mantid/MantidDock.h \
	    src/Mantid/AlgMonitor.h \
	    src/MantidPlotReleaseDate.h \
	    src/Mantid/MantidAbout.h \
	    src/Mantid/InputHistory.h \
	    src/Mantid/Preferences.h \
        src/Mantid/MantidSampleLogDialog.h \
        src/Mantid/AlgorithmHistoryWindow.h\
        src/Mantid/MantidMatrixDialog.h \
	    src/Mantid/PeakPickerTool.h \
	    src/Mantid/MantidCurve.h \
	    src/Mantid/WorkspaceObserver.h \
	    src/Mantid/UserFitFunctionDialog.h \
	    src/Mantid/FitPropertyBrowser.h \
	    src/Mantid/PropertyHandler.h \
	    src/Mantid/IFunctionWrapper.h \
	    src/Mantid/FitParameterTie.h \
	    src/Mantid/RemoveErrorsDialog.h \
	    src/Mantid/SequentialFitDialog.h \
	    src/Mantid/SelectWorkspacesDialog.h \
	    src/Mantid/InstrumentWidget/GLColor.h \
	    src/Mantid/InstrumentWidget/GLObject.h \
	    src/Mantid/InstrumentWidget/GLTrackball.h \
	    src/Mantid/InstrumentWidget/GLViewport.h \
	    src/Mantid/InstrumentWidget/Instrument3DWidget.h \
            src/Mantid/InstrumentWidget/GL3DWidget.h \
            src/Mantid/InstrumentWidget/OpenGLError.h \
            src/Mantid/InstrumentWidget/GLActor.h \
	    src/Mantid/InstrumentWidget/GLActorCollection.h \
	    src/Mantid/InstrumentWidget/MantidObject.h \
            src/Mantid/InstrumentWidget/TexObject.h \
	    src/Mantid/InstrumentWidget/InstrumentWindow.h \
	    src/Mantid/InstrumentWidget/InstrumentWindowRenderTab.h \
	    src/Mantid/InstrumentWidget/InstrumentWindowPickTab.h \
		src/Mantid/InstrumentWidget/BinDialog.h	\
		src/Mantid/InstrumentWidget/GLGroupPickBox.h \
		src/Mantid/InstrumentWidget/InstrumentTreeWidget.h \
		src/Mantid/InstrumentWidget/InstrumentTreeModel.h \
		src/Mantid/InstrumentWidget/ICompAssemblyActor.h \
		src/Mantid/InstrumentWidget/CompAssemblyActor.h \
                src/Mantid/InstrumentWidget/ObjCompAssemblyActor.h \
		src/Mantid/InstrumentWidget/ObjComponentActor.h \
		src/Mantid/InstrumentWidget/InstrumentActor.h \
		src/Mantid/InstrumentWidget/RectangularDetectorActor.h \
		src/Mantid/InstrumentWidget/MantidColorMap.h \
		src/Mantid/InstrumentWidget/ColorMapWidget.h \
                src/Mantid/InstrumentWidget/UnwrappedSurface.h \
                src/Mantid/InstrumentWidget/UnwrappedCylinder.h \
                src/Mantid/InstrumentWidget/UnwrappedSphere.h \
                src/Mantid/InstrumentWidget/OneCurvePlot.h \
                src/Mantid/InstrumentWidget/CollapsiblePanel.h \
                src/Mantid/InstrumentWidget/DetSelector.h \
                src/Mantid/ManageCustomMenus.h

###################### FORMS ##############################################

#FORMS += src/Mantid/WorkspaceMgr.ui
FORMS += src/Mantid/UserFitFunctionDialog.ui
FORMS += src/Mantid/MantidAbout.ui
FORMS += src/Mantid/RemoveErrorsDialog.ui
FORMS += src/Mantid/SequentialFitDialog.ui
FORMS += src/Mantid/ManageCustomMenus.ui

###################### SOURCES ##############################################

SOURCES  += src/ApplicationWindow.cpp \
            src/Graph.cpp \
            src/Graph3D.cpp \
            src/Table.cpp \
            src/SetColValuesDialog.cpp \
            src/CurvesDialog.cpp \
            src/PlotDialog.cpp \
            src/Plot3DDialog.cpp \
            src/PlotWizard.cpp \
            src/ExportDialog.cpp \
            src/AxesDialog.cpp \
            src/PolynomFitDialog.cpp \
            src/TableDialog.cpp \
            src/TextDialog.cpp \
            src/ScalePicker.cpp\
            src/TitlePicker.cpp \
            src/CanvasPicker.cpp \
            src/ExpDecayDialog.cpp \
            src/FunctionDialog.cpp \
            src/FitDialog.cpp \
            src/SurfaceDialog.cpp \
            src/LineDialog.cpp \
            src/PlotCurve.cpp \
            src/QwtErrorPlotCurve.cpp \
            src/QwtPieCurve.cpp \
            src/ErrDialog.cpp \
            src/LegendWidget.cpp \
            src/ArrowMarker.cpp \
            src/ImageMarker.cpp \
            src/ImageDialog.cpp \
            src/MultiLayer.cpp\
            src/LayerDialog.cpp \
            src/IntDialog.cpp \
            src/SortDialog.cpp\
            src/Bar.cpp \
            src/Cone3D.cpp \
            src/DataSetDialog.cpp \
            src/ConfigDialog.cpp \
            src/QwtBarCurve.cpp \
            src/BoxCurve.cpp \
            src/QwtHistogram.cpp \
            src/VectorCurve.cpp \
            src/Matrix.cpp \
            src/MatrixDialog.cpp \
            src/MatrixSizeDialog.cpp \
            src/MatrixValuesDialog.cpp \
            src/MyParser.cpp\
            src/ColorBox.cpp \
            src/SymbolBox.cpp \
            src/PatternBox.cpp \
            src/importOPJ.cpp\
            src/main.cpp \
            src/SymbolDialog.cpp \
            src/Plot.cpp \
            src/ColorButton.cpp \
            src/AssociationsDialog.cpp \
            src/RenameWindowDialog.cpp \
			src/MdiSubWindow.cpp \
            src/InterpolationDialog.cpp\
            src/nrutil.cpp\
            src/fit_gsl.cpp\
            src/SmoothCurveDialog.cpp\
            src/FilterDialog.cpp\
            src/FFTDialog.cpp\
            src/Note.cpp\
            src/Folder.cpp\
            src/FindDialog.cpp\
            src/TextFormatButtons.cpp\
            src/ScriptEdit.cpp\
            src/ScriptEditor.cpp\
            src/ImportASCIIDialog.cpp\
            src/ImageExportDialog.cpp\
            src/ScaleDraw.cpp\
            src/FunctionCurve.cpp\
            src/Fit.cpp\
            src/MultiPeakFit.cpp\
            src/ExponentialFit.cpp\
            src/PolynomialFit.cpp\
            src/PluginFit.cpp\
            src/NonLinearFit.cpp\
            src/SigmoidalFit.cpp\
			src/LogisticFit.cpp\
            src/ScriptingEnv.cpp\
            src/Scripted.cpp\
            src/Script.cpp\
            src/ScriptingLangDialog.cpp\
            src/TableStatistics.cpp\
            src/Spectrogram.cpp\
            src/ColorMapEditor.cpp\
			src/ColorMapDialog.cpp\
            src/SelectionMoveResizer.cpp\
            src/Filter.cpp\
            src/Differentiation.cpp\
            src/Integration.cpp\
            src/Interpolation.cpp\
            src/SmoothFilter.cpp\
            src/FFTFilter.cpp\
            src/FFT.cpp\
            src/Convolution.cpp\
            src/Correlation.cpp\
            src/ScreenPickerTool.cpp\
            src/DataPickerTool.cpp\
            src/RangeSelectorTool.cpp\
            src/TranslateCurveTool.cpp\
            src/MultiPeakFitTool.cpp\
            src/CurveRangeDialog.cpp\
            src/LineProfileTool.cpp\
            src/PlotEnrichement.cpp\
            src/ExtensibleFileDialog.cpp\
            src/OpenProjectDialog.cpp\
            src/Grid.cpp\
            src/MatrixModel.cpp\
            src/FitModelHandler.cpp \
            src/TextEditor.cpp \
            src/CustomActionDialog.cpp \
            src/DoubleSpinBox.cpp\
            src/MatrixCommand.cpp \
            src/UserFunction.cpp \
            src/ContourLinesEditor.cpp \
            src/PenStyleBox.cpp \
            src/ScriptingWindow.cpp\
            src/ScriptManagerWidget.cpp\
            src/Mantid/MantidApplication.cpp \
	    src/Mantid/LoadRawDlg.cpp \
	    src/Mantid/ExecuteAlgorithm.cpp \
	    src/Mantid/ImportWorkspaceDlg.cpp \
	    src/Mantid/AbstractMantidLog.cpp \
	    src/Mantid/MantidLog.cpp \
	    src/Mantid/MantidUI.cpp \
	    src/Mantid/MantidMatrix.cpp \
	    src/Mantid/MantidDock.cpp \
	    src/Mantid/AlgMonitor.cpp \
	    src/Mantid/MantidAbout.cpp \
	    src/Mantid/InputHistory.cpp \
	    src/Mantid/Preferences.cpp \
        src/Mantid/MantidSampleLogDialog.cpp \
        src/Mantid/AlgorithmHistoryWindow.cpp\
        src/Mantid/MantidMatrixDialog.cpp \
	    src/Mantid/PeakPickerTool.cpp \
	    src/Mantid/MantidCurve.cpp \
        src/Mantid/WorkspaceObserver.cpp \
	    src/Mantid/UserFitFunctionDialog.cpp \
	    src/Mantid/FitPropertyBrowser.cpp \
	    src/Mantid/PropertyHandler.cpp \
	    src/Mantid/IFunctionWrapper.cpp \
	    src/Mantid/FitParameterTie.cpp \
	    src/Mantid/RemoveErrorsDialog.cpp \
	    src/Mantid/SequentialFitDialog.cpp \
	    src/Mantid/SelectWorkspacesDialog.cpp \
	    src/Mantid/InstrumentWidget/GLColor.cpp \
	    src/Mantid/InstrumentWidget/GLObject.cpp \
	    src/Mantid/InstrumentWidget/GLTrackball.cpp \
	    src/Mantid/InstrumentWidget/GLViewport.cpp \
	    src/Mantid/InstrumentWidget/Instrument3DWidget.cpp \
	    src/Mantid/InstrumentWidget/GL3DWidget.cpp \
            src/Mantid/InstrumentWidget/OpenGLError.cpp \
            src/Mantid/InstrumentWidget/GLActor.cpp \
	    src/Mantid/InstrumentWidget/GLActorCollection.cpp \
	    src/Mantid/InstrumentWidget/MantidObject.cpp \
            src/Mantid/InstrumentWidget/TexObject.cpp \
	    src/Mantid/InstrumentWidget/InstrumentWindow.cpp \
	    src/Mantid/InstrumentWidget/InstrumentWindowRenderTab.cpp \
	    src/Mantid/InstrumentWidget/InstrumentWindowPickTab.cpp \
		src/Mantid/InstrumentWidget/BinDialog.cpp  \
		src/Mantid/InstrumentWidget/GLGroupPickBox.cpp \
		src/Mantid/InstrumentWidget/InstrumentTreeWidget.cpp \
		src/Mantid/InstrumentWidget/InstrumentTreeModel.cpp \
                src/Mantid/InstrumentWidget/ICompAssemblyActor.cpp	\
                src/Mantid/InstrumentWidget/CompAssemblyActor.cpp	\
                src/Mantid/InstrumentWidget/ObjCompAssemblyActor.cpp	\
		src/Mantid/InstrumentWidget/ObjComponentActor.cpp	\	
		src/Mantid/InstrumentWidget/InstrumentActor.cpp \
		src/Mantid/InstrumentWidget/RectangularDetectorActor.cpp \
		src/Mantid/InstrumentWidget/MantidColorMap.cpp \
		src/Mantid/InstrumentWidget/ColorMapWidget.cpp \
                src/Mantid/InstrumentWidget/UnwrappedSurface.cpp \
                src/Mantid/InstrumentWidget/UnwrappedCylinder.cpp \
                src/Mantid/InstrumentWidget/UnwrappedSphere.cpp \
                src/Mantid/InstrumentWidget/OneCurvePlot.cpp \
                src/Mantid/InstrumentWidget/CollapsiblePanel.cpp \
                src/Mantid/InstrumentWidget/DetSelector.cpp \
                src/Mantid/ManageCustomMenus.cpp \
		src/pixmaps.cpp


###############################################################
##################### Compression (zlib123) ###################
###############################################################

SOURCES += src/zlib123/minigzip.c

###############################################################
################# Origin Import (liborigin) ###################
###############################################################

HEADERS += src/origin/OPJFile.h
SOURCES += src/origin/OPJFile.cpp

###############################################################
################# Module: Plot 2D #############################
###############################################################

    HEADERS += src/plot2D/ScaleEngine.h
    SOURCES += src/plot2D/ScaleEngine.cpp

###############################################################
################# Module: FFT 2D ##############################
###############################################################

    HEADERS += src/analysis/fft2D.h
    SOURCES += src/analysis/fft2D.cpp

###############################################################
################# Restricted Module: EmfEngine ################
###############################################################

#contains(RESTRICTED_MODULES, EMF) {
#	DEFINES += EMF_OUTPUT

   # INCLUDEPATH += ../3rdparty/libEMF/include
	#unix:LIBS += -L../3rdparty/libEMF/lib
#	win32:LIBS += -lgdi32

#	INCLUDEPATH += ../3rdparty/EmfEngine
  #  HEADERS += ../3rdparty/EmfEngine/EmfEngine.h
   # SOURCES += ../3rdparty/EmfEngine/EmfEngine.cpp
#}

###############################################################
##################### SCRIPTING LANGUAGES SECTION #############
###############################################################

##################### Default: muParser v1.28 #################

contains(SCRIPTING_LANGS, muParser) {
  DEFINES += SCRIPTING_MUPARSER

  HEADERS += src/muParserScript.h \
             src/muParserScripting.h \

  SOURCES += src/muParserScript.cpp \
             src/muParserScripting.cpp
}

##################### PYTHON + SIP + PyQT #####################

contains(SCRIPTING_LANGS, Python) {
 
  contains(CONFIG, CustomInstall){
  	INSTALLS += pythonconfig
  	pythonconfig.files += qtiplotrc.py \
  						qtiUtil.py

  	unix: pythonconfig.path = /usr/local/qtiplot
  	win32: pythonconfig.path = $$INSTALLBASE
  	DEFINES += PYTHON_CONFIG_PATH="\\\"$$replace(pythonconfig.path," ","\ ")\\\"
  }
  
  DEFINES += SCRIPTING_PYTHON

  HEADERS += src/PythonSystemHeader.h src/PythonScript.h src/PythonScripting.h
  SOURCES += src/PythonScript.cpp src/PythonScripting.cpp

  unix {
    INCLUDEPATH += $$system(python python-includepath.py)
    LIBS        += $$system(python -c "\"from distutils import sysconfig; print '-lpython'+sysconfig.get_config_var('VERSION')\"")
    LIBS        += -lm
    system(mkdir -p $${SIP_DIR})
    system($$system(python python-sipcmd.py) -c $${SIP_DIR} src/qti.sip)
  }

  win32 {
    INCLUDEPATH += $$system(call python python-includepath.py)
    #LIBS        += $$system(call python python-libs-win.py)
    system($$system(call python python-sipcmd.py) -c $${SIP_DIR} -w src/qti.sip)
  }

##################### SIP generated files #####################

  SOURCES += $${SIP_DIR}/sipqticmodule.cpp\
             $${SIP_DIR}/sipqtiApplicationWindow.cpp\
             $${SIP_DIR}/sipqtiGraph.cpp\
             $${SIP_DIR}/sipqtiGraphOptions.cpp\
             $${SIP_DIR}/sipqtiGraph3D.cpp\
             $${SIP_DIR}/sipqtiArrowMarker.cpp\
			 $${SIP_DIR}/sipqtiImageMarker.cpp\
			 $${SIP_DIR}/sipqtiLegendWidget.cpp\
			 $${SIP_DIR}/sipqtiGrid.cpp\
             $${SIP_DIR}/sipqtiMultiLayer.cpp\
             $${SIP_DIR}/sipqtiTable.cpp\
             $${SIP_DIR}/sipqtiMatrix.cpp\
             $${SIP_DIR}/sipqtiMdiSubWindow.cpp\
             $${SIP_DIR}/sipqtiNote.cpp\
             $${SIP_DIR}/sipqtiPythonScript.cpp\
             $${SIP_DIR}/sipqtiPythonScripting.cpp\
             $${SIP_DIR}/sipqtiFolder.cpp\
             $${SIP_DIR}/sipqtiQList*.cpp\
             $${SIP_DIR}/sipqtiFit.cpp \
             $${SIP_DIR}/sipqtiExponentialFit.cpp \
             $${SIP_DIR}/sipqtiTwoExpFit.cpp \
             $${SIP_DIR}/sipqtiThreeExpFit.cpp \
             $${SIP_DIR}/sipqtiSigmoidalFit.cpp \
			 $${SIP_DIR}/sipqtiLogisticFit.cpp \
             $${SIP_DIR}/sipqtiGaussAmpFit.cpp \
             $${SIP_DIR}/sipqtiLorentzFit.cpp \
             $${SIP_DIR}/sipqtiNonLinearFit.cpp \
             $${SIP_DIR}/sipqtiPluginFit.cpp \
             $${SIP_DIR}/sipqtiMultiPeakFit.cpp \
             $${SIP_DIR}/sipqtiPolynomialFit.cpp \
             $${SIP_DIR}/sipqtiLinearFit.cpp \
             $${SIP_DIR}/sipqtiGaussFit.cpp \
             $${SIP_DIR}/sipqtiFilter.cpp \
             $${SIP_DIR}/sipqtiDifferentiation.cpp \
             $${SIP_DIR}/sipqtiIntegration.cpp \
			 $${SIP_DIR}/sipqtiInterpolation.cpp \
			 $${SIP_DIR}/sipqtiSmoothFilter.cpp \
			 $${SIP_DIR}/sipqtiFFTFilter.cpp \
			 $${SIP_DIR}/sipqtiFFT.cpp \
			 $${SIP_DIR}/sipqtiCorrelation.cpp \
			 $${SIP_DIR}/sipqtiConvolution.cpp \
			 $${SIP_DIR}/sipqtiDeconvolution.cpp \
             $${SIP_DIR}/sipqtiMantidMatrix.cpp\
             $${SIP_DIR}/sipqtiMantidUI.cpp \
             $${SIP_DIR}/sipqtiInstrumentWindow.cpp

}
###############################################################
