#############################################################################################
# __curvefitting Python module
#############################################################################################

set ( MODULE_TEMPLATE src/curvefitting.cpp.in )

# Files containing export definitions, these are automatically processed
# -- Do NOT sort this list. The order defines the order in which the export
#    definitions occur and some depend on their base classes being exported first --
set ( EXPORT_FILES
  src/Exports/ProductFunction.cpp
)

set ( MODULE_DEFINITION ${CMAKE_CURRENT_BINARY_DIR}/curvefitting.cpp )
create_module ( ${MODULE_TEMPLATE} ${MODULE_DEFINITION} ${EXPORT_FILES} )

#############################################################################################
# Helper code
#############################################################################################
set ( SRC_FILES )

set ( INC_FILES )

set ( PY_FILES __init__.py )


#############################################################################################
# Copy over the pure Python files for the module
#############################################################################################
# Set the destination directory
set ( OUTPUT_DIR ${PYTHON_PKG_ROOT}/_plugins )

if(CMAKE_GENERATOR STREQUAL Xcode)
  # Set the output directory for the libraries.
  set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PYTHON_PKG_ROOT}/_plugins )
endif()

# copy_files_to_dir ( "${PY_FILES}" ${CMAKE_CURRENT_SOURCE_DIR} ${OUTPUT_DIR}
#                      PYTHON_INSTALL_FILES )

#############################################################################################
# Create the target for this directory
#############################################################################################
set ( FRAMEWORK_DIR ../../.. )
include_directories ( ${FRAMEWORK_DIR}/CurveFitting/inc )

add_library ( PythonCurveFittingModule ${MODULE_DEFINITION} ${EXPORT_FILES} ${SRC_FILES}
              ${INC_FILES} ${PYTHON_INSTALL_FILES} )
set_python_properties( PythonCurveFittingModule _curvefitting )
# set_target_output_directory ( PythonCurveFittingModule ${OUTPUT_DIR} .pyd )

# Add the required dependencies
target_link_libraries ( PythonCurveFittingModule LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
            PythonInterfaceCore
            PythonAPIModule
            PythonGeometryModule
            PythonKernelModule
            API
            CurveFitting
            Kernel
            HistogramData
            Geometry
            ${PYTHON_LIBRARIES}
            ${POCO_LIBRARIES}
            ${Boost_LIBRARIES} )


if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties( PythonCurveFittingModule PROPERTIES
    INSTALL_RPATH "@loader_path/../../../MacOS;@loader_path/../../../../plugins;@loader_path/../kernel/;@loader_path/../geometry/;@loader_path/../api/")
elseif ( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
  set_target_properties( PythonCurveFittingModule PROPERTIES
    INSTALL_RPATH "\$ORIGIN/../../../${LIB_DIR};\$ORIGIN/../../../${PLUGINS_DIR};\$ORIGIN/../kernel/;\$ORIGIN/../geometry/;\$ORIGIN/../api/")
endif ()

###########################################################################
# Installation settings
###########################################################################
mtd_install_targets( TARGETS PythonCurveFittingModule INSTALL_DIRS ${BIN_DIR}/mantid/_plugins ${WORKBENCH_BIN_DIR}/mantid/_plugins )

# Pure Python files
mtd_install_files( FILES ${PY_FILES} INSTALL_DIRS ${BIN_DIR}/mantid/_plugins ${WORKBENCH_BIN_DIR}/mantid/_plugins)
