// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_DLLOPEN_H_
#define MANTID_KERNEL_DLLOPEN_H_

#include <string>

#include "MantidKernel/DllConfig.h"

namespace Mantid {
namespace Kernel {

/** @class DllOpen DllOpen.h

 Simple class for opening shared libraries at run-time. Works for Windows and
 Linux.

 @author ISIS, STFC
 @date 25/10/2007
 */
class MANTID_KERNEL_DLL DllOpen {
public:
  // Unconstructible
  DllOpen() = delete;
  // Not copyable
  DllOpen(const DllOpen &) = delete;
  ~DllOpen() = delete;

public:
  /// Check if the filename conforms to the expected style for this platform
  static bool isValidFilename(const std::string &filename);

  /// Static method for opening the shared library
  static void *openDll(const std::string &filepath);

  /// Static method for closing the shared library
  static void closeDll(void *handle);

private:
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_DLLOPEN_H_*/
