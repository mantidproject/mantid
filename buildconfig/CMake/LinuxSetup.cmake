# ######################################################################################################################
# Who are we
# ######################################################################################################################
include(DetermineLinuxDistro)

# ######################################################################################################################
# If required, find tbbmalloc
# ######################################################################################################################
option(USE_TBBMALLOC "If true, use LD_PRELOAD=libtbbmalloc_proxy.so in startup scripts" ON)
# If not wanted, just carry on without it
if(USE_TBBMALLOC)
  find_package(TBB)
  if(NOT TBB_FOUND)
    message(STATUS "TBB not found, skipping TBB malloc proxy setup.")
    set(USE_TBBMALLOC OFF)
  else()
    # The launch scripts use TBBMALLOC_LIBRARIES to set LD_PRELOAD.
    if(TARGET TBB::tbbmalloc_proxy)
      set(TBBMALLOC_LIBRARIES TBB::tbbmalloc_proxy)
      set(TBBMALLOC_FOUND TRUE)
      message(STATUS "Found TBB malloc proxy library: ${TBBMALLOC_LIBRARIES}")
      get_target_property(_tbbmalloc_proxy_location TBB::tbbmalloc_proxy IMPORTED_LOCATION)
      if(NOT _tbbmalloc_proxy_location)
        get_target_property(_tbb_configs TBB::tbbmalloc_proxy IMPORTED_CONFIGURATIONS)
        foreach(_conf ${_tbb_configs})
          get_target_property(_tbbmalloc_proxy_location TBB::tbbmalloc_proxy IMPORTED_LOCATION_${_conf})
          if(_tbbmalloc_proxy_location)
            break()
          endif()
        endforeach()
      endif()

      if(_tbbmalloc_proxy_location)
        set(TBBMALLOC_RUNTIME_LIB "${_tbbmalloc_proxy_location}")
        message(STATUS "Determined location of TBB malloc proxy library: ${TBBMALLOC_RUNTIME_LIB}")
      else()
        message(WARNING "Could not determine location of TBB malloc proxy library")
      endif()
    else()
      message(WARNING "TBB malloc proxy not found even though USE_TBBMALLOC is ON. Continuing without it.")
    endif()
  endif()
  # if it can't be found still carry on as the build will work. The package depenendencies will install it for the end
  # users
else(USE_TBBMALLOC)
  message(STATUS "TBB malloc will not be included in startup scripts")
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
