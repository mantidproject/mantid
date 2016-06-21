###########################################################################
# Who are we
###########################################################################
include ( DetermineLinuxDistro )

###########################################################################
# Use the system-installed version of Python.
###########################################################################
find_package ( PythonLibs REQUIRED )
# If found, need to add debug library into libraries variable
if ( PYTHON_DEBUG_LIBRARIES )
  set ( PYTHON_LIBRARIES optimized ${PYTHON_LIBRARIES} debug ${PYTHON_DEBUG_LIBRARIES} )
endif ()

###########################################################################
# If required, find tcmalloc
###########################################################################
set ( USE_TCMALLOC ON CACHE BOOL "If true, use LD_PRELOAD=libtcmalloc.so in startup scripts" )
# If not wanted, just carry on without it
if ( USE_TCMALLOC )
  find_package ( Tcmalloc )
  if ( TCMALLOC_FOUND )
    set ( TCMALLOC_LIBRARY ${TCMALLOC_LIBRARIES} )
  endif ( TCMALLOC_FOUND )
  # if it can't be found still carry on as the build will work. The package
  # depenendencies will install it for the end users
else ( USE_TCMALLOC )
  message ( STATUS "TCMalloc will not be included in startup scripts" )
endif ()

###########################################################################
# Set up package scripts for this distro
###########################################################################
include ( LinuxPackageScripts )
