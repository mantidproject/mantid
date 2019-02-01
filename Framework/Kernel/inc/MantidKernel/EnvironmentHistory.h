// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_ENVIRONMENTHISTORY_H_
#define MANTID_KERNEL_ENVIRONMENTHISTORY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include <iosfwd>
#include <string>

namespace Mantid {
namespace Kernel {
/** This class stores information about the Environment of the computer used by
 the framework.

  @author Dickon Champion, ISIS, RAL
  @date 21/01/2008
*/
class MANTID_KERNEL_DLL EnvironmentHistory {
public:
  /// returns the framework version
  std::string frameworkVersion() const;
  /// returns the os name
  std::string osName() const;
  /// returns the os version
  std::string osVersion() const;
  /// print contents of object
  void printSelf(std::ostream &, const int indent = 0) const;

private:
  /// Private, unimplemented copy assignment operator
  EnvironmentHistory &operator=(const EnvironmentHistory &);
};

MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &,
                                           const EnvironmentHistory &);

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_ENVIRONMENTHISTORY_H_*/
