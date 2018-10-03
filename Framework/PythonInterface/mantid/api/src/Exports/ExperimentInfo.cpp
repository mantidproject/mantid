// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/kernel/Policies/RemoveConst.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::API::ExperimentInfo;
using Mantid::PythonInterface::Policies::RemoveConstSharedPtr;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(ExperimentInfo)

GNU_DIAG_OFF("unused-local-typedef")
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GNU_DIAG_OFF("conversion")
/// Overload generator for getInstrumentFilename
BOOST_PYTHON_FUNCTION_OVERLOADS(getInstrumentFilename_Overload,
                                ExperimentInfo::getInstrumentFilename, 1, 2)
GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")

void export_ExperimentInfo() {
  register_ptr_to_python<boost::shared_ptr<ExperimentInfo>>();

  class_<ExperimentInfo, boost::noncopyable>("ExperimentInfo", no_init)
      .def("getInstrument", &ExperimentInfo::getInstrument,
           return_value_policy<RemoveConstSharedPtr>(), args("self"),
           "Returns the :class:`~mantid.geometry.Instrument` for this run.")

      .def("getInstrumentFilename", &ExperimentInfo::getInstrumentFilename,
           getInstrumentFilename_Overload(
               "Returns IDF filename", (arg("instrument"), arg("date") = "")))
      .staticmethod("getInstrumentFilename")

      .def("sample", &ExperimentInfo::sample,
           return_value_policy<reference_existing_object>(), args("self"),
           "Return the :class:`~mantid.api.Sample` object. This cannot be "
           "modified, use mutableSample to modify.")

      .def("mutableSample", &ExperimentInfo::mutableSample,
           return_value_policy<reference_existing_object>(), args("self"),
           "Return a modifiable :class:`~mantid.api.Sample` object.")

      .def("run", &ExperimentInfo::run,
           return_value_policy<reference_existing_object>(), args("self"),
           "Return the :class:`~mantid.api.Run` object. This cannot be "
           "modified, use mutableRun to modify.")

      .def("mutableRun", &ExperimentInfo::mutableRun,
           return_value_policy<reference_existing_object>(), args("self"),
           "Return a modifiable :class:`~mantid.api.Run` object.")

      .def("getRunNumber", &ExperimentInfo::getRunNumber, args("self"),
           "Returns the run identifier for this run.")

      .def("getEFixed",
           (double (ExperimentInfo::*)(const Mantid::detid_t) const) &
               ExperimentInfo::getEFixed,
           args("self", "detId"))

      .def("setEFixed", &ExperimentInfo::setEFixed,
           args("self", "detId", "value"))

      .def("getEMode", &ExperimentInfo::getEMode, args("self"),
           "Returns the energy mode.")

      .def("detectorInfo", &ExperimentInfo::detectorInfo,
           return_value_policy<reference_existing_object>(), args("self"),
           "Return a const reference to the "
           ":class:`~mantid.geometry.DetectorInfo` "
           "object.")
      .def("spectrumInfo", &ExperimentInfo::spectrumInfo,
           return_value_policy<reference_existing_object>(), args("self"),
           "Return a const reference to the :class:`~mantid.api.SpectrumInfo` "
           "object.")
      .def("componentInfo", &ExperimentInfo::componentInfo,
           return_value_policy<reference_existing_object>(), args("self"),
           "Return a const reference to the "
           ":class:`~mantid.geometry.ComponentInfo` "
           "object.");
}
