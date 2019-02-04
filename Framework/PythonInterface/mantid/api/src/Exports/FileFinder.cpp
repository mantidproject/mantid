// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/FileFinder.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/core/ReleaseGlobalInterpreterLock.h"
#include <boost/python/class.hpp>
#include <boost/python/list.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/reference_existing_object.hpp>

using Mantid::API::FileFinder;
using Mantid::API::FileFinderImpl;
using namespace boost::python;

namespace {
GNU_DIAG_OFF("unused-local-typedef")
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GNU_DIAG_OFF("conversion")

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(getFullPathOverloader, getFullPath, 1, 2)

GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")
} // namespace

/**
 * Runs FileFinder.findRuns after releasing the python GIL.
 * @param self :: A reference to the calling object
 * @param hintstr :: A string containing the run number and possibly instrument
 * to search for
 * @param exts_list :: A python list containing strings of file extensions to
 * search
 * @param useExtsOnly :: bool. If true, use exts_list only. If false, use
 * combination of exts_list and facility_exts.
 */
std::vector<std::string> runFinderProxy(FileFinderImpl &self,
                                        std::string hintstr, list exts_list,
                                        const bool useExtsOnly) {
  // Convert python list to c++ vector
  std::vector<std::string> exts;
  for (int i = 0; i < len(exts_list); ++i)
    exts.push_back(extract<std::string>(exts_list[i]));

  //   Before calling the function we need to release the GIL,
  //   drop the Python threadstate and reset anything installed
  //   via PyEval_SetTrace while we execute the C++ code -
  //   ReleaseGlobalInterpreter does this for us
  Mantid::PythonInterface::ReleaseGlobalInterpreterLock
      releaseGlobalInterpreterLock;
  return self.findRuns(hintstr, exts, useExtsOnly);
}

void export_FileFinder() {
  class_<FileFinderImpl, boost::noncopyable>("FileFinderImpl", no_init)
      .def("getFullPath", &FileFinderImpl::getFullPath,
           getFullPathOverloader(
               (arg("self"), arg("path"), arg("ignoreDirs") = false),
               "Return a full path to the given file if it can be found within "
               "datasearch.directories paths. Directories can be ignored with "
               "ignoreDirs=True. An empty string is returned otherwise."))
      .def("findRuns", &runFinderProxy,
           (arg("self"), arg("hintstr"), arg("exts_list") = list(),
            arg("useExtsOnly") = false),
           "Find a list of files file given a hint. "
           "The hint can be a comma separated list of run numbers and can also "
           "include ranges of runs, e.g. 123-135 or equivalently 123-35"
           "If no instrument prefix is given then the current default is used."
           "exts_list is an optional list containing strings of file "
           "extensions to search."
           "useExtsOnly is an optional bool. If it's true then don't use "
           "facility exts.")
      .def("getCaseSensitive", &FileFinderImpl::getCaseSensitive, (arg("self")),
           "Option to get if file finder should be case sensitive.")
      .def("setCaseSensitive", &FileFinderImpl::setCaseSensitive,
           (arg("self"), arg("cs")),
           "Option to set if file finder should be case sensitive.")
      .def("Instance", &FileFinder::Instance,
           return_value_policy<reference_existing_object>(),
           "Returns a reference to the FileFinder singleton instance")
      .staticmethod("Instance");
}
