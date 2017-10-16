###########################################################################
# Do the sip generation for the legacy python bindings.
# This is a Qt4-only module and serves to support MantidPlot alone.
###########################################################################

include_directories ( ${PYTHON_INCLUDE_PATH} )

set ( SIP_SPEC ${CMAKE_CURRENT_SOURCE_DIR}/mantidqt.sip )
set ( SIP_SRC_IN ${CMAKE_CURRENT_SOURCE_DIR}/sip_mantidqt.cpp.in )
set ( SIP_SRC ${CMAKE_CURRENT_BINARY_DIR}/sip_mantidqt.cpp )
set ( SIP_SRC_AUTO sipmantidqtpythonpart0.cpp )

# Sip needs to have a dependency on all headers within it
set ( WIDGETS_SRC_DIR ../../widgets )
set ( SIP_HDRS
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/PythonSystemHeader.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/WorkspaceObserver.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/GraphOptions.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/AlgorithmDialog.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/UserSubWindow.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/InterfaceManager.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/MantidDesktopServices.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/MWRunFiles.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/WidgetScrollbarDecorator.h
  ${WIDGETS_SRC_DIR}/sliceviewer/inc/MantidQtWidgets/SliceViewer/SliceViewerWindow.h
  ${WIDGETS_SRC_DIR}/sliceviewer/inc/MantidQtWidgets/SliceViewer/LineViewer.h
  ${WIDGETS_SRC_DIR}/sliceviewer/inc/MantidQtWidgets/SliceViewer/PeaksPresenter.h
  ${WIDGETS_SRC_DIR}/sliceviewer/inc/MantidQtWidgets/SliceViewer/ProxyCompositePeaksPresenter.h
  ${WIDGETS_SRC_DIR}/sliceviewer/inc/MantidQtWidgets/SliceViewer/PeaksPresenter.h
  ${WIDGETS_SRC_DIR}/sliceviewer/inc/MantidQtWidgets/SliceViewer/SliceViewer.h
  ${WIDGETS_SRC_DIR}/factory/inc/MantidQtWidgets/Factory/WidgetFactory.h
  ${WIDGETS_SRC_DIR}/refdetectorview/inc/MantidQtWidgets/RefDetectorView/RefIVConnections.h
  ${WIDGETS_SRC_DIR}/refdetectorview/inc/MantidQtWidgets/RefDetectorView/RefMatrixWSImageView.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/FitPropertyBrowser.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/DataProcessorUI/WhiteList.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/DataProcessorUI/PreprocessMap.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/DataProcessorUI/ProcessingAlgorithm.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/DataProcessorUI/PostprocessingAlgorithm.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/DataProcessorUI/QDataProcessorWidget.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/DataProcessorUI/DataProcessorMainPresenter.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/DataProcessorUI/AppendRowCommand.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/DataProcessorUI/AppendGroupCommand.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/DataProcessorUI/ClearSelectedCommand.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/DataProcessorUI/CopySelectedCommand.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/DataProcessorUI/CutSelectedCommand.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/DataProcessorUI/DeleteGroupCommand.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/DataProcessorUI/DeleteRowCommand.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/DataProcessorUI/ExpandCommand.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/DataProcessorUI/ExportTableCommand.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/DataProcessorUI/GroupRowsCommand.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/DataProcessorUI/ImportTableCommand.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/DataProcessorUI/NewTableCommand.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/DataProcessorUI/OpenTableCommand.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/DataProcessorUI/OptionsCommand.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/DataProcessorUI/PasteSelectedCommand.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/DataProcessorUI/PlotGroupCommand.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/DataProcessorUI/PlotRowCommand.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/DataProcessorUI/ProcessCommand.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/DataProcessorUI/SaveTableCommand.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/DataProcessorUI/SaveTableAsCommand.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/DataProcessorUI/SeparatorCommand.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/DataProcessorUI/WorkspaceCommand.h
  ${WIDGETS_SRC_DIR}/common/inc/MantidQtWidgets/Common/SlitCalculator.h
  ${WIDGETS_SRC_DIR}/instrumentview/inc/MantidQtWidgets/InstrumentView/InstrumentWidgetTab.h
  ${WIDGETS_SRC_DIR}/instrumentview/inc/MantidQtWidgets/InstrumentView/InstrumentWidget.h
  ${WIDGETS_SRC_DIR}/instrumentview/inc/MantidQtWidgets/InstrumentView/InstrumentWidgetRenderTab.h
  ${WIDGETS_SRC_DIR}/instrumentview/inc/MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h
  ${WIDGETS_SRC_DIR}/instrumentview/inc/MantidQtWidgets/InstrumentView/InstrumentWidgetMaskTab.h
  ${WIDGETS_SRC_DIR}/instrumentview/inc/MantidQtWidgets/InstrumentView/InstrumentWidgetTreeTab.h
)

