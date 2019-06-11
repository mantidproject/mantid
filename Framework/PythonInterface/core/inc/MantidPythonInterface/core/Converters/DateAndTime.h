// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINERFACE_CORE_DATEANDTIME_H_
#define MANTID_PYTHONINERFACE_CORE_DATEANDTIME_H_

#include "MantidKernel/DateAndTime.h"
#include "MantidPythonInterface/core/DllConfig.h"
#include <boost/python/object.hpp>
#include <numpy/ndarraytypes.h>

namespace Mantid {
namespace PythonInterface {
namespace Converters {

/// Convert to numpy's datetime64. This is panda's name for the function.
MANTID_PYTHONINTERFACE_CORE_DLL PyObject *
to_datetime64(const Types::Core::DateAndTime &dateandtime);
/// Total nanoseconds since the unix epoch
MANTID_PYTHONINTERFACE_CORE_DLL npy_datetime
to_npy_datetime(const Types::Core::DateAndTime &dateandtime);
MANTID_PYTHONINTERFACE_CORE_DLL PyArray_Descr *descr_ns();
MANTID_PYTHONINTERFACE_CORE_DLL boost::shared_ptr<Types::Core::DateAndTime>
to_dateandtime(const boost::python::api::object &value);
} // namespace Converters
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINERFACE_CORE_DATEANDTIME_H_ */
