# ######################################################################################################################
# Who are we
# ######################################################################################################################
include(DetermineLinuxDistro)

# ######################################################################################################################
# If required, find jemalloc
# ######################################################################################################################
option(USE_JEMALLOC "If true, use LD_PRELOAD=libjemalloc.so in startup scripts" ON)
# If not wanted, just carry on without it
if(USE_JEMALLOC)
  find_package(JemallocLib REQUIRED)
  if(JEMALLOCLIB_FOUND)
    set(JEMALLOC_LIBRARY ${JEMALLOC_LIBRARIES})
  endif(JEMALLOCLIB_FOUND)
  # if it can't be found still carry on as the build will work. The package depenendencies will install it for the end
  # users
else(USE_JEMALLOC)
  message(STATUS "Jemalloc will not be included in startup scripts")
endif()

# ######################################################################################################################
# If required, find the address sanitizer library: libasan
# ######################################################################################################################
string(TOLOWER "${USE_SANITIZER}" USE_SANITIZERS_LOWER)
if(${USE_SANITIZERS_LOWER} MATCHES "address")
  find_package(AsanLib REQUIRED)
  if(ASANLIB_FOUND)
    set(ASAN_LIBRARY ${ASAN_LIBRARIES})
  endif(ASANLIB_FOUND)
endif()

# Tag used by dynamic loader to identify directory of loading library
set(DL_ORIGIN_TAG \$ORIGIN)

# ######################################################################################################################
# Set up package scripts for this distro
# ######################################################################################################################
include(LinuxPackageScripts)
