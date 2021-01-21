// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include <string>

namespace Mantid::Kernel {

/** ChecksumHelper : A selection of helper methods for calculating checksums
 */
namespace ChecksumHelper {
// loads a file, optionally converting line endings
MANTID_KERNEL_DLL std::string loadFile(const std::string &filepath, const bool unixEOL);
/// create a md5 checksum from a string
MANTID_KERNEL_DLL std::string md5FromString(const std::string &input);
/// create a SHA-1 checksum from a string
MANTID_KERNEL_DLL std::string sha1FromString(const std::string &input);
/// create a SHA-1 checksum from a file
MANTID_KERNEL_DLL std::string sha1FromFile(const std::string &filepath, const bool unixEOL);
/// create a git checksum from a file (these match the git hash-object command)
MANTID_KERNEL_DLL std::string gitSha1FromFile(const std::string &filepath);
} // namespace ChecksumHelper

} // namespace Mantid::Kernel
