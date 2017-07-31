#include "MantidAPI/FileFinder.h"
#include "MantidKernel/WarningSuppressions.h"
#include <boost/python/class.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/reference_existing_object.hpp>

using Mantid::API::FileFinder;
using Mantid::API::FileFinderImpl;
using namespace boost::python;

namespace {
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#endif
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GCC_DIAG_OFF(conversion)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(getFullPathOverloader, getFullPath, 1, 2)
GCC_DIAG_ON(conversion)
#ifdef __clang__
#pragma clang diagnostic pop
#endif
}

void export_FileFinder() {
  class_<FileFinderImpl, boost::noncopyable>("FileFinderImpl", no_init)
      .def("getFullPath", &FileFinderImpl::getFullPath,
           getFullPathOverloader(
               (arg("self"), arg("path"), arg("ignoreDirs") = false),
               "Return a full path to the given file if it can be found within "
               "datasearch.directories paths. Directories can be ignored with "
               "ignoreDirs=True. An empty string is returned otherwise."))
      .def("findRuns", &FileFinderImpl::findRuns, (arg("self"), arg("hintstr")),
           "Find a list of files file given a hint. "
           "The hint can be a comma separated list of run numbers and can also "
           "include ranges of runs, e.g. 123-135 or equivalently 123-35"
           "If no instrument prefix is given then the current default is used.")
      .def("Instance", &FileFinder::Instance,
           return_value_policy<reference_existing_object>(),
           "Returns a reference to the FileFinder singleton instance")
      .staticmethod("Instance");
}
