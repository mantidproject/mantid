// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"

#include <set>
#include <string>

namespace Mantid {
namespace Kernel {
/**
   This Glob class uses the glob() method of Poco::Glob class to make it more reliable.

    @author Roman Tolchenov, Tessella plc
    @date 23/07/2009
*/

class MANTID_KERNEL_DLL Glob {
public:
  /// Glob option constants (compatible with Poco::Glob)
  static constexpr int GLOB_DEFAULT = 0;
  static constexpr int GLOB_CASELESS = 4;

  /// Creates a set of files that match the given pathPattern.
  static void glob(const std::string &pathPattern, std::set<std::string> &files, int options = 0);

  /**
   * Convert a glob pattern to an equivalent regular expression. This essentially converts non-escaped "*" to ".+" and
   * non-escaped "?" to ".". This is how Poco's Glob module acts.
   */
  static std::string globToRegex(const std::string &globPattern);
};

} // namespace Kernel
} // namespace Mantid