set( SRC_UNITY_IGNORE_FILES )

###########################################################################
# Sip generated files
###########################################################################

# The code generated by sip causes compiler warnings therefore the
# generated file is wrapped by ${SIP_SRC} and these warnings are
# disabled. In order for VS2010 to to this correctly the second
# custom command below is required

# Flags used:
# -e : C++ exceptions turn into python exceptions.
# -j1 : split into 1 file
# -w : enable warnings
# -o : automatic docstrings (not supported in old version < 4.10)

add_custom_command ( OUTPUT ${SIP_SRC_AUTO}
                     COMMAND ${SIP_EXECUTABLE}
                          -I ${PYQT4_SIP_DIR} ${PYQT4_SIP_FLAGS}
                          -c ${CMAKE_CURRENT_BINARY_DIR} -j1 -w
                          -e
                          ${SIP_SPEC}
                     DEPENDS mantidqt.sip ${SIP_HDRS}
                     COMMENT "Generating mantidqt python bindings using sip"
)

add_custom_command ( OUTPUT ${SIP_SRC}
                     COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different ${SIP_SRC_IN} ${SIP_SRC}
                     DEPENDS ${SIP_SRC_AUTO}
                     COMMENT ""
)

# Needed for sip.h header that can end up in a different place to to the main Python include directory
include_directories ( SYSTEM ${SIP_INCLUDE_DIR} )

# Needed for sip generated files to find includes in src
include_directories ( ${CMAKE_CURRENT_SOURCE_DIR} )

# Other folders that need to be included...
set ( WIDGETS_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/../../widgets )
include_directories ( ${WIDGETS_SRC_DIR}/sliceviewer/inc )
include_directories ( ${WIDGETS_SRC_DIR}/common/inc )
include_directories ( ${WIDGETS_SRC_DIR}/instrumentview/inc )
include_directories ( ${WIDGETS_SRC_DIR}/factory/inc )
include_directories ( ${WIDGETS_SRC_DIR}/spectrumviewer/inc )
include_directories ( ${WIDGETS_SRC_DIR}/refdetectorview/inc )
include_directories ( ${WIDGETS_BINARY_DIR}/sliceviewer/Qt4 ) # to find the ui_*.h auto-generated files
include_directories ( ${WIDGETS_BINARY_DIR}/refdetectorview/Qt4 ) # to find the ui_*.h auto-generated files

# This creates the target library, just for python bindings
add_library ( mantidqtpython MODULE ${SIP_SRC} )

if(WIN32)
  # Windows: Python library name needs to end in .pyd for Windows
  set_target_properties( mantidqtpython PROPERTIES PREFIX "" SUFFIX ".pyd" )
  # Debug python library expects imported module names to end in _d
  if ( PYTHON_DEBUG_LIBRARY )
    set_target_properties ( mantidqtpython PROPERTIES DEBUG_OUTPUT_NAME mantidqtpython_d )
  endif ()
  # For a debug build copy the special PyQt4 debug build to the bin directory
  add_custom_command (
    TARGET mantidqtpython
    POST_BUILD
    COMMAND if 1==$<CONFIG:Debug> ${CMAKE_COMMAND}
      -E copy_directory ${PYTHON_DIR}/msvc-site-packages/debug/PyQt4 ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/PyQt4
    COMMENT "Copying debug PyQt4 to bin"
  )

elseif ( APPLE )
	# Mac: and in .so on the Mac, with no "lib" prefix either
  	set_target_properties ( mantidqtpython PROPERTIES PREFIX "" SUFFIX .so )

if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties ( mantidqtpython PROPERTIES INSTALL_RPATH "@loader_path/../MacOS")
endif ()

else ()
	# Linux: needs to NOT have the usual "lib" prefix.
	set_target_properties( mantidqtpython PROPERTIES PREFIX "" )
endif ()

# ... and links to other required libs ...
target_link_libraries ( mantidqtpython LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
            MantidQtWidgetsCommonQt4
            MantidQtWidgetsSliceViewerQt4
            MantidQtWidgetsFactoryQt4
            MantidQtWidgetsRefDetectorViewQt4
            MantidQtWidgetsInstrumentViewQt4
            ${CORE_MANTIDLIBS}
            ${POCO_LIBRARIES}
            ${Boost_LIBRARIES}
            ${QT_LIBRARIES}
            ${QWT5_LIBRARIES}
            ${PYTHON_LIBRARIES}
            )


###########################################################################
# Installation settings
###########################################################################

install ( TARGETS mantidqtpython DESTINATION ${BIN_DIR} )
