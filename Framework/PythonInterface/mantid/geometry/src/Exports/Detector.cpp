#include "MantidGeometry/Instrument/Detector.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidBeamline/DetectorInfo.h"
#include <boost/python/class.hpp>

using Mantid::Geometry::Detector;
using Mantid::Geometry::IDetector;
using Mantid::Geometry::ObjComponent;
using namespace boost::python;

namespace {
bool isMaskedDeprecated(const Detector &self) {
  PyErr_Warn(PyExc_DeprecationWarning, "'Detector::isMasked' is deprecated, "
                                       "use 'DetectorInfo::isMasked' instead.");
  const auto &detInfo = self.parameterMap().detectorInfo();
  return detInfo.isMasked(self.index());
}
}

/**
 * Enables boost.python to automatically "cast" an object up to the
 * appropriate Detector leaf type
 */
void export_Detector() {
  class_<Detector, bases<IDetector, ObjComponent>, boost::noncopyable>(
      "Detector", no_init)
      .def("isMasked", &isMaskedDeprecated, arg("self"),
           "Returns the value of the masked flag. True means ignore this "
           "detector");
}
