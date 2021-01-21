// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/core/DllConfig.h"
#include <boost/python/wrapper.hpp>

namespace Mantid {
namespace PythonInterface {

/**
This namespace contains helper functions for classes that are overridden in
Python
*/
/// Checks whether the given object's type dictionary contains the named
/// attribute.
bool MANTID_PYTHONINTERFACE_CORE_DLL typeHasAttribute(PyObject *obj, const char *attr);
/// An overload for the above taking a wrapper reference
bool MANTID_PYTHONINTERFACE_CORE_DLL typeHasAttribute(const boost::python::detail::wrapper_base &wrapper,
                                                      const char *attr);

} // namespace PythonInterface
} // namespace Mantid
