# We need to find the right boost python version for the given python version. Unfortunately
# there is not standard naming convention across the platforms, for example:
#  - fedora, arch: libboost_python.so (python2), libboost_python3.so (python3)
#  - debian, ubuntu: libboost_python.so (python2), libboost_python-py34.so (python 3.4)
#  - windows: boost_python (python2), ????? (python3)
#  - others?
if ( MSVC )
  if ( ${Boost_VERSION} LESS 106700)
    find_package ( Boost REQUIRED python )
  else ()
    find_package ( Boost REQUIRED python27 )
  endif ()
else ()
  if ( PYTHON_VERSION_MAJOR GREATER 2 )
    # Try a known set of suffixes plus a user-defined set
    # Define a cache variable to supply additional suffxies. These are tried first
    set ( BOOST_PYTHON_ADDITIONAL_SUFFIXES "" CACHE STRING "Additional suffixes to try when searching for the boost python3 library. These are prepended to the default list" )
    set ( _suffixes "${BOOST_PYTHON_ADDITIONAL_SUFFIXES};${PYTHON_VERSION_MAJOR};-py${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR};${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR}")
    foreach ( _suffix ${_suffixes})
      find_package ( Boost COMPONENTS python${_suffix} )
      if ( Boost_FOUND )
        break ()
      endif ()
    endforeach ()
    if ( NOT Boost_FOUND )
      message ( FATAL_ERROR "Cannot find appropriate boost python version after trying with the "
                "following library suffixes: ${_suffixes}" )
    endif ()
  else ()
    # Assumes that the default version is 27
    find_package ( Boost COMPONENTS python )
    if ( NOT Boost_FOUND )
      find_package ( Boost COMPONENTS python27 )
    endif ()
    if ( NOT Boost_FOUND )
      message ( FATAL_ERROR "Cannot find appropriate boost python version after trying with the "
                "following library suffixes: ;27" )
    endif ()
  endif ()
endif ()
