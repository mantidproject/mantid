// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX-License-Identifier: GPL-3.0+
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidLiveData/DllConfig.h"
#include <stdexcept>
#include <string>

namespace Mantid {
namespace LiveData {
namespace Exception {

/** An exception that can be thrown by an ILiveListener implementation to
    notify LoadLiveData that it is not yet ready to return data. This could
    be, for example, because it has not yet completed its initialisation step
    or if the instrument from which data is being read is not in a run.
    LoadLiveData will ask for data again after a short delay.  Other exceptions
    thrown by the listener will have the effect of stopping the algorithm.

    This class has an out-of-line constructor and destructor (key function)
    defined in Exception.cpp and is exported with MANTID_LIVEDATA_DLL.  That
    pins its typeinfo and vtable to a single translation unit inside
    libMantidLiveData, which is required for @c catch(NotYet&) to work across
    dylib boundaries on macOS (libc++abi matches catch clauses by typeinfo
    pointer equality).
*/
class MANTID_LIVEDATA_DLL NotYet : public std::runtime_error {
public:
  explicit NotYet(const std::string &message);
  ~NotYet() override;
};

} // namespace Exception
} // namespace LiveData
} // namespace Mantid
