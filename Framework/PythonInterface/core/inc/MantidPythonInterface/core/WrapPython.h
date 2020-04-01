// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

// Including Python.h from a location where a "slots" is an active macro,
// e.g. when a Qt header is included, causes a failure under Python 3
// due to a slots field inside the PyType_Spec type being redefined.

// Include this header instead of Python.h to avoid this.

#pragma push_macro("slots")
#undef slots
#include <boost/python/detail/wrap_python.hpp>
#pragma pop_macro("slots")
