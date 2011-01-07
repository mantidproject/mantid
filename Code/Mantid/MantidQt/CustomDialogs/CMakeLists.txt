set ( SRC_FILES src/CreateSampleShapeDialog.cpp
                src/LoadAsciiDialog.cpp
                src/LoadDAEDialog.cpp
                src/LoadRawDialog.cpp
                src/LoginDialog.cpp
                src/LOQScriptInputDialog.cpp
                src/MantidGLWidget.cpp
                src/PlotAsymmetryByLogValueDialog.cpp
                src/SampleShapeHelpers.cpp
)

# Include files aren't required, but this makes them appear in Visual Studio
set ( INC_FILES inc/MantidQtCustomDialogs/CreateSampleShapeDialog.h
                inc/MantidQtCustomDialogs/LoadAsciiDialog.h
                inc/MantidQtCustomDialogs/LoadDAEDialog.h
                inc/MantidQtCustomDialogs/LoadRawDialog.h
                inc/MantidQtCustomDialogs/LoginDialog.h
                inc/MantidQtCustomDialogs/LOQScriptInputDialog.h
                inc/MantidQtCustomDialogs/MantidGLWidget.h
                inc/MantidQtCustomDialogs/PlotAsymmetryByLogValueDialog.h
                inc/MantidQtCustomDialogs/SampleShapeHelpers.h
)

set ( MOC_FILES inc/MantidQtCustomDialogs/CreateSampleShapeDialog.h
                inc/MantidQtCustomDialogs/LoadAsciiDialog.h
                inc/MantidQtCustomDialogs/LoadDAEDialog.h
                inc/MantidQtCustomDialogs/LoadRawDialog.h
                inc/MantidQtCustomDialogs/LoginDialog.h
                inc/MantidQtCustomDialogs/LOQScriptInputDialog.h
                inc/MantidQtCustomDialogs/MantidGLWidget.h
                inc/MantidQtCustomDialogs/PlotAsymmetryByLogValueDialog.h
                inc/MantidQtCustomDialogs/SampleShapeHelpers.h
)

set ( UI_FILES inc/MantidQtCustomDialogs/CreateSampleShapeDialog.ui
               inc/MantidQtCustomDialogs/DiagScriptInputDialog.ui
               inc/MantidQtCustomDialogs/LoginDialog.ui
               inc/MantidQtCustomDialogs/LOQScriptInputDialog.ui
               inc/MantidQtCustomDialogs/PlotAsymmetryByLogValueDialog.ui
)

include_directories ( inc )

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} ) 
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

add_library ( CustomDialogs ${SRC_FILES} ${INC_FILES} ${MOCCED_FILES} ${UI_HDRS} )
# Set the name of the generated library
set_target_properties ( CustomDialogs PROPERTIES OUTPUT_NAME MantidQtCustomDialogs )
# Add to the 'MantidQt' group in VS
set_property ( TARGET CustomDialogs PROPERTY FOLDER "MantidQt" )

target_link_libraries ( CustomDialogs MantidQtAPI )