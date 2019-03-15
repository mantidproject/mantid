find_package ( Cppcheck )

if ( CPPCHECK_EXECUTABLE )
  set ( CPPCHECK_SOURCE_DIRS
        Framework
        MantidPlot
        qt
      )

  set ( CPPCHECK_USE_INCLUDE_DIRS OFF CACHE BOOL "Use specified include directories. WARNING: cppcheck will run significantly slower." )

  set ( CPPCHECK_INCLUDE_DIRS
        Framework/Algorithms/inc
        Framework/PythonInterface/inc
        Framework/Nexus/inc
        Framework/MPIAlgorithms/inc
        Framework/MDAlgorithms/inc
        Framework/DataHandling/inc
        Framework/WorkflowAlgorithms/inc
        Framework/MDEvents/inc
        Framework/DataObjects/inc
        Framework/Geometry/inc
        Framework/ICat/inc
        Framework/CurveFitting/inc
        Framework/API/inc
        Framework/TestHelpers/inc
        Framework/Crystal/inc
        Framework/Kernel/inc
        qt/paraview_ext/VatesAPI/inc
        qt/paraview_ext/VatesSimpleGui/ViewWidgets/inc
        qt/paraview_ext/VatesSimpleGui/QtWidgets/inc
        qt/widgets/common/inc
        qt/widgets/factory/inc
        qt/widgets/instrumentview/inc
        qt/widgets/refdetectrview/inc
        qt/widgets/sliceviewer/inc
        qt/widgets/spectrumviewer/inc
        qt/widgets/plugins/algorithm_dialogs/inc
        qt/widgets/plugins/designer/inc
        qt/scientific_interfaces
      )

  set ( CPPCHECK_EXCLUDES
        Framework/LiveData/src/ISIS/DAE/
        Framework/LiveData/src/ISIS/private/flatbuffers/
        Framework/LiveData/src/ISIS/private/schema/
        Framework/DataHandling/src/LoadRaw/
        Framework/ICat/inc/MantidICat/ICat3/GSoapGenerated/
        Framework/ICat/src/ICat3/GSoapGenerated/
        Framework/ICat/src/ICat3/ICat3GSoapGenerated.cpp
        Framework/ICat/inc/MantidICat/ICat4/GSoapGenerated/
        Framework/ICat/src/ICat4/GSoapGenerated/
        Framework/ICat/src/ICat4/ICat4GSoapGenerated.cpp
        Framework/ICat/src/GSoap/
        Framework/ICat/src/GSoap.cpp
        Framework/Kernel/src/ANN/
        Framework/Kernel/src/ANN_complete.cpp
        Framework/Kernel/src/Math/Optimization/SLSQPMinimizer.cpp
        MantidPlot/src/nrutil.cpp
        MantidPlot/src/origin/OPJFile.cpp
        MantidPlot/src/zlib123/minigzip.c
        Framework/SINQ/src/PoldiPeakFit.cpp
        qt/widgets/common/src/QtPropertyBrowser/
        qt/widgets/common/inc/MantidQtWidgets/Common/QtPropertyBrowser/
      )

  # Header files to be ignored require different handling
  set ( CPPCHECK_HEADER_EXCLUDES
        MantidPlot/src/origin/OPJFile.h
        MantidPlot/src/origin/tree.hh
      )

  # setup the standard arguments
  set (_cppcheck_args "${CPPCHECK_ARGS}")
    list ( APPEND _cppcheck_args ${CPPCHECK_TEMPLATE_ARG} "--inline-suppr")
    if ( CPPCHECK_NUM_THREADS GREATER 0)
        list ( APPEND _cppcheck_args -j ${CPPCHECK_NUM_THREADS} )
  endif ( CPPCHECK_NUM_THREADS GREATER 0)

  # process list of include/exclude directories
  set (_cppcheck_source_dirs)
  foreach (_dir ${CPPCHECK_SOURCE_DIRS} )
    set ( _tmpdir "${CMAKE_SOURCE_DIR}/${_dir}" )
    if ( EXISTS ${_tmpdir} )
      list ( APPEND _cppcheck_source_dirs ${_tmpdir} )
    endif ()
  endforeach()

  set (_cppcheck_includes)
  foreach( _dir ${CPPCHECK_INCLUDE_DIRS} )
    set ( _tmpdir "${CMAKE_SOURCE_DIR}/${_dir}" )
    if ( EXISTS ${_tmpdir} )
      list ( APPEND _cppcheck_includes -I ${_tmpdir} )
    endif ()
  endforeach()
  if (CPPCHECK_USE_INCLUDE_DIRS)
    list ( APPEND _cppcheck_args ${_cppcheck_includes} )
  endif (CPPCHECK_USE_INCLUDE_DIRS)

  set (_cppcheck_excludes)
  foreach( _file ${CPPCHECK_EXCLUDES} )
    set ( _tmp "${CMAKE_SOURCE_DIR}/${_file}" )
    if ( EXISTS ${_tmp} )
      list ( APPEND _cppcheck_excludes -i ${_tmp} )
    endif ()
  endforeach()
  list ( APPEND _cppcheck_args ${_cppcheck_excludes} )

  # Handle header files in the required manner
  set (_cppcheck_header_excludes)
  foreach( _file ${CPPCHECK_HEADER_EXCLUDES} )
    set ( _tmp "${CMAKE_SOURCE_DIR}/${_file}" )
    if ( EXISTS ${_tmp} )
      list ( APPEND _cppcheck_header_excludes --suppress=*:${_tmp} )
    endif()
  endforeach()
  list ( APPEND _cppcheck_args ${_cppcheck_header_excludes} )

  # put the finishing bits on the final command call
  set (_cppcheck_xml_args)
  if (CPPCHECK_GENERATE_XML)
    list( APPEND _cppcheck_xml_args  --xml --xml-version=2 ${_cppcheck_source_dirs} 2> ${CMAKE_BINARY_DIR}/cppcheck.xml )
  else (CPPCHECK_GENERATE_XML)
    list( APPEND _cppcheck_xml_args  ${_cppcheck_source_dirs} )
  endif (CPPCHECK_GENERATE_XML)

  # generate the target
  if (NOT TARGET cppcheck)
    add_custom_target ( cppcheck
                        COMMAND ${CPPCHECK_EXECUTABLE} ${_cppcheck_args} ${_cppcheck_header_excludes} ${_cppcheck_xml_args}
                        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                        COMMENT "Running cppcheck"
                      )
    set_target_properties(cppcheck PROPERTIES EXCLUDE_FROM_ALL TRUE)
  endif()
endif ( CPPCHECK_EXECUTABLE )
