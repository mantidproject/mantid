// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"

namespace Mantid {
namespace Kernel {

/**
 * This class is simply used in the subscription of classes into the various
 * factories in Mantid. The fact that the constructor takes an int means that
 * the comma operator can be used to make a call to the factories' subscribe
 * method in the first part.
 *
 * The MANTID_KERNEL_DLL needs to be available where this helper is imported and used.
 */
class MANTID_KERNEL_DLL RegistrationHelper { // clang-format off
public:
  /** Constructor. Does nothing.
   * @param i :: Takes an int and does nothing with it
   */
  inline RegistrationHelper(int i) { UNUSED_ARG(i); }
};
// clang-format on
} // namespace Kernel
} // namespace Mantid
