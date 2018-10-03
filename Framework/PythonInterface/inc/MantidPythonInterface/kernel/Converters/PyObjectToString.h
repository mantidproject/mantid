// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINERFACE_CONVERTERS_PYOBJECTTOSTRING_H_
#define MANTID_PYTHONINERFACE_CONVERTERS_PYOBJECTTOSTRING_H_

#include "MantidKernel/System.h"
#include <boost/python/object.hpp>

namespace Mantid {
namespace PythonInterface {
namespace Converters {

/**
 * Convert a python object to a string or throw an exception. This will convert
 * unicode strings in python2 via utf8.
 */
DLLExport std::string pyObjToStr(const boost::python::object &value);

} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINERFACE_CONVERTERS_PYOBJECTTOSTRING_H_ */
