// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_REGISTRATIONHELPER_H_
#define MANTID_KERNEL_REGISTRATIONHELPER_H_

#include "MantidKernel/System.h"

namespace Mantid {
namespace Kernel {

/**
 * This class is simply used in the subscription of classes into the various
 * factories in Mantid. The fact that the constructor takes an int means that
 * the comma operator can be used to make a call to the factories' subscribe
 * method in the first part.
 */
class DLLExport RegistrationHelper {
public:
  /** Constructor. Does nothing.
   * @param i :: Takes an int and does nothing with it
   */
  inline RegistrationHelper(int i) { UNUSED_ARG(i); }
};
} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_REGISTRATIONHELPER_H_ */
