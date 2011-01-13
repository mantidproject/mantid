file ( GLOB QTIPLOT_SRCS src/*.cpp
                         src/origin/OPJFile.cpp
                         src/plot2D/ScaleEngine.cpp
                         src/analysis/fft2D.cpp
                         src/zlib123/minigzip.c
)

file ( GLOB MANTID_SRCS src/Mantid/*.cpp src/Mantid/InstrumentWidget/*.cpp )

# Do the sip generation.

find_package ( PyQt4 REQUIRED )
find_package ( SIP REQUIRED )
separate_arguments ( PYQT4_SIP_FLAGS )
set ( SIP_SPEC ${CMAKE_CURRENT_SOURCE_DIR}/src/qti.sip )
set ( SIP_SRC sipqtipart0.cpp )
# TODO: I think we need to explicitly add in all dependencies here
add_custom_command ( OUTPUT ${SIP_SRC}
                     COMMAND ${SIP_EXECUTABLE}
                          -I ${PYQT4_SIP_DIR} ${PYQT4_SIP_FLAGS}
                          -c ${CMAKE_CURRENT_BINARY_DIR} -j1 -w 
                          ${SIP_SPEC}
                     DEPENDS src/qti.sip
                     COMMENT "Generating python bindings using sip"
)

# Needed for sip generated files to find includes in src
include_directories ( ${CMAKE_CURRENT_SOURCE_DIR} )

set ( SRC_FILES ${QTIPLOT_SRCS} ${MANTID_SRCS} ${SIP_SRC} )

file ( GLOB MOC_FILES src/*.h src/Mantid/*.h src/Mantid/InstrumentWidget/*.h )

set ( UI_FILES src/Mantid/UserFitFunctionDialog.ui
               src/Mantid/MantidAbout.ui
               src/Mantid/RemoveErrorsDialog.ui
               src/Mantid/SequentialFitDialog.ui
               src/Mantid/ManageCustomMenus.ui
)

qt4_wrap_cpp ( MOCCED_FILES ${MOC_FILES} )

qt4_wrap_ui ( UI_HDRS ${UI_FILES} )
include_directories ( ${CMAKE_CURRENT_BINARY_DIR} )

qt4_add_resources ( RES_FILES ${PROJECT_SOURCE_DIR}/Images/images.qrc )

include_directories ( ${MUPARSER_INCLUDE_DIR} )
include_directories ( ${ZLIB_INCLUDE_DIRS} )

include_directories ( ${QWT_INCLUDE_DIR} )

find_package ( QwtPlot3d REQUIRED )
include_directories ( ${QWTPLOT3D_INCLUDE_DIR} )

find_package ( QScintilla REQUIRED )
include_directories ( ${QSCINTILLA_INCLUDE_DIR} )

# This section deals with creating the MantidPlotReleaseDate.h header
if ( Subversion_FOUND )
  # extract working copy information for SOURCE_DIR into RELDATE_XXX variables
  Subversion_WC_INFO(${PROJECT_SOURCE_DIR} RELDATE)
else ()
  # Just use a dummy version number and print a warning
  message ( "Subversion not found - using 0 for revision number in MantidPlotReleaseDate.h" )
  set ( RELDATE_WC_REVISION 0 )
endif ()
# Create the header with the revision number in
#set ( HEADER_DIR src/Mantid )
configure_file ( ${CMAKE_CURRENT_SOURCE_DIR}/src/Mantid/MantidPlotReleaseDate.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/MantidPlotReleaseDate.h 
)

add_definitions ( -DSCRIPTING_MUPARSER )
add_definitions ( -DSCRIPTING_PYTHON )
add_definitions ( -DQSCINTILLA_DLL )     # Will only have an effect on Windows (as is desired)

include_directories ( src )

add_executable ( MantidPlot ${SRC_FILES} ${MOCCED_FILES} ${UI_HDRS} ${RES_FILES} )

target_link_libraries ( MantidPlot 
                        ${CORE_MANTIDLIBS} MantidQtAPI MantidWidgets
                        QtPropertyBrowser ${QT_LIBRARIES} 
                        ${QWT_LIBRARIES} ${QWTPLOT3D_LIBRARIES}
                        ${QSCINTILLA_LIBRARIES}
                        ${PYTHON_LIBRARIES}
                        ${ZLIB_LIBRARIES}
)

# This sets things to generate a console application for a debug build on Windows
if(WIN32)
   set_target_properties(MantidPlot PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
   set_target_properties(MantidPlot PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
   set_target_properties(MantidPlot PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
   set_target_properties(MantidPlot PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "_CONSOLE")
   set_target_properties(MantidPlot PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
   set_target_properties(MantidPlot PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")
endif(WIN32)

# Now copy the required python files into the build destination
set ( PYTHON_INSTALL_FILES qtiplotrc.py qtiplotrc.py qtiplotrc.py mantidplotrc.py )
foreach ( PYFILE ${PYTHON_INSTALL_FILES} )
  add_custom_command ( TARGET MantidPlot POST_BUILD 
                       COMMAND ${CMAKE_COMMAND} ARGS -E copy_if_different 
                         ${CMAKE_CURRENT_SOURCE_DIR}/${PYFILE}
                         ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR} 
  )
endforeach ( PYFILE ${PYTHON_INSTALL_FILES} )
