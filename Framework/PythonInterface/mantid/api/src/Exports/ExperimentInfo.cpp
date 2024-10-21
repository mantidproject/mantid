// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/InstrumentFileFinder.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
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

using Mantid::API::ExperimentInfo;
using Mantid::API::InstrumentFileFinder;
using Mantid::PythonInterface::Policies::RemoveConstSharedPtr;
using namespace Mantid::PythonInterface;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(ExperimentInfo)

/// Converter from C++ signature to python signature
// As we can't have two converters in the global namespace we will
// keep the converter with ExperimentInfo despite the C++ impl moving to
// InstrumentFileFinder for backwards compatibility and as there is a single
// user
list getResourceFilenames(const std::string &prefix, const list &fileFormats, const list &directoryNames,
                          const std::string &date) {
  return Converters::ToPyList<std::string>()(
      InstrumentFileFinder::getResourceFilenames(prefix, Converters::PySequenceToVector<std::string>(fileFormats)(),
                                                 Converters::PySequenceToVector<std::string>(directoryNames)(), date));
}

std::string getInstrumentFilenameWarn(const std::string &instName, const std::string &date = "") {
  PyErr_Warn(PyExc_DeprecationWarning, "ExperimentInfo.getInstrumentFilename() is deprecated.\n"
                                       "Use InstrumentFileFinder.getInstrumentFilename() instead.");
  return InstrumentFileFinder::getInstrumentFilename(instName, date);
}

GNU_DIAG_OFF("unused-local-typedef")
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GNU_DIAG_OFF("conversion")
/// Overload generator for getInstrumentFilename
BOOST_PYTHON_FUNCTION_OVERLOADS(getInstrumentFilename_Overload, getInstrumentFilenameWarn, 1, 2)
GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")

namespace {
void setSample(ExperimentInfo &expInfo, const Mantid::API::Sample &sample) { expInfo.mutableSample() = sample; }
void setRun(ExperimentInfo &expInfo, const Mantid::API::Run &run) { expInfo.mutableRun() = run; }
} // namespace

void export_ExperimentInfo() {
  register_ptr_to_python<std::shared_ptr<ExperimentInfo>>();

  class_<ExperimentInfo, boost::noncopyable>("ExperimentInfo", no_init)
      .def("getInstrument", &ExperimentInfo::getInstrument, return_value_policy<RemoveConstSharedPtr>(), args("self"),
           "Returns the :class:`~mantid.geometry.Instrument` for this run.")
      // InstrumentFileFinder binds Kept for backwards compat.
      .def("getResourceFilenames", &getResourceFilenames,
           (arg("prefix"), arg("fileFormats"), arg("directoryNames"), arg("date")),
           "Compile a list of files in compliance with name pattern-matching,\n"
           "file format, and date-stamp constraints\n\n"
           "Ideally, the valid-from and valid-to of any valid file should\n"
           "encapsulate the argument date. If this is not possible, then\n"
           "the file with the most recent valid-from stamp is selected\n\n"
           "prefix:         the name of a valid file must begin with this "
           "pattern\n"
           "fileFormats:    list of valid file extensions\n"
           "directoryNames: list of directories to be searched\n"
           "date :          the 'valid-from' and 'valid-to 'dates of a valid\n"
           "file will encapsulate this date (e.g '1900-01-31 23:59:00')\n"
           "\nreturns : list of absolute paths for each valid file\n")
      .staticmethod("getResourceFilenames")
      .def("getInstrumentFilename", &InstrumentFileFinder::getInstrumentFilename,
           getInstrumentFilename_Overload("Returns IDF filename", (arg("instrument"), arg("date") = "")))
      .staticmethod("getInstrumentFilename")
      .def("sample", &ExperimentInfo::sample, return_value_policy<reference_existing_object>(), args("self"),
           "Return the :class:`~mantid.api.Sample` object. This cannot be "
           "modified, use mutableSample to modify.")
      .def("mutableSample", &ExperimentInfo::mutableSample, return_value_policy<reference_existing_object>(),
           args("self"), "Return a modifiable :class:`~mantid.api.Sample` object.")
      .def("run", &ExperimentInfo::run, return_value_policy<reference_existing_object>(), args("self"),
           "Return the :class:`~mantid.api.Run` object. This cannot be "
           "modified, use mutableRun to modify.")
      .def("mutableRun", &ExperimentInfo::mutableRun, return_value_policy<reference_existing_object>(), args("self"),
           "Return a modifiable :class:`~mantid.api.Run` object.")
      .def("getRunNumber", &ExperimentInfo::getRunNumber, args("self"), "Returns the run identifier for this run.")
      .def("getEFixed", (double(ExperimentInfo::*)(const Mantid::detid_t) const) & ExperimentInfo::getEFixed,
           args("self", "detId"))
      .def("setEFixed", &ExperimentInfo::setEFixed, args("self", "detId", "value"))
      .def("getEMode", &ExperimentInfo::getEMode, args("self"), "Returns the energy mode.")
      .def("detectorInfo", &ExperimentInfo::detectorInfo, return_value_policy<reference_existing_object>(),
           args("self"),
           "Return a const reference to the "
           ":class:`~mantid.geometry.DetectorInfo` "
           "object.")
      .def("spectrumInfo", &ExperimentInfo::spectrumInfo, return_value_policy<reference_existing_object>(),
           args("self"),
           "Return a const reference to the :class:`~mantid.api.SpectrumInfo` "
           "object.")
      .def("componentInfo", &ExperimentInfo::componentInfo, return_value_policy<reference_existing_object>(),
           args("self"),
           "Return a const reference to the "
           ":class:`~mantid.geometry.ComponentInfo` "
           "object.")
      .def("setSample", setSample, args("self", "sample"))
      .def("setRun", setRun, args("self", "run"));
}
