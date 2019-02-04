// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_GLOB_H_
#define MANTID_KERNEL_GLOB_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include <Poco/Glob.h>
#include <Poco/Path.h>

#include <set>
#include <string>

namespace Mantid {
namespace Kernel {
/** This Glob class overrides the glob() method of Poco::Glob class
    to make it more reliable.

    @author Roman Tolchenov, Tessella plc
    @date 23/07/2009
*/

class MANTID_KERNEL_DLL Glob : public Poco::Glob {
public:
  /// Creates a set of files that match the given pathPattern.
  static void glob(const Poco::Path &pathPattern, std::set<std::string> &files,
                   int options = 0);
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_GLOB_H_*/
