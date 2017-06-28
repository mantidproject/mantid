include(ExternalProject)

set(xmlrunner_version "2.1.0")

option(USE_SYSTEM_XMLRUNNER "Use the system installed unittest-xmlrunner?" OFF)

if(USE_SYSTEM_XMLRUNNER)
  # Currrently assumes item is importable
  message(STATUS "Using system unittest-xml-runner")
else()
  message(STATUS "Using unittest-xml-runner from ExternalProject")

  # Download and unpack at configure time
  set ( XMLRUNNER_DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/python-xmlrunner-download )
  configure_file(${CMAKE_SOURCE_DIR}/buildconfig/CMake/Python-xmlrunner.in ${XMLRUNNER_DOWNLOAD_DIR}/CMakeLists.txt)

  execute_process(COMMAND ${CMAKE_COMMAND} . WORKING_DIRECTORY ${XMLRUNNER_DOWNLOAD_DIR} )
  execute_process(COMMAND ${CMAKE_COMMAND} --build . WORKING_DIRECTORY ${XMLRUNNER_DOWNLOAD_DIR} )

  set(PYTHON_XMLRUNNER_DIR "${CMAKE_BINARY_DIR}/python-xmlrunner-src" CACHE PATH "Location of xmlrunner package")

endif()
