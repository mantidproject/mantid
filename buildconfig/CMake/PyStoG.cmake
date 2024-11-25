include(ExternalProject)

set(_PyStoG_VERSION f184ebf9a72aae48ae0d5267fd6ab2e7df0988f6) # v0.4.7
set(_PyStoG_download_dir ${CMAKE_CURRENT_BINARY_DIR}/../PyStoG-download)
set(_PyStoG_source_dir ${_PyStoG_download_dir}/src/PyStoG/pystog)
set(_PyStoG_source_test_dir ${_PyStoG_download_dir}/src/PyStoG/tests)
set(_PyStoG_scripts_dir ${CMAKE_CURRENT_BINARY_DIR}/pystog)
set(_PyStoG_test_root_dir ${CMAKE_CURRENT_BINARY_DIR}/test/pystog)
set(_PyStoG_test_dir ${_PyStoG_test_root_dir}/tests)

externalproject_add(
  PyStoG
  PREFIX ${_PyStoG_download_dir}
  GIT_REPOSITORY "https://github.com/neutrons/pystog.git"
  GIT_TAG ${_PyStoG_VERSION}
  EXCLUDE_FROM_ALL 1
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  TEST_COMMAND ""
  INSTALL_COMMAND ""
)

# clone the git repository
message(STATUS "Fetching/updating PyStoG")
execute_process(
  COMMAND ${CMAKE_COMMAND} ARGS -P ${_PyStoG_download_dir}/tmp/PyStoG-gitclone.cmake RESULT_VARIABLE _exit_code
)
if(_exit_code EQUAL 0)
  execute_process(
    COMMAND ${CMAKE_COMMAND} ARGS -P ${_PyStoG_download_dir}/tmp/PyStoG-gitupdate.cmake RESULT_VARIABLE _exit_code
  )
  if(NOT _exit_code EQUAL 0)
    message(FATAL_ERROR "Unable to update PyStoG")
  endif()
else()
  message(FATAL_ERROR "Unable to clone PyStoG")
endif()

set(_source_files converter.py fourier_filter.py transformer.py utils.py)

set(_test_files test_converter.py test_fourier_filter.py test_transformer.py)

set(_test_files_support __init__.py materials.py utils.py)

# copy over the source files we want
file(MAKE_DIRECTORY ${_PyStoG_scripts_dir})
foreach(_py_file ${_source_files})
  file(COPY ${_PyStoG_source_dir}/${_py_file} DESTINATION ${_PyStoG_scripts_dir})
endforeach()

# copy over the relavant tests
file(MAKE_DIRECTORY ${_PyStoG_test_dir})
foreach(_py_file ${_test_files})
  file(COPY ${_PyStoG_source_test_dir}/${_py_file} DESTINATION ${_PyStoG_test_dir})
endforeach()
foreach(_py_file ${_test_files_support})
  file(COPY ${_PyStoG_source_test_dir}/${_py_file} DESTINATION ${_PyStoG_test_dir})
endforeach()

# copy over the test data
file(COPY ${_PyStoG_source_test_dir}/test_data DESTINATION ${_PyStoG_test_dir})

# register the tests
set(PYUNITTEST_PYTHONPATH_EXTRA ${_PyStoG_test_root_dir})
pyunittest_add_test(${_PyStoG_test_dir} python.scripts.pystog ${_test_files})

# create the "simple" __init__.py
set(_PyStoG_INIT_CONTENTS
    "from pystog.converter import Converter
from pystog.transformer import Transformer
from pystog.fourier_filter import FourierFilter

__all__ = ['Converter', 'Transformer', 'FourierFilter', ]

__version__ = '${_PyStoG_VERSION}'
"
)
file(WRITE ${_PyStoG_scripts_dir}/__init__.py ${_PyStoG_INIT_CONTENTS})

# install the results
foreach(_bundle ${BUNDLES})
  install(
    DIRECTORY ${_PyStoG_scripts_dir}
    DESTINATION ${_bundle}scripts
    COMPONENT Runtime
  )
endforeach()
