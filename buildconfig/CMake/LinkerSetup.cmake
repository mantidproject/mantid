# Setup for a faster linker. mold (Linux) links large shared libraries (e.g. Algorithms, DataHandling) significantly
# faster than the default bfd/gold linker.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
  option(USE_MOLD_LINKER "Use the mold linker if available" ON)
  if(USE_MOLD_LINKER)
    find_program(MOLD_FOUND ld.mold)
    if(MOLD_FOUND)
      message(STATUS "Using mold linker")
      string(APPEND CMAKE_EXE_LINKER_FLAGS " -fuse-ld=mold")
      string(APPEND CMAKE_SHARED_LINKER_FLAGS " -fuse-ld=mold")
      string(APPEND CMAKE_MODULE_LINKER_FLAGS " -fuse-ld=mold")
    endif()
  endif()
endif()

# Setup for a faster linker on Windows. lld-link is a drop-in replacement for MSVC's link.exe and is considerably faster
# for the large DLLs. Restricted to Ninja generators; the Visual Studio generator keeps the default MSBuild link step.
if(MSVC AND CMAKE_GENERATOR MATCHES "Ninja")
  option(USE_LLD_LINKER "Use the lld-link linker if available" ON)
  if(USE_LLD_LINKER)
    if(CMAKE_LINKER_LLD)
      message(STATUS "Using lld-link linker: ${CMAKE_LINKER_LLD}")
      set(CMAKE_LINKER_TYPE LLD)
    else()
      message(STATUS "lld-link not found, using the default MSVC linker")
    endif()
  endif()
endif()
