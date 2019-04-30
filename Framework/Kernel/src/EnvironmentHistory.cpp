// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/EnvironmentHistory.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/MantidVersion.h"
#include <ostream>

namespace Mantid {
namespace Kernel {

/**
 * Returns the framework version
 * @returns A string containing the Mantid version
 */
std::string EnvironmentHistory::frameworkVersion() const {
  return MantidVersion::version();
}
/**
 * Returns the OS name
 * @returns A string containing the OS name
 */
std::string EnvironmentHistory::osName() const {
  return ConfigService::Instance().getOSName();
}
/**
 * Returns the OS version
 * @returns A string containing the OS version
 */
std::string EnvironmentHistory::osVersion() const {
  return ConfigService::Instance().getOSVersion();
}

/** Prints a text representation of itself
 *  @param os :: The ouput stream to write to
 *  @param indent :: an indentation value to make pretty printing of object and
 * sub-objects
 */
void EnvironmentHistory::printSelf(std::ostream &os, const int indent) const {
  os << std::string(indent, ' ') << "Framework Version: " << frameworkVersion()
     << '\n';
  os << std::string(indent, ' ') << "OS name: " << osName() << '\n';
  os << std::string(indent, ' ') << "OS version: " << osVersion() << '\n';
}

/** Prints a text representation
 *  @param os :: The ouput stream to write to
 *  @param EH :: The EnvironmentHistory to output
 *  @returns The ouput stream
 */
std::ostream &operator<<(std::ostream &os, const EnvironmentHistory &EH) {
  EH.printSelf(os);
  return os;
}

} // namespace Kernel
} // namespace Mantid
