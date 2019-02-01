// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/DetectorInfoItem.h"
#include "MantidGeometry/Instrument/DetectorInfoIterator.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
#include "MantidPythonInterface/api/DetectorInfoPythonIterator.h"
#include "MantidPythonInterface/core/Converters/WrapWithNDArray.h"
#include "MantidPythonInterface/kernel/Policies/VectorToNumpy.h"
#include <boost/iterator/iterator_facade.hpp>
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/copy_non_const_reference.hpp>
#include <boost/python/iterator.hpp>
#include <boost/python/return_by_value.hpp>
#include <boost/python/return_value_policy.hpp>
#include <iterator>

using Mantid::Geometry::DetectorInfo;
using Mantid::Geometry::DetectorInfoItem;
using Mantid::Geometry::DetectorInfoIterator;
using Mantid::PythonInterface::DetectorInfoPythonIterator;

using Mantid::Kernel::Quat;
using Mantid::Kernel::V3D;
using namespace boost::python;
using namespace Mantid::PythonInterface;

namespace {
// Helper method to make the python iterator
DetectorInfoPythonIterator make_pyiterator(DetectorInfo &detectorInfo) {
  return DetectorInfoPythonIterator(detectorInfo);
}
/// return_value_policy for read-only numpy array
using return_readonly_numpy =
    return_value_policy<Policies::VectorRefToNumpy<Converters::WrapReadOnly>>;
} // namespace

// Export DetectorInfo
void export_DetectorInfo() {

  // Function pointers to distinguish between overloaded versions
  bool (DetectorInfo::*isMonitor)(const size_t) const =
      &DetectorInfo::isMonitor;

  bool (DetectorInfo::*isMasked)(const size_t) const = &DetectorInfo::isMasked;

  double (DetectorInfo::*twoTheta)(const size_t) const =
      &DetectorInfo::twoTheta;

  Mantid::Kernel::V3D (DetectorInfo::*position)(const size_t) const =
      &DetectorInfo::position;

  Mantid::Kernel::Quat (DetectorInfo::*rotation)(const size_t) const =
      &DetectorInfo::rotation;

  void (DetectorInfo::*setMasked)(const size_t, bool) =
      &DetectorInfo::setMasked;

  double (DetectorInfo::*l2)(const size_t) const = &DetectorInfo::l2;

  // Export to Python
  class_<DetectorInfo, boost::noncopyable>("DetectorInfo", no_init)

      .def("__iter__", make_pyiterator)

      .def("__len__", &DetectorInfo::size, (arg("self")),
           "Returns the size of the DetectorInfo, i.e., the number of "
           "detectors in the instrument.")

      .def("size", &DetectorInfo::size, (arg("self")),
           "Returns the size of the DetectorInfo, i.e., the number of "
           "detectors in the instrument.")

      .def("isMonitor", isMonitor, (arg("self"), arg("index")),
           "Returns True if the detector is a monitor.")

      .def("isMasked", isMasked, (arg("self"), arg("index")),
           "Returns True if the detector is masked.")

      .def("setMasked", setMasked, (arg("self"), arg("index"), arg("masked")),
           "Set the mask flag of the detector where the detector is identified "
           "by 'index'.")

      .def("clearMaskFlags", &DetectorInfo::clearMaskFlags, (arg("self")),
           "Sets all mask flags to false (unmasked).")

      .def("isEquivalent", &DetectorInfo::isEquivalent,
           (arg("self"), arg("other")),
           "Returns True if the content of this "
           "detector is equivalent to the content "
           "of the other detector.")

      .def("twoTheta", twoTheta, (arg("self"), arg("index")),
           "Returns 2 theta (scattering angle w.r.t beam direction).")

      .def("position", position, (arg("self"), arg("index")),
           "Returns the absolute position of the detector where the detector "
           "is identified by 'index'.")

      .def("rotation", rotation, (arg("self"), arg("index")),
           "Returns the absolute rotation of the detector where the detector "
           "is identified by 'index'.")
      .def("detectorIDs", &DetectorInfo::detectorIDs, return_readonly_numpy(),
           "Returns all detector ids sorted by detector index")
      .def("l2", l2, (arg("self"), arg("index")),
           "Returns the l2 scattering distance")
      .def("l1", &DetectorInfo::l1, arg("self"),
           "Returns the l1 scattering distance");
}
