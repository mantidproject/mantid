# Core Python/C++ code used by all layers accessing the Python C API. It
# contains functionality such as classes for GIL management, conversion of
# Python errors to exceptions etc.

# It should not link to any other Mantid libraries.

set(SRC_FILES
    src/Converters/NDArrayTypeIndex.cpp
    src/Converters/WrapWithNDArray.cpp
    src/ErrorHandling.cpp
    src/GlobalInterpreterLock.cpp
    src/NDArray.cpp
    src/ReleaseGlobalInterpreterLock.cpp
    src/WrapperHelpers.cpp)

set(INC_FILES
    inc/MantidPythonInterface/core/Converters/NDArrayTypeIndex.h
    inc/MantidPythonInterface/core/Converters/VectorToNDArray.h
    inc/MantidPythonInterface/core/Converters/WrapWithNDArray.h
    inc/MantidPythonInterface/core/CallMethod.h
    inc/MantidPythonInterface/core/DllConfig.h
    inc/MantidPythonInterface/core/ErrorHandling.h
    inc/MantidPythonInterface/core/GlobalInterpreterLock.h
    inc/MantidPythonInterface/core/NDArray.h
    inc/MantidPythonInterface/core/ReleaseGlobalInterpreterLock.h
    inc/MantidPythonInterface/core/VersionCompat.h
    inc/MantidPythonInterface/core/WrapperHelpers.h)

# Add the target for this directory
set(_target_name PythonInterfaceCore)
add_library(${_target_name} ${SRC_FILES} ${INC_FILES})
target_include_directories(${_target_name} PUBLIC inc)
# Set the name of the generated library
set_target_properties(${_target_name}
                      PROPERTIES OUTPUT_NAME
                                 MantidPythonInterfaceCore
                                 COMPILE_DEFINITIONS
                                 IN_MANTID_PYTHONINTERFACE_CORE)

# Dependencies
target_link_libraries(${_target_name}
                      PRIVATE ${TCMALLOC_LIBRARIES_LINKTIME}
                              Types
                              ${Boost_LIBRARIES}
                              ${PYTHON_LIBRARIES})

# Add to the 'Framework/Python' group in MSVS
set_property(TARGET ${_target_name} PROPERTY FOLDER "MantidFramework/Python")

# rpath settings
if(OSX_VERSION VERSION_GREATER 10.8)
  set_target_properties(${_target_name}
                        PROPERTIES INSTALL_RPATH "@loader_path/../MacOS")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(${_target_name}
                        PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

mtd_install_targets(TARGETS
                    PythonInterfaceCore
                    INSTALL_DIRS
                    ${LIB_DIR}
                    ${WORKBENCH_LIB_DIR})
