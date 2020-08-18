# ##############################################################################
# Who are we
# ##############################################################################
include(DetermineLinuxDistro)

# ##############################################################################
# If required, find tcmalloc
# ##############################################################################
option(USE_TCMALLOC "If true, use LD_PRELOAD=libtcmalloc.so in startup scripts"
       ON
)
# If not wanted, just carry on without it
if(USE_TCMALLOC)
  find_package(Tcmalloc)
  if(TCMALLOC_FOUND)
    set(TCMALLOC_LIBRARY ${TCMALLOC_LIBRARIES})
  endif(TCMALLOC_FOUND)
  # if it can't be found still carry on as the build will work. The package
  # depenendencies will install it for the end users
else(USE_TCMALLOC)
  message(STATUS "TCMalloc will not be included in startup scripts")
endif()

# Tag used by dynamic loader to identify directory of loading library
set(DL_ORIGIN_TAG \$ORIGIN)

# ##############################################################################
# Set up package scripts for this distro
# ##############################################################################
include(LinuxPackageScripts)
