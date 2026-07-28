// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/SolidAngleParams.h"
#include "MantidKernel/V3D.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Geometry::IDetector;
using Mantid::Geometry::IObjComponent;
using Mantid::Kernel::V3D;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(IDetector)

namespace {
// Deprecation wrappers. Legacy detector access is being retired in favour of the
// index-based DetectorInfo/ComponentInfo access layers. See the 'Instrument
// Access via SpectrumInfo, DetectorInfo, ComponentInfo' concept page.
Mantid::detid_t getIDDeprecated(const IDetector &self) {
  PyErr_Warn(PyExc_DeprecationWarning, "'IDetector.getID' is deprecated in Mantid 7.0, "
                                       "use 'DetectorInfo.detectorIDs' instead. "
                                       "For more information, see the instrument access layers concept page: "
                                       "https://docs.mantidproject.org/nightly/concepts/InstrumentAccessLayers.html");
  return self.getID();
}

double solidAngleDeprecated(const IDetector &self, const V3D &observer) {
  PyErr_Warn(PyExc_DeprecationWarning, "'IDetector.solidAngle' is deprecated in Mantid 7.0, "
                                       "use 'ComponentInfo.solidAngle' instead. "
                                       "For more information, see the instrument access layers concept page: "
                                       "https://docs.mantidproject.org/nightly/concepts/InstrumentAccessLayers.html");
  // Redirect through the preferred ComponentInfo access layer. A detector's
  // component index equals its detector index.
  const auto &parameterMap = self.parameterMap();
  const auto index = parameterMap.detectorInfo().indexOf(self.getID());
  return parameterMap.componentInfo().solidAngle(index, Mantid::Geometry::SolidAngleParams(observer));
}

double getTwoThetaDeprecated(const IDetector &self, const V3D &observer, const V3D &axis) {
  PyErr_Warn(PyExc_DeprecationWarning, "'IDetector.getTwoTheta' is deprecated in Mantid 7.0, "
                                       "use 'DetectorInfo.twoTheta' instead. "
                                       "For more information, see the instrument access layers concept page: "
                                       "https://docs.mantidproject.org/nightly/concepts/InstrumentAccessLayers.html");
  return self.getTwoTheta(observer, axis);
}

double getPhiDeprecated(const IDetector &self) {
  PyErr_Warn(PyExc_DeprecationWarning, "'IDetector.getPhi' is deprecated in Mantid 7.0, "
                                       "use 'DetectorInfo.azimuthal' instead. "
                                       "For more information, see the instrument access layers concept page: "
                                       "https://docs.mantidproject.org/nightly/concepts/InstrumentAccessLayers.html");
  return self.getPhi();
}
} // namespace

void export_IDetector() {
  register_ptr_to_python<std::shared_ptr<IDetector>>();
  register_ptr_to_python<std::shared_ptr<const IDetector>>();

  class_<IDetector, bases<IObjComponent>, boost::noncopyable>("IDetector", no_init)
      .def("getID", &getIDDeprecated, arg("self"), "Returns the detector ID (deprecated, use DetectorInfo.detectorIDs)")
      .def("solidAngle", &solidAngleDeprecated, (arg("self"), arg("observer")),
           "Return the solid angle in steradians between this "
           "detector and an observer (deprecated, use ComponentInfo.solidAngle)")
      .def("getTwoTheta", &getTwoThetaDeprecated, (arg("self"), arg("observer"), arg("axis")),
           "Calculate the angle between this detector, another component and "
           "an axis (deprecated, use DetectorInfo.twoTheta)")
      .def("getPhi", &getPhiDeprecated, arg("self"),
           "Returns the azimuthal angle of this detector (deprecated, use DetectorInfo.azimuthal)");
}
