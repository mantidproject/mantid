// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#ifndef Q_MOC_RUN
#include <boost/regex.hpp>
#endif

namespace Mantid {
namespace Kernel {
namespace Strings {
/// Find if a pattern matches a string
template <typename T> MANTID_KERNEL_DLL int StrComp(const char *, const boost::regex &, T &, const int = 0);

/// Find if a pattern matches
template <typename T> MANTID_KERNEL_DLL int StrComp(const std::string &, const boost::regex &, T &, const int = 0);

/// Find is a pattern matches
MANTID_KERNEL_DLL int StrLook(const std::string &, const boost::regex &);

/// Split  a line into component parts
MANTID_KERNEL_DLL std::vector<std::string> StrParts(std::string, const boost::regex &);

/// Split  a line searched parts
template <typename T> int StrFullSplit(const std::string &, const boost::regex &, std::vector<T> &);
/// Split  a line searched parts
template <typename T> int StrFullSplit(const char *, const boost::regex &, std::vector<T> &);
/// Split  a line searched parts
template <typename T> int StrSingleSplit(const std::string &, const boost::regex &, std::vector<T> &);
/// Split  a line searched parts
template <typename T> int StrSingleSplit(const char *, const boost::regex &, std::vector<T> &);

/// Cut out the searched section and returns component
template <typename T> int StrFullCut(std::string &, const boost::regex &, T &, const int = -1);
/// Cut out the searched section and returns component
template <typename T> int StrFullCut(std::string &, const boost::regex &, std::vector<T> &);

/// Find a compmonent in a Regex in a file
template <typename T> MANTID_KERNEL_DLL int findComp(std::istream &, const boost::regex &, T &);

/// Finds a pattern in a file
MANTID_KERNEL_DLL int findPattern(std::istream &, const boost::regex &, std::string &);

} // NAMESPACE Strings

} // NAMESPACE Kernel

} // NAMESPACE Mantid
