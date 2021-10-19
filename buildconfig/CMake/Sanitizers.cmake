# ######################################################################################################################
# Controls various project wide sanitizer options
# ######################################################################################################################

set(USE_SANITIZER
    "Off"
    CACHE STRING "Sanitizer mode to enable"
)
set_property(CACHE USE_SANITIZER PROPERTY STRINGS Off Address Thread Undefined)

string(TOLOWER "${USE_SANITIZER}" USE_SANITIZERS_LOWER)

if(NOT ${USE_SANITIZERS_LOWER} MATCHES "off")

  if(CMAKE_COMPILER_IS_GNUCXX AND (NOT CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 8))
    message(FATAL_ERROR "GCC 7 and below do not support sanitizers")
  endif()

  if(MSVC)
    if(MSVC_VERSION LESS 1927)
      message(FATAL_ERROR "MSVC version below 16.7 doesn't support address sanitizer")
    endif()
  endif()

  if(MSVC)
    # some units don't compile using the /O2 flag
    message("Replacing -O2 flag with -O1 so that code compiles with sanitizer")
    string(REPLACE "/O2" "/O1" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
    string(REPLACE "/O2" "/O1" CMAKE_C_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO})
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO
        ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}
        CACHE STRING "" FORCE
    )
    set(CMAKE_C_FLAGS_RELWITHDEBINFO
        ${CMAKE_C_FLAGS_RELWITHDEBINFO}
        CACHE STRING "" FORCE
    )

    if(${USE_SANITIZERS_LOWER} MATCHES "address")
      add_compile_options($<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:/fsanitize=address>)
      add_link_options(
        "$<IF:$<CONFIG:Debug>,/wholearchive:clang_rt.asan_dbg_dynamic-x86_64.lib;/wholearchive:clang_rt.asan_dbg_dynamic_runtime_thunk-x86_64.lib;/wholearchive:vcasand.lib,/wholearchive:clang_rt.asan_dynamic-x86_64.lib;/wholearchive:clang_rt.asan_dynamic_runtime_thunk-x86_64.lib;/wholearchive:vcasan.lib>"
      )
    else()
      message(FATAL_ERROR "MSVC only supports address sanitizer")
    endif()
  else()
    # Check and warn if we are not in a mode without debug symbols
    string(TOLOWER "${CMAKE_BUILD_TYPE}" build_type_lower)
    if("${build_type_lower}" MATCHES "release" OR "${build_type_lower}" MATCHES "minsizerel")
      message(WARNING "You are running address sanitizers without debug information, try RelWithDebInfo")
    endif()
    # RelWithDebug runs with -o2 which performs inlining reducing the usefulness
    message("Replacing -O2 flag with -O1 to preserve stack trace on sanitizers")
    string(REPLACE "-O2" "-O1" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
    string(REPLACE "-O2" "-O1" CMAKE_C_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO})
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO
        ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}
        CACHE STRING "" FORCE
    )
    set(CMAKE_C_FLAGS_RELWITHDEBINFO
        ${CMAKE_C_FLAGS_RELWITHDEBINFO}
        CACHE STRING "" FORCE
    )
    if(${build_type_lower} MATCHES "relwithdebinfo")
      add_compile_options(-fno-omit-frame-pointer -fno-optimize-sibling-calls)
    endif()

    # Allow all instrumented code to continue beyond the first error
    add_compile_options($<$<NOT:$<STREQUAL:$<LOWER_CASE:"${USE_SANITIZER}">,"off">>:-fsanitize-recover=all>)
    # Address
    add_compile_options($<$<STREQUAL:$<LOWER_CASE:"${USE_SANITIZER}">,"address">:-fsanitize=address>)
    add_link_options($<$<STREQUAL:$<LOWER_CASE:"${USE_SANITIZER}">,"address">:-fsanitize=address>)

    # Thread
    add_compile_options($<$<STREQUAL:$<LOWER_CASE:"${USE_SANITIZER}">,"thread">:-fsanitize=thread>)
    add_link_options($<$<STREQUAL:$<LOWER_CASE:"${USE_SANITIZER}">,"thread">:-fsanitize=thread>)

    # Undefined RTTI information is not exported for some classes causing the linker to fail whilst adding vptr
    # instrumentation
    add_compile_options(
      $<$<STREQUAL:$<LOWER_CASE:"${USE_SANITIZER}">,"undefined">:-fsanitize=undefined>
      $<$<STREQUAL:$<LOWER_CASE:"${USE_SANITIZER}">,"undefined">:-fno-sanitize=vptr>
    )

    add_link_options($<$<STREQUAL:$<LOWER_CASE:"${USE_SANITIZER}">,"undefined">:-fsanitize=undefined>)
  endif()
endif()
