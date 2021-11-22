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
fetchcontent_makeavailable(Eigen)
