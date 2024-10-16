# Core Python/C++ code used by all layers accessing the Python C API. It contains functionality such as classes for GIL
# management, conversion of Python errors to exceptions etc.

# It should not link to any other Mantid libraries.

set(SRC_FILES
    src/ErrorHandling.cpp
    src/GlobalInterpreterLock.cpp
    src/NDArray.cpp
    src/PythonObjectProperty.cpp
    src/ReleaseGlobalInterpreterLock.cpp
    src/UninstallTrace.cpp
    src/PythonLoggingChannel.cpp
    src/PythonStdoutChannel.cpp
    src/WrapperHelpers.cpp
    src/Converters/CloneToNDArray.cpp
    src/Converters/DateAndTime.cpp
    src/Converters/NDArrayToVector.cpp
    src/Converters/NDArrayTypeIndex.cpp
    src/Converters/NumpyFunctions.cpp
    src/Converters/PyObjectToMatrix.cpp
    src/Converters/PyObjectToV3D.cpp
    src/Converters/PyObjectToVMD.cpp
    src/Converters/WrapWithNDArray.cpp
)

set(INC_FILES
    inc/MantidPythonInterface/core/CallMethod.h
    inc/MantidPythonInterface/core/Copyable.h
    inc/MantidPythonInterface/core/DataServiceExporter.h
    inc/MantidPythonInterface/core/DllConfig.h
    inc/MantidPythonInterface/core/ErrorHandling.h
    inc/MantidPythonInterface/core/ExtractSharedPtr.h
    inc/MantidPythonInterface/core/GlobalInterpreterLock.h
    inc/MantidPythonInterface/core/IsNone.h
    inc/MantidPythonInterface/core/NDArray.h
    inc/MantidPythonInterface/core/PropertyWithValueExporter.h
    inc/MantidPythonInterface/core/PythonLoggingChannel.h
    inc/MantidPythonInterface/core/PythonObjectInstantiator.h
    inc/MantidPythonInterface/core/PythonObjectProperty.h
    inc/MantidPythonInterface/core/PythonStdoutChannel.h
    inc/MantidPythonInterface/core/ReleaseGlobalInterpreterLock.h
    inc/MantidPythonInterface/core/StlExportDefinitions.h
    inc/MantidPythonInterface/core/TypedValidatorExporter.h
    inc/MantidPythonInterface/core/UninstallTrace.h
    inc/MantidPythonInterface/core/VersionCompat.h
    inc/MantidPythonInterface/core/WrapperHelpers.h
    inc/MantidPythonInterface/core/WrapPython.h
    inc/MantidPythonInterface/core/Converters/CArrayToNDArray.h
    inc/MantidPythonInterface/core/Converters/CloneToNDArray.h
    inc/MantidPythonInterface/core/Converters/ContainerDtype.h
    inc/MantidPythonInterface/core/Converters/DateAndTime.h
    inc/MantidPythonInterface/core/Converters/MapToPyDictionary.h
    inc/MantidPythonInterface/core/Converters/MatrixToNDArray.h
    inc/MantidPythonInterface/core/Converters/NDArrayToVector.h
    inc/MantidPythonInterface/core/Converters/NDArrayTypeIndex.h
    inc/MantidPythonInterface/core/Converters/NumpyFunctions.h
    inc/MantidPythonInterface/core/Converters/PyObjectToMatrix.h
    inc/MantidPythonInterface/core/Converters/PyObjectToV3D.h
    inc/MantidPythonInterface/core/Converters/PyObjectToVMD.h
    inc/MantidPythonInterface/core/Converters/PySequenceToVector.h
    inc/MantidPythonInterface/core/Converters/ToPyList.h
    inc/MantidPythonInterface/core/Converters/VectorToNDArray.h
    inc/MantidPythonInterface/core/Converters/WrapWithNDArray.h
    inc/MantidPythonInterface/core/Policies/MatrixToNumpy.h
    inc/MantidPythonInterface/core/Policies/RemoveConst.h
    inc/MantidPythonInterface/core/Policies/VectorToNumpy.h
)

# Add a precompiled header where they are supported
enable_precompiled_headers(inc/MantidPythonInterface/core/PrecompiledHeader.h SRC_FILES)

# Add the target for this directory
set(_target_name PythonInterfaceCore)

add_library(${_target_name} ${SRC_FILES} ${INC_FILES})
add_library(Mantid::${_target_name} ALIAS ${_target_name})

target_include_directories(
  ${_target_name} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc> $<INSTALL_INTERFACE:include/Mantid>
)

# Set the name of the generated library
set_target_properties(
  ${_target_name} PROPERTIES OUTPUT_NAME MantidPythonInterfaceCore COMPILE_DEFINITIONS IN_MANTID_PYTHONINTERFACE_CORE
)
# We need an if check here as older versions of boost won't define this target we can remove it once we adopt conda
# fully.
if(TARGET Boost::python${Python_VERSION_MAJOR}${Python_VERSION_MINOR})
  set(PUBLIC_TARGETS Python::NumPy Boost::python${Python_VERSION_MAJOR}${Python_VERSION_MINOR})
else()
  set(PUBLIC_TARGETS Python::NumPy ${BoostPython_LIBRARIES})
endif()
if(USE_PYTHON_DYNAMIC_LIB)
  list(APPEND PUBLIC_TARGETS Python::Python)
endif()
# Dependencies
target_link_libraries(
  ${_target_name}
  PUBLIC ${PUBLIC_TARGETS} Mantid::Kernel
  PRIVATE Mantid::Types
)

# Add to the 'Framework/Python' group in MSVS
set_property(TARGET ${_target_name} PROPERTY FOLDER "MantidFramework/Python")

# rpath settings
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_target_properties(${_target_name} PROPERTIES INSTALL_RPATH "@loader_path/../MacOS")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set_target_properties(${_target_name} PROPERTIES INSTALL_RPATH "\$ORIGIN/../${LIB_DIR}")
endif()

set(TARGET_EXPORT_NAME "MantidPythonInterfaceCoreTargets")
mtd_install_framework_lib(TARGETS PythonInterfaceCore EXPORT_NAME ${TARGET_EXPORT_NAME})
