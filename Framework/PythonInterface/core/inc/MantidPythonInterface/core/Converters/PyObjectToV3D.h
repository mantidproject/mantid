// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINERFACE_PYOBJECTTOV3D_H_
#define MANTID_PYTHONINERFACE_PYOBJECTTOV3D_H_

#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"
#include <boost/python/object.hpp>

namespace Mantid {
namespace PythonInterface {
namespace Converters {
/**
 * Takes a Python object and if it supports
 * indexing and is of length 3 then it will
 * attempt to convert a Kernel::V3D object from
 * it
 */
struct DLLExport PyObjectToV3D {
  PyObjectToV3D(const boost::python::object &p);
  /// Produces a V3D object from the given PyObject
  Kernel::V3D operator()();

private:
  /// A reference to the object
  const boost::python::object &m_obj;
  /// Is the object a wrapped instance of V3D
  bool m_alreadyV3D;
};
} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINERFACE_PYOBJECTTOV3D_H_ */
