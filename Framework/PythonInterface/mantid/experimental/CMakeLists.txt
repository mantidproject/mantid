set ( MODULE_TEMPLATE src/experimental.cpp.in )

set ( EXPORT_FILES
  src/Exports/PythonSmartPointerHelpers.cpp
  )

set ( MODULE_DEFINITION ${CMAKE_CURRENT_BINARY_DIR}/experimental.cpp )
create_module ( ${MODULE_TEMPLATE} ${MODULE_DEFINITION} ${EXPORT_FILES} )

set ( SRC_FILES )

set ( INC_FILES )

set ( PY_FILES
  __init__.py
  _aliases.py
  pythonapi.py
  )

#############################################################################################
# Add rule to copy over the pure Python files for the module
#############################################################################################
set ( OUTPUT_DIR ${PYTHON_PKG_ROOT}/experimental )

if(CMAKE_GENERATOR STREQUAL Xcode)
  # Set the output directory for the libraries.
  set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PYTHON_PKG_ROOT}/experimental )
endif()

copy_files_to_dir ( "${PY_FILES}" ${CMAKE_CURRENT_SOURCE_DIR} ${OUTPUT_DIR}
                     PYTHON_INSTALL_FILES )

#############################################################################################
# Create the target for this directory
#############################################################################################

add_library ( PythonExperimentalModule ${MODULE_DEFINITION} ${EXPORT_FILES} ${SRC_FILES}
              ${INC_FILES} ${PYTHON_INSTALL_FILES} )
set_python_properties( PythonExperimentalModule _experimental )
set_target_output_directory ( PythonExperimentalModule ${OUTPUT_DIR} .pyd )

# Add the required dependencies
target_link_libraries ( PythonExperimentalModule LINK_PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
            PythonAPIModule
            PythonGeometryModule
            PythonKernelModule
            PythonDataObjectsModule
            API
            DataObjects
            Kernel
            Geometry
            ${PYTHON_LIBRARIES}
            ${POCO_LIBRARIES}
            ${Boost_LIBRARIES} )


if (OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties( PythonExperimentalModule PROPERTIES
                         INSTALL_RPATH "@loader_path/../../../MacOS;@loader_path/../kernel/;@loader_path/../geometry/;@loader_path/../api/")
endif ()
###########################################################################
# Installation settings
###########################################################################
install ( TARGETS PythonExperimentalModule ${SYSTEM_PACKAGE_TARGET} DESTINATION ${BIN_DIR}/mantid/experimental )

# Pure Python files
install ( FILES ${PY_FILES} DESTINATION ${BIN_DIR}/mantid/experimental )
