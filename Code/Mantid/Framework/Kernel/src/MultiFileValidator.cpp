#include "MantidKernel/MultiFileValidator.h"

namespace Mantid {
namespace Kernel {
namespace {
// static logger
Logger g_log("MultiFileValidator");
}

/// Default constructor.
MultiFileValidator::MultiFileValidator()
    : TypedValidator<std::vector<std::vector<std::string>>>(),
      m_fileValidator(std::vector<std::string>(), true) {}

/** Copy constructor
 *  @param mfv :: The object with which to construct this object.
 */
MultiFileValidator::MultiFileValidator(const MultiFileValidator &mfv)
    : TypedValidator<std::vector<std::vector<std::string>>>(),
      m_fileValidator(mfv.m_fileValidator) {}

/** Constructor
 *  @param extensions :: The permitted file extensions (e.g. .RAW)
 */
MultiFileValidator::MultiFileValidator(
    const std::vector<std::string> &extensions)
    : TypedValidator<std::vector<std::vector<std::string>>>(),
      m_fileValidator(extensions, true) {}

/// Destructor
MultiFileValidator::~MultiFileValidator() {}

/// Returns the set of valid values
std::vector<std::string> MultiFileValidator::allowedValues() const {
  return m_fileValidator.allowedValues();
}

/**
 * Clone the validator
 * @returns A pointer to a new validator with the same properties as this one
 */
IValidator_sptr MultiFileValidator::clone() const {
  return boost::make_shared<MultiFileValidator>(*this);
}

/** Checks that the files exist. The filenames of any files that dont exist
 *  are returned in an error message, else the message is "".
 *
 *  @param values :: a vector of vectors of file names
 *  @returns An error message to display to users or an empty string on no error
 */
std::string MultiFileValidator::checkValidity(
    const std::vector<std::vector<std::string>> &values) const {
  if (values.empty())
    return m_fileValidator.isValid("");

  std::string accumulatedErrors("");

  typedef std::vector<std::vector<std::string>>::const_iterator
      VecVecString_cIt;
  typedef std::vector<std::string>::const_iterator VecString_cIt;

  for (VecVecString_cIt rowIt = values.begin(); rowIt != values.end();
       ++rowIt) {
    std::vector<std::string> row = (*rowIt);
    for (VecString_cIt valueIt = row.begin(); valueIt != row.end(); ++valueIt) {
      // For each filename value, check its validity, and and accumulate any
      // errors.
      std::string error = m_fileValidator.isValid(*valueIt);
      if (!error.empty()) {
        if (accumulatedErrors.empty())
          accumulatedErrors =
              "Could not validate the following file(s): " + (*valueIt);
        else
          accumulatedErrors = accumulatedErrors + ", " + (*valueIt);
      }
    }
  }

  return accumulatedErrors;
}

} // namespace Kernel
} // namespace Mantid
