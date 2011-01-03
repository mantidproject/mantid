file ( GLOB SRC_FILES src/*.cpp )

set ( MOC_FILES inc/MantidQtAPI/AlgorithmDialog.h 
                inc/MantidQtAPI/GenericDialog.h
                inc/MantidQtAPI/ManageUserDirectories.h
                inc/MantidQtAPI/MantidDialog.h
                inc/MantidQtAPI/MantidWidget.h
                inc/MantidQtAPI/PythonRunner.h
                inc/MantidQtAPI/UserSubWindow.h
)

set ( UI_FILES inc/MantidQtAPI/ManageUserDirectories.ui )

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} ) 
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

# For Windows:
add_definitions ( -DIN_MANTIDQT_API )

add_library ( MantidQtAPI ${SRC_FILES} ${MOCCED_FILES} ${UI_HDRS} )
# Add to the 'MantidQt' group in VS
set_property ( TARGET MantidQtAPI PROPERTY FOLDER "MantidQt" )

target_link_libraries ( MantidQtAPI ${CORE_MANTIDLIBS} ${QT_LIBRARIES} )