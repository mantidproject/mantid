// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/core/Policies/RemoveConst.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using namespace Mantid::Geometry;
using Mantid::detid_t;
using namespace boost::python;
using Mantid::PythonInterface::Policies::RemoveConstSharedPtr;

GET_POINTER_SPECIALIZATION(Instrument)

namespace {

// Deprecation wrappers. Legacy Instrument-tree access is being retired in favour
// of the index-based ComponentInfo/DetectorInfo access layers. See the
// 'Instrument Access via SpectrumInfo, DetectorInfo, ComponentInfo' concept page.
IComponent_const_sptr getSampleDeprecated(const Instrument &self) {
  PyErr_Warn(PyExc_DeprecationWarning, "'Instrument.getSample' is deprecated in Mantid 7.0, "
                                       "use 'ComponentInfo.sample'/'samplePosition' instead. "
                                       "For more information, see the instrument access layers concept page: "
                                       "https://docs.mantidproject.org/nightly/concepts/InstrumentAccessLayers.html");
  return self.getSample();
}

IComponent_const_sptr getSourceDeprecated(const Instrument &self) {
  PyErr_Warn(PyExc_DeprecationWarning, "'Instrument.getSource' is deprecated in Mantid 7.0, "
                                       "use 'ComponentInfo.source'/'sourcePosition' instead. "
                                       "For more information, see the instrument access layers concept page: "
                                       "https://docs.mantidproject.org/nightly/concepts/InstrumentAccessLayers.html");
  return self.getSource();
}

IComponent_const_sptr getComponentByNameDeprecated(const Instrument &self, const std::string &cname,
                                                   const int nlevels) {
  PyErr_Warn(PyExc_DeprecationWarning, "'Instrument.getComponentByName' is deprecated in Mantid 7.0, "
                                       "use 'ComponentInfo.indexOfAny' instead. "
                                       "For more information, see the instrument access layers concept page: "
                                       "https://docs.mantidproject.org/nightly/concepts/InstrumentAccessLayers.html");
  return self.getComponentByName(cname, nlevels);
}

IDetector_const_sptr getDetectorDeprecated(const Instrument &self, const detid_t &detector_id) {
  PyErr_Warn(PyExc_DeprecationWarning, "'Instrument.getDetector' is deprecated in Mantid 7.0, "
                                       "use 'DetectorInfo.indexOf' instead. "
                                       "For more information, see the instrument access layers concept page: "
                                       "https://docs.mantidproject.org/nightly/concepts/InstrumentAccessLayers.html");
  return self.getDetector(detector_id);
}

std::size_t getNumberDetectorsDeprecated(const Instrument &self, const bool skipMonitors = false) {
  PyErr_Warn(PyExc_DeprecationWarning, "'Instrument.getNumberDetectors' is deprecated in Mantid 7.0, "
                                       "use 'DetectorInfo.size' instead. "
                                       "For more information, see the instrument access layers concept page: "
                                       "https://docs.mantidproject.org/nightly/concepts/InstrumentAccessLayers.html");
  return self.getNumberDetectors(skipMonitors);
}

std::vector<RectangularDetector_const_sptr> findRectDetectorsDeprecated(const Instrument &self) {
  PyErr_Warn(PyExc_DeprecationWarning, "'Instrument.findRectDetectors' is deprecated in Mantid 7.0, "
                                       "use 'ComponentInfo.componentType' to identify Rectangular banks instead. "
                                       "For more information, see the instrument access layers concept page: "
                                       "https://docs.mantidproject.org/nightly/concepts/InstrumentAccessLayers.html");
  return self.findRectDetectors();
}

std::vector<GridDetector_const_sptr> findGridDetectorsDeprecated(const Instrument &self) {
  PyErr_Warn(PyExc_DeprecationWarning, "'Instrument.findGridDetectors' is deprecated in Mantid 7.0, "
                                       "use 'ComponentInfo.componentType' to identify Grid banks instead. "
                                       "For more information, see the instrument access layers concept page: "
                                       "https://docs.mantidproject.org/nightly/concepts/InstrumentAccessLayers.html");
  return self.findGridDetectors();
}

// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GNU_DIAG_OFF("conversion")

BOOST_PYTHON_FUNCTION_OVERLOADS(Instrument_getNumberDetectors, getNumberDetectorsDeprecated, 1, 2)

GNU_DIAG_ON("conversion")

} // namespace

