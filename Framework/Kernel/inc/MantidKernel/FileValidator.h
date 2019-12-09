// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_FILEVALIDATOR_H_
#define MANTID_KERNEL_FILEVALIDATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/TypedValidator.h"
#include <vector>

namespace Mantid {
namespace Kernel {

bool has_ending(const std::string &value, const std::string &ending);

/** FileValidator is a validator that checks that a filepath is valid.

    @author Matt Clarke, ISIS.
    @date 25/06/2008
*/
class MANTID_KERNEL_DLL FileValidator : public TypedValidator<std::string> {
public:
  explicit FileValidator(
      const std::vector<std::string> &extensions = std::vector<std::string>(),
      bool testFileExists = true);
  std::vector<std::string> allowedValues() const override;
  IValidator_sptr clone() const override;

protected:
  /// The list of permitted extensions
  std::vector<std::string> m_extensions;
  /// Flag indicating whether to test for existence of filename
  bool m_testExist;

private:
  std::string checkValidity(const std::string &value) const override;
  bool endswith(const std::string &value) const;
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_FILEVALIDATOR_H_*/
