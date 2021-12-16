if(CONDA_ENV)
  find_package(Eigen3 3.4 REQUIRED)
else()
  # Manually grab Eigen
  cmake_minimum_required(VERSION 3.14) # For FetchContent_MakeAvailable
  include(FetchContent)
  fetchcontent_declare(
    Eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG 3.4.0
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
  )

  option(EIGEN_BUILD_DOC OFF)
  option(BUILD_TESTING OFF)
  option(EIGEN_LEAVE_TEST_IN_ALL_TARGET OFF)
  option(EIGEN_BUILD_PKGCONFIG OFF)
  option(EIGEN_TEST_NOQT ON) # Only used in demos and tests

  # Preserve shared attrs
  set(CMAKE_INSTALL_PREFIX_OLD ${CMAKE_INSTALL_PREFIX})
  set(CMAKE_BUILD_TYPE_OLD ${CMAKE_BUILD_TYPE})

  # Install to build/_deps/eigen-install
  set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/_deps/eigen-install")
  set(CMAKE_BUILD_TYPE Release)

  fetchcontent_makeavailable(Eigen)

  # Mark target as a system include
  get_target_property(EIGEN_INC_DIRS eigen INTERFACE_INCLUDE_DIRECTORIES)
  set_target_properties(eigen PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${EIGEN_INC_DIRS}")

  # Restore shared attrs to not affect main mantid project
  set(CMAKE_INSTALL_PREFIX
      ${CMAKE_INSTALL_PREFIX_OLD}
      CACHE STRING "Install path" FORCE
  )
  set(CMAKE_BUILD_TYPE
      ${CMAKE_BUILD_TYPE_OLD}
      CACHE STRING "Choose the type of build" FORCE
  )
endif()
