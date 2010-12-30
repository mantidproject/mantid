file ( GLOB SRC_FILES src/*.cpp )

file ( GLOB MOC_FILES inc/MantidQtCustomDialogs/*.h )

file ( GLOB UI_FILES inc/MantidQtCustomDialogs/*.ui )

include_directories ( inc )

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} ) 
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

add_library ( CustomDialogs ${SRC_FILES} ${MOCCED_FILES} ${UI_HDRS} )
# Set the name of the generated library
set_target_properties ( CustomDialogs PROPERTIES OUTPUT_NAME MantidQtCustomDialogs )
# Add to the 'MantidQt' group in VS
set_property ( TARGET CustomDialogs PROPERTY FOLDER "MantidQt" )

target_link_libraries ( CustomDialogs MantidQtAPI )