set ( SRC_FILES
	DPDFBackgroundRemover.cpp
	DPDFDisplayControl.cpp
	DPDFFitControl.cpp
	DPDFFitOptionsBrowser.cpp
	DPDFFourierTransform.cpp
	DPDFInputDataControl.cpp
	DisplayCurveFitTest.cpp
	SliceSelector.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
# IMPORTANT: Include files are required in the MOC_FILES set. Scroll down to find it.
set ( INC_FILES
	DllConfig.h
	DPDFBackgroundRemover.h
	DPDFDisplayControl.h
	DPDFFitControl.h
	DPDFFitOptionsBrowser.h
	DPDFFourierTransform.h
	DPDFInputDataControl.h
	DisplayCurveFitTest.h
	SliceSelector.h
)

set ( MOC_FILES 
    DisplayCurveFitTest.h
    DPDFBackgroundRemover.h
    DPDFDisplayControl.h
    DPDFFitControl.h
    DPDFFitOptionsBrowser.h
    DPDFFourierTransform.h
    DPDFInputDataControl.h
    SliceSelector.h
)

set ( UI_FILES 
    DisplayCurveFitTest.ui
    DPDFBackgroundRemover.ui
    DPDFFitControl.ui
    DPDFFourierTransform.ui
    SliceSelector.ui
)

mtd_add_qt_library (TARGET_NAME MantidScientificInterfacesDynamicPDF
                    SRC ${SRC_FILES}
                    MOC ${MOC_FILES}
                    NOMOC ${INC_FILES}
                    UI ${UI_FILES}
                    DEFS IN_MANTIDQT_DYNAMICPDF
                    PERECOMPILED PrecompiledHeader.h
                    LINK_LIBS
                        ${TCMALLOC_LIBRARIES_LINKTIME}
                        ${CORE_MANTIDLIBS}
                        ${POCO_LIBRARIES}
                        ${Boost_LIBRARIES}
                        ${QT_LIBRARIES}
                        ${QWT_LIBRARIES}
                        ${OPENGL_LIBRARIES}
                    MTD_QT_LINK_LIBS
                        MantidQtWidgetsCommon
                    INSTALL_DIR ${PLUGINS_DIR}/qtplugins/mantid
                    OSX_INSTALL_RPATH "@loader_path/../../../Contents/MacOS"
)

if ( MSVC_IDE )
  # Add to the 'ScientificInterfaces' group in VS
  set_property ( TARGET MantidScientificInterfacesDynamicPDFQt4 PROPERTY FOLDER "ScientificInterfaces" )
endif()
