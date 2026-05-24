// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX-License-Identifier: GPL-3.0+
#include "MantidLiveData/Exception.h"

namespace Mantid {
namespace LiveData {
namespace Exception {

NotYet::NotYet(const std::string &message) : std::runtime_error(message) {}

// Out-of-line destructor is the key function: it pins the vtable and typeinfo
// for NotYet to this translation unit so only one copy exists per process.
NotYet::~NotYet() = default;

} // namespace Exception
} // namespace LiveData
} // namespace Mantid
