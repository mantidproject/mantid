// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Matrix.h"
#include "MantidKernel/System.h"
#include <boost/python/object.hpp>

namespace Mantid {
namespace PythonInterface {
namespace Converters {
/**
 * Takes a Python object and if it supports
 * indexing and is two dimensional it attempts to
 * convert it to a Kernel::Matrix object. Note, this
 * currently only suuports Matrix<double>
 */
struct DLLExport PyObjectToMatrix {
  PyObjectToMatrix(const boost::python::object &p);
  /// Produces a V3D object from the given PyObject
  Kernel::Matrix<double> operator()();

private:
  /// A reference to the object
  const boost::python::object &m_obj;
  /// Is the object a wrapped instance of Matrix<double>
  bool m_alreadyMatrix;
};
} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid
