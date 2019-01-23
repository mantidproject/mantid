// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_DirectoryValidator_H_
#define MANTID_KERNEL_DirectoryValidator_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "IValidator.h"
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/FileValidator.h"
#include <string>
#include <vector>

namespace Mantid {
namespace Kernel {

/** DirectoryValidator is a validator that checks that a directory path is
   valid.

    @author Janik Zikovsky, SNS
    @date Nov 12, 2010
*/
class MANTID_KERNEL_DLL DirectoryValidator : public FileValidator {
public:
  explicit DirectoryValidator(bool testDirectoryExists = true);
  std::vector<std::string> allowedValues() const override;
  IValidator_sptr clone() const override;

private:
  std::string checkValidity(const std::string &value) const override;
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_DirectoryValidator_H_*/
