// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAPI/FileFinderHelpers.h"

#include "MantidKernel/WarningSuppressions.h"

#include "MantidPythonInterface/core/Converters/PySequenceToVector.h"
#include "MantidPythonInterface/core/Converters/ToPyList.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/core/Policies/RemoveConst.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/list.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/register_ptr_to_python.hpp>

#include <string>

using Mantid::API::FileFinderHelpers;
using namespace Mantid::PythonInterface;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(FileFinderHelpers)

GNU_DIAG_OFF("unused-local-typedef")
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GNU_DIAG_OFF("conversion")
/// Overload generator for getInstrumentFilename
BOOST_PYTHON_FUNCTION_OVERLOADS(getInstrumentFilename_Overload,
                                FileFinderHelpers::getInstrumentFilename, 1, 2)
BOOST_PYTHON_FUNCTION_OVERLOADS(getIPFPath_Overload,
                                FileFinderHelpers::getIPFPath, 1, 2)
GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")

void exportFileFinderHelpers() {
  register_ptr_to_python<std::shared_ptr<FileFinderHelpers>>();

  class_<FileFinderHelpers>("FileFinderHelpers", no_init)
      // -
      .def("getInstrumentFilename", &FileFinderHelpers::getInstrumentFilename,
           getInstrumentFilename_Overload(
               "Returns IDF filename", (arg("instrument"), arg("date") = "")))
      .staticmethod("getInstrumentFilename")
      // -
      .def("getIPFPath", &FileFinderHelpers::getIPFPath,
           getIPFPath_Overload(
               "Returns the full path to the given instrument parameter file "
               "for the named instrument if it exists in the instrument search "
               "directories, or the optional user provided path.\n"

               "instName:      The name of the instrument to lookup the IPF "
               "for\n"
               "directoryHint: (Optional) Searches the user provided path "
               "before any instrument dirs\n"
               "returns:       The full path as a string if found, else an "
               "empty string",
               (arg("instName"), arg("directoryHint") = "")))
      .staticmethod("getIPFPath");
}