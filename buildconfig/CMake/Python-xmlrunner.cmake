include(ExternalProject)

set(xmlrunner_version "2.1.0")

option(USE_SYSTEM_XMLRUNNER "Use the system installed unittest-xmlrunner?" OFF)

if ( WIN32 )
  # Installed by 3rd party dependencies bundle
  set ( USE_SYSTEM_XMLRUNNER ON )
endif ()

if(USE_SYSTEM_XMLRUNNER)
  # Currently assumes item is importable
  message(STATUS "Using system unittest-xml-runner")
else()
  message(STATUS "Using unittest-xml-runner from ExternalProject")

  # Download and unpack at configure time
  set ( XMLRUNNER_DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/python-xmlrunner-download )
  configure_file(${CMAKE_SOURCE_DIR}/buildconfig/CMake/Python-xmlrunner.in ${XMLRUNNER_DOWNLOAD_DIR}/CMakeLists.txt)

  execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" -DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION} . WORKING_DIRECTORY ${XMLRUNNER_DOWNLOAD_DIR} )
  execute_process(COMMAND ${CMAKE_COMMAND} --build . WORKING_DIRECTORY ${XMLRUNNER_DOWNLOAD_DIR} )

  set(PYTHON_XMLRUNNER_DIR "${CMAKE_BINARY_DIR}/python-xmlrunner-src" CACHE PATH "Location of xmlrunner package")

endif()
