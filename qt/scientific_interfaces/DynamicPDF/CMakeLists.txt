set ( SRC_FILES
	DPDFBackgroundRemover.cpp
	DPDFDisplayControl.cpp
	DPDFFitControl.cpp
	DPDFFitOptionsBrowser.cpp
	DPDFFourierTransform.cpp
	DPDFInputDataControl.cpp
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
	SliceSelector.h
)

set ( MOC_FILES
    DPDFBackgroundRemover.h
    DPDFDisplayControl.h
    DPDFFitControl.h
    DPDFFitOptionsBrowser.h
    DPDFFourierTransform.h
    DPDFInputDataControl.h
    SliceSelector.h
)


set ( UI_FILES
    DPDFBackgroundRemover.ui
    DPDFFitControl.ui
    DPDFFourierTransform.ui
    SliceSelector.ui
)

mtd_add_qt_library (TARGET_NAME MantidScientificInterfacesDynamicPDF
  QT_VERSION 4
  SRC ${SRC_FILES}
  MOC ${MOC_FILES}
  NOMOC ${INC_FILES}
  UI ${UI_FILES}
  DEFS IN_MANTIDQT_DYNAMICPDF
  PRECOMPILED PrecompiledHeader.h
  INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
  SYSTEM_INCLUDE_DIRS
    ${Boost_INCLUDE_DIRS}
  LINK_LIBS
    ${TCMALLOC_LIBRARIES_LINKTIME}
    ${CORE_MANTIDLIBS}
    ${POCO_LIBRARIES}
    ${Boost_LIBRARIES}
    ${OPENGL_LIBRARIES}
  QT4_LINK_LIBS
    Qwt5
  MTD_QT_LINK_LIBS
    MantidQtWidgetsCommon
    MantidQtWidgetsLegacyQwt
  INSTALL_DIR_BASE
    ${PLUGINS_DIR}
  OSX_INSTALL_RPATH
    @loader_path/../../Contents/MacOS
  LINUX_INSTALL_RPATH
    "\$ORIGIN/../../${LIB_DIR}"
)
