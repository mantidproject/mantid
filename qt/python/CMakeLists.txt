###########################################################################
# Do the sip generation, for python bindings
###########################################################################

include_directories ( ${PYTHON_INCLUDE_PATH} )

set ( SIP_SPEC ${CMAKE_CURRENT_SOURCE_DIR}/mantidqt.sip )
set ( SIP_SRC_IN ${CMAKE_CURRENT_SOURCE_DIR}/sip_mantidqt.cpp.in )
set ( SIP_SRC ${CMAKE_CURRENT_BINARY_DIR}/sip_mantidqt.cpp )
set ( SIP_SRC_AUTO sipmantidqtpythonpart0.cpp )

# Sip needs to have a dependency on all headers within it
set ( SIP_HDRS
  ../widgets/common/inc/MantidQtWidgets/Common/PythonSystemHeader.h
  ../widgets/common/inc/MantidQtWidgets/Common/WorkspaceObserver.h
  ../widgets/common/inc/MantidQtWidgets/Common/GraphOptions.h
  ../widgets/common/inc/MantidQtWidgets/Common/AlgorithmDialog.h
  ../widgets/common/inc/MantidQtWidgets/Common/UserSubWindow.h
  ../widgets/common/inc/MantidQtWidgets/Common/InterfaceManager.h
  ../widgets/common/inc/MantidQtWidgets/Common/MantidDesktopServices.h
  ../widgets/common/inc/MantidQtWidgets/Common/MWRunFiles.h
  ../widgets/common/inc/MantidQtWidgets/Common/WidgetScrollbarDecorator.h
  ../widgets/sliceviewer/inc/MantidQtWidgets/SliceViewer/SliceViewerWindow.h
  ../widgets/sliceviewer/inc/MantidQtWidgets/SliceViewer/LineViewer.h
  ../widgets/sliceviewer/inc/MantidQtWidgets/SliceViewer/PeaksPresenter.h
  ../widgets/sliceviewer/inc/MantidQtWidgets/SliceViewer/ProxyCompositePeaksPresenter.h
  ../widgets/sliceviewer/inc/MantidQtWidgets/SliceViewer/PeaksPresenter.h
  ../widgets/sliceviewer/inc/MantidQtWidgets/SliceViewer/SliceViewer.h
  ../widgets/factory/inc/MantidQtWidgets/Factory/WidgetFactory.h
  ../widgets/refdetectorview/inc/MantidQtWidgets/RefDetectorView/RefIVConnections.h
  ../widgets/refdetectorview/inc/MantidQtWidgets/RefDetectorView/RefMatrixWSImageView.h
  ../widgets/common/inc/MantidQtWidgets/Common/FitPropertyBrowser.h
  ../widgets/common/inc/MantidQtWidgets/Common/DataProcessorUI/DataProcessorWhiteList.h
  ../widgets/common/inc/MantidQtWidgets/Common/DataProcessorUI/DataProcessorPreprocessMap.h
  ../widgets/common/inc/MantidQtWidgets/Common/DataProcessorUI/DataProcessorProcessingAlgorithm.h
  ../widgets/common/inc/MantidQtWidgets/Common/DataProcessorUI/DataProcessorPostprocessingAlgorithm.h
  ../widgets/common/inc/MantidQtWidgets/Common/DataProcessorUI/QDataProcessorWidget.h
  ../widgets/common/inc/MantidQtWidgets/Common/DataProcessorUI/DataProcessorMainPresenter.h
  ../widgets/common/inc/MantidQtWidgets/Common/DataProcessorUI/AppendRowCommand.h
  ../widgets/common/inc/MantidQtWidgets/Common/DataProcessorUI/AppendGroupCommand.h
  ../widgets/common/inc/MantidQtWidgets/Common/DataProcessorUI/ClearSelectedCommand.h
  ../widgets/common/inc/MantidQtWidgets/Common/DataProcessorUI/CopySelectedCommand.h
  ../widgets/common/inc/MantidQtWidgets/Common/DataProcessorUI/CutSelectedCommand.h
  ../widgets/common/inc/MantidQtWidgets/Common/DataProcessorUI/DeleteGroupCommand.h
  ../widgets/common/inc/MantidQtWidgets/Common/DataProcessorUI/DeleteRowCommand.h
  ../widgets/common/inc/MantidQtWidgets/Common/DataProcessorUI/ExpandCommand.h
  ../widgets/common/inc/MantidQtWidgets/Common/DataProcessorUI/ExportTableCommand.h
  ../widgets/common/inc/MantidQtWidgets/Common/DataProcessorUI/GroupRowsCommand.h
  ../widgets/common/inc/MantidQtWidgets/Common/DataProcessorUI/ImportTableCommand.h
  ../widgets/common/inc/MantidQtWidgets/Common/DataProcessorUI/NewTableCommand.h
  ../widgets/common/inc/MantidQtWidgets/Common/DataProcessorUI/OpenTableCommand.h
  ../widgets/common/inc/MantidQtWidgets/Common/DataProcessorUI/OptionsCommand.h
  ../widgets/common/inc/MantidQtWidgets/Common/DataProcessorUI/PasteSelectedCommand.h
  ../widgets/common/inc/MantidQtWidgets/Common/DataProcessorUI/PlotGroupCommand.h
  ../widgets/common/inc/MantidQtWidgets/Common/DataProcessorUI/PlotRowCommand.h
  ../widgets/common/inc/MantidQtWidgets/Common/DataProcessorUI/ProcessCommand.h
  ../widgets/common/inc/MantidQtWidgets/Common/DataProcessorUI/SaveTableCommand.h
  ../widgets/common/inc/MantidQtWidgets/Common/DataProcessorUI/SaveTableAsCommand.h
  ../widgets/common/inc/MantidQtWidgets/Common/DataProcessorUI/SeparatorCommand.h
  ../widgets/common/inc/MantidQtWidgets/Common/DataProcessorUI/DataProcessorWorkspaceCommand.h
  ../widgets/common/inc/MantidQtWidgets/Common/SlitCalculator.h
  ../widgets/instrumentview/inc/MantidQtWidgets/InstrumentView/InstrumentWidgetTab.h
  ../widgets/instrumentview/inc/MantidQtWidgets/InstrumentView/InstrumentWidget.h
  ../widgets/instrumentview/inc/MantidQtWidgets/InstrumentView/InstrumentWidgetRenderTab.h
  ../widgets/instrumentview/inc/MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h
  ../widgets/instrumentview/inc/MantidQtWidgets/InstrumentView/InstrumentWidgetMaskTab.h
  ../widgets/instrumentview/inc/MantidQtWidgets/InstrumentView/InstrumentWidgetTreeTab.h
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
include_directories ( ../widgets/sliceviewer/inc )
include_directories ( ../widgets/common/inc )
include_directories ( ../widgets/instrumentview/inc )
include_directories ( ../widgets/factory/inc )
include_directories ( ../widgets/spectrumviewer/inc )
include_directories ( ../widgets/refdetectorview/inc )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR}/../widgets/sliceviewer ) # to find the ui_*.h auto-generated files
include_directories ( ${CMAKE_CURRENT_BINARY_DIR}/../widgets/refdetectorview ) # to find the ui_*.h auto-generated files

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
            MantidQtWidgetsCommon
            MantidQtWidgetsSliceViewer
            MantidQtWidgetsFactory
            MantidQtWidgetsRefDetectorView
            MantidQtWidgetsInstrumentView
            ${CORE_MANTIDLIBS}
            ${POCO_LIBRARIES}
            ${Boost_LIBRARIES}
            ${QT_LIBRARIES}
            ${QWT_LIBRARIES}
            ${PYTHON_LIBRARIES}
            )


###########################################################################
# Installation settings
###########################################################################

install ( TARGETS mantidqtpython DESTINATION ${BIN_DIR} )
