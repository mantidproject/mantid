# We need to find the right boost python version for the given python version. Unfortunately
# there is not standard naming convention across the platforms, for example:
#  - fedora, arch: libboost_python.so (python2), libboost_python3.so (python3)
#  - debian, ubuntu: libboost_python.so (python2), libboost_python-py34.so (python 3.4)
#  - windows: boost_python (python2), ????? (python3)
#  - others?
if ( MSVC )
  find_package ( Boost ${BOOST_VERSION_REQUIRED} COMPONENTS python${Python_VERSION_MAJOR}${Python_VERSION_MINOR} REQUIRED )
else ()
  # Try a known set of suffixes plus a user-defined set
  # Define a cache variable to supply additional suffxies. These are tried first
  set ( BOOST_PYTHON_ADDITIONAL_SUFFIXES "" CACHE STRING "Additional suffixes to try when searching for the boost python3 library. These are prepended to the default list" )
  set ( _suffixes "${BOOST_PYTHON_ADDITIONAL_SUFFIXES};${Python_VERSION_MAJOR}${Python_VERSION_MINOR};${Python_VERSION_MAJOR};-py${Python_VERSION_MAJOR}${Python_VERSION_MINOR}")
  foreach ( _suffix ${_suffixes})
    find_package ( Boost ${BOOST_VERSION_REQUIRED} COMPONENTS python${_suffix} )
    if ( Boost_FOUND )
      break ()
    endif ()
  endforeach ()
  if ( NOT Boost_FOUND )
    message ( FATAL_ERROR "Cannot find appropriate boost python version after trying with the "
              "following library suffixes: ${_suffixes}" )
  endif ()
endif ()
