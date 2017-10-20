set ( SRC_FILES
	src/WidgetFactory.cpp
)

set ( INC_FILES
	inc/MantidQtWidgets/Factory/WidgetFactory.h
)

set ( MOC_FILES 
	inc/MantidQtWidgets/Factory/WidgetFactory.h
)

set ( UI_FILES 
)

set ( TEST_FILES
	test/WidgetFactoryTest.h
)

include_directories ( inc )
include_directories ( ../sliceviewer/inc )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR}/../sliceviewer )

# For Windows:
add_definitions ( -DIN_MANTIDQT_FACTORY )

# Target
mtd_add_qt_library (TARGET_NAME MantidQtWidgetsFactory
  SRC ${SRC_FILES}
  MOC ${MOC_FILES}
  NOMOC ${INC_FILES}
  LINK_LIBS
    ${CORE_MANTIDLIBS}
    ${POCO_LIBRARIES}
    ${Boost_LIBRARIES}
  QT4_LINK_LIBS
    Qwt5
  MTD_QT_LINK_LIBS
    MantidQtWidgetsCommon
    MantidQtWidgetsSliceViewer
)


