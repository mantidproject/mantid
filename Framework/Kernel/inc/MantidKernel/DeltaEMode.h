// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_DELTAEMODE_H_
#define MANTID_KERNEL_DELTAEMODE_H_

#include "MantidKernel/DllConfig.h"
#include <string>
#include <vector>

namespace Mantid {
namespace Kernel {
/**
 * Defines the possible energy transfer modes:
 *   - Elastic
 *   - Direct
 *   - Indirect
 * and functions to convert to/from strings.
 * It also returns a list of the available modes
 */
struct MANTID_KERNEL_DLL DeltaEMode {
  /** Define the available energy transfer modes
   *  It is important to assign enums proper numbers, until direct
   * correspondence between enums and their emodes
   *  used by the external units conversion algorithms within the Mantid, so the
   * agreement should be the stame        */
  enum Type {
    Elastic = 0,
    Direct = 1,
    Indirect = 2,
    Undefined //< The type for the situations, where instrument can not be
    // reasonably defined (e.g.  ws with detector information lost)
    /// this mode should not be displayed among modes availible to select but
    /// may have string representation
  };
  /// Return a string representation of the given mode
  static std::string asString(const Type mode);
  /// Returns the emode from the given string
  static Type fromString(const std::string &modeStr);
  /// Returns the string list of available modes
  static const std::vector<std::string> availableTypes();
};
} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_DELTAEMODE_H_ */
