file ( GLOB SRC_FILES src/*.cpp )

file ( GLOB MOC_FILES inc/MantidQtMantidWidgets/*.h )

file ( GLOB UI_FILES inc/MantidQtMantidWidgets/*.ui )

include_directories ( inc )
include_directories ( ${QWT_INCLUDE_DIR} )

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} ) 
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

# For Windows:
add_definitions ( -DIN_MANTIDQT_MANTIDWIDGETS )

add_library ( MantidWidgets ${SRC_FILES} ${MOCCED_FILES} ${UI_HDRS} )
# Add to the 'MantidQt' group in VS
set_property ( TARGET MantidWidgets PROPERTY FOLDER "MantidQt" )

target_link_libraries ( MantidWidgets MantidQtAPI ${QT_LIBRARIES} ${QWT_LIBRARIES} )