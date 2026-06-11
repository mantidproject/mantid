// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/DetectorInfoItem.h"
#include "MantidGeometry/Instrument/DetectorInfoIterator.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"
#include "MantidPythonInterface/core/Converters/WrapWithNDArray.h"
#include "MantidPythonInterface/core/Policies/VectorToNumpy.h"
#include "MantidPythonInterface/geometry/DetectorInfoPythonIterator.h"
#include <boost/iterator/iterator_facade.hpp>
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/copy_non_const_reference.hpp>
#include <boost/python/iterator.hpp>
#include <boost/python/return_by_value.hpp>
#include <boost/python/return_value_policy.hpp>
#include <iterator>

#define PY_ARRAY_UNIQUE_SYMBOL GEOMETRY_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/ndarrayobject.h>

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
using return_readonly_numpy = return_value_policy<Policies::VectorRefToNumpy<Converters::WrapReadOnly>>;

PyObject *numpyArrayFromVector(const std::vector<V3D> &vec) {
  const size_t n_detectors = vec.size();
  constexpr size_t vec_size = 3;
  npy_intp dims[2] = {static_cast<npy_intp>(n_detectors), vec_size};
  PyObject *numpy_array = PyArray_SimpleNew(2, dims, NPY_DOUBLE);
  if (!numpy_array) {
    PyErr_SetString(PyExc_RuntimeError, "Failed to create numpy array");
    throw boost::python::error_already_set();
  }
  double *data = static_cast<double *>(PyArray_DATA(reinterpret_cast<PyArrayObject *>(numpy_array)));
  for (size_t i = 0; i < n_detectors; ++i) {
    const size_t base_index = i * vec_size;
    data[base_index + 0] = vec[i].X();
    data[base_index + 1] = vec[i].Y();
    data[base_index + 2] = vec[i].Z();
  }
  return numpy_array;
}

PyObject *allPositions(const DetectorInfo &self) {
  const std::vector<V3D> positions = self.allPositions();
  return numpyArrayFromVector(positions);
}

PyObject *allScaleFactors(const DetectorInfo &self) {
  const std::vector<V3D> positions = self.allScaleFactors();
  return numpyArrayFromVector(positions);
}

PyObject *allRotations(const DetectorInfo &self) {
  const std::vector<Quat> rotations = self.allRotations();
  const size_t n_detectors = rotations.size();
  const size_t quat_size = 4;
  npy_intp dims[2] = {static_cast<npy_intp>(n_detectors), quat_size};
  PyObject *numpy_rotations_array = PyArray_SimpleNew(2, dims, NPY_DOUBLE);
  if (!numpy_rotations_array) {
    PyErr_SetString(PyExc_RuntimeError, "Failed to create numpy array");
    throw boost::python::error_already_set();
  }
  double *data = static_cast<double *>(PyArray_DATA(reinterpret_cast<PyArrayObject *>(numpy_rotations_array)));
  for (size_t i = 0; i < n_detectors; ++i) {
    const size_t base_index = i * quat_size;
    data[base_index + 0] = rotations[i].imagI();
    data[base_index + 1] = rotations[i].imagJ();
    data[base_index + 2] = rotations[i].imagK();
    data[base_index + 3] = rotations[i].real();
  }
  return numpy_rotations_array;
}

} // namespace

// Export DetectorInfo
void export_DetectorInfo() {

  // Function pointers to distinguish between overloaded versions
  bool (DetectorInfo::*isMonitor)(const size_t) const = &DetectorInfo::isMonitor;

  bool (DetectorInfo::*isMasked)(const size_t) const = &DetectorInfo::isMasked;

  double (DetectorInfo::*twoTheta)(const size_t) const = &DetectorInfo::twoTheta;

  double (DetectorInfo::*signedTwoTheta)(const size_t) const = &DetectorInfo::signedTwoTheta;

  double (DetectorInfo::*azimuthal)(const size_t) const = &DetectorInfo::azimuthal;

  Mantid::Kernel::V3D (DetectorInfo::*position)(const size_t) const = &DetectorInfo::position;

  Mantid::Kernel::Quat (DetectorInfo::*rotation)(const size_t) const = &DetectorInfo::rotation;

  void (DetectorInfo::*setMasked)(const size_t, bool) = &DetectorInfo::setMasked;

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

      .def("indexOf", &DetectorInfo::indexOf, (arg("self"), arg("detId")),
           "Returns the index of the detector with the given id.")

      .def("isMonitor", isMonitor, (arg("self"), arg("index")), "Returns True if the detector is a monitor.")

      .def("isMasked", isMasked, (arg("self"), arg("index")), "Returns True if the detector is masked.")

      .def("setMasked", setMasked, (arg("self"), arg("index"), arg("masked")),
           "Set the mask flag of the detector where the detector is identified "
           "by 'index'.")

      .def("clearMaskFlags", &DetectorInfo::clearMaskFlags, (arg("self")), "Sets all mask flags to false (unmasked).")

      .def("isEquivalent", &DetectorInfo::isEquivalent, (arg("self"), arg("other")),
           "Returns True if the content of this "
           "detector is equivalent to the content "
           "of the other detector.")

      .def("twoTheta", twoTheta, (arg("self"), arg("index")),
           "Returns 2 theta (scattering angle w.r.t beam direction).")
      .def("signedTwoTheta", signedTwoTheta, (arg("self"), arg("index")),
           "Returns signed 2 theta (signed scattering angle w.r.t beam direction).")
      .def("azimuthal", azimuthal, (arg("self"), arg("index")),
           "Returns the out-of-plane angle in radians angle w.r.t. to "
           "vecPointingHorizontal")
      .def("position", position, (arg("self"), arg("index")),
           "Returns the absolute position of the detector where the detector "
           "is identified by 'index'.")
      .def("allPositions", &allPositions, (arg("self")),
           "Returns the absolute positions of all detectors as a numpy array.")
      .def("rotation", rotation, (arg("self"), arg("index")),
           "Returns the absolute rotation of the detector where the detector "
           "is identified by 'index'.")
      .def("allRotations", &allRotations, (arg("self")),
           "Returns the absolute rotations of all detectors as a numpy array of quarternions, [i, j, k, w].")
      .def("allScaleFactors", &allScaleFactors, (arg("self")),
           "Returns the scale factors of all detectors as a numpy array.")
      .def("detectorIDs", &DetectorInfo::detectorIDs, return_readonly_numpy(),
           "Returns all detector ids sorted by detector index")
      .def("l2", l2, (arg("self"), arg("index")), "Returns the l2 scattering distance")
      .def("l1", &DetectorInfo::l1, arg("self"), "Returns the l1 scattering distance")
      .def("hasMaskedDetectors", &DetectorInfo::hasMaskedDetectors, arg("self"),
           "Returns if there are masked detectors")
      .def("getMemorySize", &DetectorInfo::getMemorySize, arg("self"),
           "Return the memory footprint of the detector info in bytes.");
}
