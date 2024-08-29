// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/DirectoryValidator.h"
#include "MantidKernel/IValidator.h"
#include <Poco/Exception.h>
#include <Poco/Path.h>
#include <filesystem>
#include <memory>

namespace Mantid::Kernel {

/** Constructor
 *  @param testDirectoryExists :: Flag indicating whether to test for existence
 * of directory (default: yes)
 */
DirectoryValidator::DirectoryValidator(bool testDirectoryExists) : FileValidator() {
  this->m_testExist = testDirectoryExists;
}

/// Returns the set of valid values
std::vector<std::string> DirectoryValidator::allowedValues() const { return std::vector<std::string>(); }

/**
 * Clone the validator
 * @returns A pointer to a new validator with the same properties as this one
 */
IValidator_sptr DirectoryValidator::clone() const { return std::make_shared<DirectoryValidator>(*this); }

/** If m_fullTest=true if checks that the files exists, otherwise just that path
 * syntax looks valid
 *  @param value :: file name
 *  @returns An error message to display to users or an empty string on no error
 */
std::string DirectoryValidator::checkValidity(const std::string &value) const {
  // Check if the path is syntactically valid
  if (!Poco::Path().tryParse(value)) {
    return "Error in path syntax: \"" + value + "\".";
  }

  // If the path is required to exist check it is there
  if (m_testExist) {
    try {
      if (value.empty() || !std::filesystem::exists(value))
        return "Directory \"" + value + "\" not found";
      if (!std::filesystem::is_directory(value))
        return "Directory \"" + value + "\" specified is actually a file";
    } catch (Poco::FileException &) {
      return "Error accessing directory \"" + value + "\"";
    } catch (const std::filesystem::filesystem_error &) {
      return "Error accessing directory \"" + value + "\"";
    }
  }

  // Otherwise we are okay, file extensions are just a suggestion so no
  // validation on them is necessary
  return "";
}

} // namespace Mantid::Kernel