void export_Instrument() {
  register_ptr_to_python<std::shared_ptr<Instrument>>();

  class_<Instrument, bases<CompAssembly>, boost::noncopyable>("Instrument", no_init)
      .def("getSample", &getSampleDeprecated, arg("self"), return_value_policy<RemoveConstSharedPtr>(),
           "Return the :class:`~mantid.geometry.Component` object that "
           "represents the sample (deprecated, use ComponentInfo.sample)")

      .def("getSource", &getSourceDeprecated, arg("self"), return_value_policy<RemoveConstSharedPtr>(),
           "Return the :class:`~mantid.geometry.Component` object that "
           "represents the source (deprecated, use ComponentInfo.source)")

      .def("getComponentByName", &getComponentByNameDeprecated, (arg("self"), arg("cname"), arg("nlevels") = 0),
           return_value_policy<RemoveConstSharedPtr>(),
           "Returns the named :class:`~mantid.geometry.Component` "
           "(deprecated, use ComponentInfo.indexOfAny)")

      .def("getDetector", &getDetectorDeprecated, (arg("self"), arg("detector_id")),
           return_value_policy<RemoveConstSharedPtr>(),
           "Returns the :class:`~mantid.geometry.Detector` with the given ID "
           "(deprecated, use DetectorInfo.indexOf)")

      .def("getDefaultView", &Instrument::getDefaultView, arg("self"), return_value_policy<copy_const_reference>(),
           "Return the name of the preferred view in instrument view.")

      .def("getXmlText", &Instrument::getXmlText, arg("self"), return_value_policy<copy_const_reference>(),
           "Return the instrument XML.")

      .def("getNumberDetectors", &getNumberDetectorsDeprecated,
           Instrument_getNumberDetectors("Return the number of detectors (deprecated, use DetectorInfo.size)",
                                         (arg("self"), arg("skipMonitors") = false)))

      .def("getReferenceFrame", (std::shared_ptr<const ReferenceFrame> (Instrument::*)())&Instrument::getReferenceFrame,
           arg("self"), return_value_policy<RemoveConstSharedPtr>(),
           "Returns the :class:`~mantid.geometry.ReferenceFrame` attached that "
           "defines the instrument "
           "axes")

      .def("getValidFromDate", &Instrument::getValidFromDate, arg("self"),
           "Return the valid from :class:`~mantid.kernel.DateAndTime` of the "
           "instrument")

      .def("getValidToDate", &Instrument::getValidToDate, arg("self"),
           "Return the valid to :class:`~mantid.kernel.DateAndTime` of the "
           "instrument")

      .def("getFilename", &Instrument::getFilename, arg("self"), return_value_policy<copy_const_reference>(),
           "Return the name of the file that the original IDF was from")

      .def("setFilename", &Instrument::setFilename, (arg("self"), arg("filename")),
           "Set the name of the file that the original IDF was from")

      .def("getBaseInstrument", &Instrument::baseInstrument, arg("self"), return_value_policy<RemoveConstSharedPtr>(),
           "Return reference to the base instrument")

      .def("findRectDetectors", &findRectDetectorsDeprecated, arg("self"),
           "Return a list of rectangular detectors "
           "(deprecated, use ComponentInfo.componentType).")
      .def("findGridDetectors", &findGridDetectorsDeprecated, arg("self"),
           "Return a list of grid detectors "
           "(deprecated, use ComponentInfo.componentType).")
      .def("getMemorySize", &Instrument::getMemorySize, arg("self"),
           "Return the memory footprint of the instrument in bytes.");
}
