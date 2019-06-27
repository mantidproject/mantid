// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_CORE_WRAPPYTHON_H
#define MANTID_PYTHONINTERFACE_CORE_WRAPPYTHON_H

// Including Python.h from a location where a "slots" is an active macro,
// e.g. when a Qt header is included, causes a failure under Python 3
// due to a slots field inside the PyType_Spec type being redefined.

// Include this header instead of Python.h to avoid this.

#pragma push_macro("slots")
#undef slots
#include <boost/python/detail/wrap_python.hpp>
#pragma pop_macro("slots")

#endif // MANTID_PYTHONINTERFACE_CORE_WRAPPYTHON_H
