// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/MultiFileValidator.h"
#include "MantidKernel/Logger.h"
#include <memory>

namespace Mantid::Kernel {
namespace {
// static logger
Logger g_log("MultiFileValidator");
} // namespace

/// Default constructor.
MultiFileValidator::MultiFileValidator()
    : TypedValidator<std::vector<std::vector<std::string>>>(), m_fileValidator(std::vector<std::string>(), true) {}

/** Copy constructor
 *  @param mfv :: The object with which to construct this object.
 */
MultiFileValidator::MultiFileValidator(const MultiFileValidator &mfv)
    : TypedValidator<std::vector<std::vector<std::string>>>(), m_fileValidator(mfv.m_fileValidator) {}

void swap(MultiFileValidator &obj1, MultiFileValidator &obj2) {
  using std::swap;
  swap(obj1.m_fileValidator, obj2.m_fileValidator);
}

MultiFileValidator &MultiFileValidator::operator=(MultiFileValidator tmp) {
  swap(*this, tmp);
  return *this;
}

/** Constructor
 *  @param extensions :: The permitted file extensions (e.g. .RAW)
 *  @param testFilesExist :: If to check if files exist
 */
MultiFileValidator::MultiFileValidator(const std::vector<std::string> &extensions, bool testFilesExist)
    : TypedValidator<std::vector<std::vector<std::string>>>(), m_fileValidator(extensions, testFilesExist) {}

/// Returns the set of valid values
std::vector<std::string> MultiFileValidator::allowedValues() const { return m_fileValidator.allowedValues(); }

/**
 * Clone the validator
 * @returns A pointer to a new validator with the same properties as this one
 */
IValidator_sptr MultiFileValidator::clone() const { return std::make_shared<MultiFileValidator>(*this); }

/** Checks that the files exist. The filenames of any files that dont exist
 *  are returned in an error message, else the message is "".
 *
 *  @param values :: a vector of vectors of file names
 *  @returns An error message to display to users or an empty string on no error
 */
std::string MultiFileValidator::checkValidity(const std::vector<std::vector<std::string>> &values) const {
  if (values.empty())
    return m_fileValidator.isValid("");

  std::string accumulatedErrors;

  for (const auto &row : values) {
    for (const auto &valueIt : row) {
      // For each filename value, check its validity, and and accumulate any
      // errors.
      std::string error = m_fileValidator.isValid(valueIt);
      if (!error.empty()) {
        if (accumulatedErrors.empty())
          accumulatedErrors = "Could not validate the following file(s): " + valueIt;
        else
          accumulatedErrors.append(", ").append(valueIt);
      }
    }
  }

  return accumulatedErrors;
}

} // namespace Mantid::Kernel
