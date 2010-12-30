file ( GLOB SRC_FILES src/*.cpp )

file ( GLOB MOC_FILES inc/MantidQtCustomInterfaces/*.h )

file ( GLOB UI_FILES inc/MantidQtCustomInterfaces/*.ui )

include_directories ( inc )
include_directories ( ../MantidWidgets/inc )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR}/../MantidWidgets )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR}/../API )
include_directories ( ${QWT_INCLUDE_DIR} )
include_directories ( ../../QtPropertyBrowser/src )

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} ) 
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

add_library ( CustomInterfaces ${SRC_FILES} ${MOCCED_FILES} ${UI_HDRS} )
# Set the name of the generated library
set_target_properties ( CustomInterfaces PROPERTIES OUTPUT_NAME MantidQtCustomInterfaces )
# Add to the 'MantidQt' group in VS
set_property ( TARGET CustomInterfaces PROPERTY FOLDER "MantidQt" )

target_link_libraries ( CustomInterfaces MantidQtAPI MantidWidgets QtPropertyBrowser )