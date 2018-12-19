// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/ICatalogInfo.h"
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

namespace Mantid {
namespace Kernel {

std::string ICatalogInfo::transformArchivePath(const std::string &path) const {
  std::string ret;
#ifdef __linux__
  ret = replacePrefix(path, catalogPrefix(), linuxPrefix());
  ret = replaceAllOccurences(ret, "\\", "/");
#elif __APPLE__
  ret = replacePrefix(path, catalogPrefix(), macPrefix());
  ret = replaceAllOccurences(ret, "\\", "/");
#elif _WIN32
  // Check to see if path is a windows path.
  if (path.find("\\") == std::string::npos) {
    ret = replacePrefix(path, linuxPrefix(), windowsPrefix());
    ret = replaceAllOccurences(ret, "/", "\\");
  } else {
    ret = replacePrefix(path, catalogPrefix(), windowsPrefix());
  }
#endif
  return ret;
}

/**
 * Replace the content of a string using regex.
 * @param path   :: An string to search and replace on.
 * @param regex  :: The regex to search for.
 * @param prefix :: Replace result of regex with this prefix.
 * @return A string containing the replacement.
 */
std::string ICatalogInfo::replacePrefix(const std::string &path,
                                        const std::string &regex,
                                        const std::string &prefix) const {
  boost::regex re(regex);
  // Assign the result of the replacement back to path and return it.
  return boost::regex_replace(path, re, prefix);
}

/**
 * Replace all occurrences of the search string in the input with the format
 * string.
 * @param path    :: An string to search and replace on.
 * @param search  :: A substring to be searched for.
 * @param format  :: A substitute string.
 * @return A string containing the replacement.
 */
std::string
ICatalogInfo::replaceAllOccurences(const std::string &path,
                                   const std::string &search,
                                   const std::string &format) const {

  return boost::replace_all_copy(path, search, format);
}

} // namespace Kernel
} // namespace Mantid
