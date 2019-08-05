// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINERFACE_PYOBJECTTOVMD_H_
#define MANTID_PYTHONINERFACE_PYOBJECTTOVMD_H_

#include "MantidKernel/System.h"
#include "MantidKernel/VMD.h"
#include <boost/python/object.hpp>

namespace Mantid {
namespace PythonInterface {
namespace Converters {
/**
 * Takes a Python object and if it supports
 * indexing and is of length 3 then it will
 * attempt to convert a Kernel::VMD object from
 * it
 */
struct DLLExport PyObjectToVMD {
  PyObjectToVMD(const boost::python::object &p);
  /// Produces a VMD object from the given PyObject
  Kernel::VMD operator()();

private:
  /// A reference to the object
  const boost::python::object &m_obj;
  /// Is the object a wrapped instance of VMD
  bool m_alreadyVMD;
};
} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINERFACE_PYOBJECTTOVMD_H_ */
