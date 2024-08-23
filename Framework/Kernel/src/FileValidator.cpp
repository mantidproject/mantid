// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/FileValidator.h"
#include "MantidKernel/Logger.h"
#include <Poco/File.h>
#include <Poco/Path.h>
#include <boost/algorithm/string/case_conv.hpp>
#include <fstream>
#include <memory>
#include <sstream>

namespace Mantid::Kernel {

namespace {
// Initialize the static logger
Logger g_log("FileValidator");
} // namespace

/** Constructor
 *  @param extensions :: The permitted file extensions (e.g. .RAW)
 *  @param testFileExists :: Flag indicating whether to test for existence of
 * file (default: yes)
 */
FileValidator::FileValidator(const std::vector<std::string> &extensions, bool testFileExists)
    : TypedValidator<std::string>(), m_testExist(testFileExists) {
  for (const auto &extension : extensions) {
    const std::string ext = boost::to_lower_copy(extension);
    if (std::find(m_extensions.begin(), m_extensions.end(), ext) == m_extensions.end()) {
      m_extensions.emplace_back(ext);
    }
  }
}

/// Returns the set of valid values
std::vector<std::string> FileValidator::allowedValues() const { return m_extensions; }

/**
 * Clone the validator
 * @returns A pointer to a new validator with the same properties as this one
 */
IValidator_sptr FileValidator::clone() const { return std::make_shared<FileValidator>(*this); }

/** If m_fullTest=true if checks that the files exists, otherwise just that path
 * syntax looks valid
 *  @param value :: file name
 *  @returns An error message to display to users or an empty string on no error
 */
std::string FileValidator::checkValidity(const std::string &value) const {
  // Check if the path is syntactically valid
  if (!Poco::Path().tryParse(value)) {
    return "Error in path syntax: \"" + value + "\".";
  }

  // Check the extension but just issue a warning if it is not one of the
  // suggested values
  if (!(value.empty())) {
    if (!(this->endswith(value))) {
      // Dropped from warning to debug level as it was printing out on every
      // search of the archive, even when successful. re #5998
      g_log.debug() << "Unrecognised extension in file \"" << value << "\"";
      if (!this->m_extensions.empty()) {
        g_log.debug() << " [ ";
        for (const auto &extension : this->m_extensions)
          g_log.debug() << extension << " ";
        g_log.debug() << "]";
      }
      g_log.debug() << "\".\n";
    }
  }

  // create a variable for the absolute path to be used in error messages
  std::string abspath(value);
  if (!value.empty()) {
    Poco::Path path(value);
    if (path.isAbsolute())
      abspath = path.toString();
  }

  // If the file is required to exist check it is there
  if (m_testExist && (value.empty() || !Poco::File(value).exists())) {
    return "File \"" + abspath + "\" not found";
  }

  if (m_testExist && (Poco::File(value).exists())) {
    std::ifstream in;
    in.open(value.c_str());
    if (!in) {
      std::stringstream error;
      error << "Failed to open " + value + ": " << strerror(errno);
      return error.str();
    }
  }

  // Otherwise we are okay, file extensions are just a suggestion so no
  // validation on them is necessary
  return "";
}

/**
 * Confirm that the value string ends with then ending string.
 * @param value :: The string to check the ending for.
 * @param ending :: The ending the string should have.
 */
bool has_ending(const std::string &value, const std::string &ending) {
  if (ending.empty()) // always match against an empty extension
    return true;
  if (value.length() < ending.length()) // filename is not long enough
    return false;
  int result = value.compare(value.length() - ending.length(), ending.length(), ending);
  return (result == 0); // only care if it matches
}

/**
 * Checks the extension of a filename
 * @param value :: the filename to check
 * @return flag that true if the extension matches in the filename
 */
bool FileValidator::endswith(const std::string &value) const {
  if (m_extensions.empty()) // automatically match a lack of extensions
    return true;
  if ((m_extensions.size() == 1) && (m_extensions.begin()->empty()))
    return true;

  // create a lowercase copy of the filename
  std::string value_copy(value);
  std::transform(value_copy.begin(), value_copy.end(), value_copy.begin(), tolower);

  // check for the ending
  return std::any_of(m_extensions.cbegin(), m_extensions.cend(), [&](const auto extension) {
    return has_ending(value, extension) || has_ending(value_copy, extension);
  });
}

} // namespace Mantid::Kernel
