#include "MantidAPI/FileFinder.h"
#include <boost/python/class.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/reference_existing_object.hpp>

using Mantid::API::FileFinder;
using Mantid::API::FileFinderImpl;
using namespace boost::python;

namespace {
  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(getFullPathOverloader, getFullPath, 1, 2);
}

void export_FileFinder()
{
  class_<FileFinderImpl, boost::noncopyable>("FileFinderImpl", no_init)
    .def("getFullPath", &FileFinderImpl::getFullPath,
           getFullPathOverloader((arg("path"), arg("ignoreDirs")=false),
                                 "Return a full path to the given file if it can be found within datasearch.directories paths. Directories can be ignored with ignoreDirs=True. An empty string is returned otherwise."))
    .def("findRuns", &FileFinderImpl::findRuns, "Find a list of files file given a hint. "
         "The hint can be a comma separated list of run numbers and can also include ranges of runs, e.g. 123-135 or equivalently 123-35"
         "If no instrument prefix is given then the current default is used.")
    .def("Instance", &FileFinder::Instance, return_value_policy<reference_existing_object>(),
       "Returns a reference to the FileFinder singleton instance")
    .staticmethod("Instance")
         ;
}

