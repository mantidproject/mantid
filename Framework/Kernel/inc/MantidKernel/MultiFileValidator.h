// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "FileValidator.h"

#include <set>
#include <string>
#include <vector>

namespace Mantid {
namespace Kernel {
/** The MultiFileValidator validates a MultiFileProperty, which contains a
   *vector of
    vectors* of filenames - the meaning of which is discussed in
   MultiFileProperty.h.

    This is essentially a wrapper around the FileValidator class; a single
   instance
    of which is called, once for each filename.
*/
class MANTID_KERNEL_DLL MultiFileValidator : public TypedValidator<std::vector<std::vector<std::string>>> {
public:
  MultiFileValidator();
  MultiFileValidator(const MultiFileValidator &mfv);
  explicit MultiFileValidator(const std::vector<std::string> &extensions, bool testFilesExist = true);

  IValidator_sptr clone() const override;

  /// Returns the set of allowed extensions.
  std::vector<std::string> allowedValues() const override;

protected:
  /// FileValidator instance used for validating multiple files.
  FileValidator m_fileValidator;

private:
  /// Returns an error if at least one of the files is not valid, else "".
  std::string checkValidity(const std::vector<std::vector<std::string>> &values) const override;
};

} // namespace Kernel
} // namespace Mantid
