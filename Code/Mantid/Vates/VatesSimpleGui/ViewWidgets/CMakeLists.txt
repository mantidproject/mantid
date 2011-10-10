project( MantidVatesSimpleGuiViewWidgets )

# These are the C++ files to be compiled.
set( INCLUDE_FILES
  inc/MantidVatesSimpleGuiViewWidgets/ColorSelectionWidget.h
  inc/MantidVatesSimpleGuiViewWidgets/ColorUpdater.h
  inc/MantidVatesSimpleGuiViewWidgets/MdViewerWidget.h
  inc/MantidVatesSimpleGuiViewWidgets/MultisliceView.h
  inc/MantidVatesSimpleGuiViewWidgets/StandardView.h
  inc/MantidVatesSimpleGuiViewWidgets/SplatterPlotView.h
  inc/MantidVatesSimpleGuiViewWidgets/ThreesliceView.h
  inc/MantidVatesSimpleGuiViewWidgets/TimeControlWidget.h
  inc/MantidVatesSimpleGuiViewWidgets/ViewBase.h
  inc/MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h
)

set( SOURCE_FILES
  src/ColorSelectionWidget.cpp
  src/ColorUpdater.cpp
  src/MdViewerWidget.cpp
  src/MultisliceView.cpp
  src/StandardView.cpp
  src/SplatterPlotView.cpp
  src/ThreesliceView.cpp
  src/TimeControlWidget.cpp
  src/ViewBase.cpp
)

# These are the headers to be preprocessed using
# Qt's moc preprocessor.
qt4_wrap_cpp( MOC_SOURCES
  inc/MantidVatesSimpleGuiViewWidgets/ColorSelectionWidget.h
  inc/MantidVatesSimpleGuiViewWidgets/MdViewerWidget.h
  inc/MantidVatesSimpleGuiViewWidgets/MultisliceView.h
  inc/MantidVatesSimpleGuiViewWidgets/StandardView.h
  inc/MantidVatesSimpleGuiViewWidgets/SplatterPlotView.h
  inc/MantidVatesSimpleGuiViewWidgets/ThreesliceView.h
  inc/MantidVatesSimpleGuiViewWidgets/TimeControlWidget.h
  inc/MantidVatesSimpleGuiViewWidgets/ViewBase.h
)

# These are the ui files to be processed using
# Qt's ui file processor.
qt4_wrap_ui( UI_BUILT_SOURCES
  inc/MantidVatesSimpleGuiViewWidgets/ColorSelectionWidget.ui
  inc/MantidVatesSimpleGuiViewWidgets/MdViewerWidget.ui
  inc/MantidVatesSimpleGuiViewWidgets/MultisliceView.ui
  inc/MantidVatesSimpleGuiViewWidgets/StandardView.ui
  inc/MantidVatesSimpleGuiViewWidgets/SplatterPlotView.ui
  inc/MantidVatesSimpleGuiViewWidgets/ThreesliceView.ui
  inc/MantidVatesSimpleGuiViewWidgets/TimeControlWidget.ui
)

# Setup the include locations.
include_directories(
  inc
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/../QtWidgets
)

# For Windows
add_definitions( -DIN_MANTID_VATES_SIMPLEGUI_VIEWWIDGETS )
# Add the target for this directory
add_library( VatesSimpleGuiViewWidgets ${INCLUDE_FILES} ${SOURCE_FILES}
${MOC_SOURCES} ${UI_BUILT_SOURCES} )
# Set the name of the generated library
set_target_properties( VatesSimpleGuiViewWidgets PROPERTIES OUTPUT_NAME MantidVatesSimpleGuiViewWidgets )
# Add to the 'VatesSimpleGui' group in VS
set_property( TARGET VatesSimpleGuiViewWidgets PROPERTY FOLDER MantidVatesSimpleGui )

target_link_libraries( VatesSimpleGuiViewWidgets
VatesSimpleGuiQtWidgets
pqApplicationComponents
pqComponents
${QT_LIBRARIES}
MantidQtAPI
)

install( TARGETS VatesSimpleGuiViewWidgets ${TARGET_TYPE} DESTINATION ${LIB_DIR} )
