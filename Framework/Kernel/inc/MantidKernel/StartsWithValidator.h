// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_STARTSWITHVALIDATOR_H_
#define MANTID_KERNEL_STARTSWITHVALIDATOR_H_

#include "MantidKernel/IValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/System.h"
#include <set>
#include <string>
#include <vector>

namespace Mantid {
namespace Kernel {
/** StartsWithValidator is a validator that requires the value of a property to
   start with one
    of the strings in a defined list of possibilities.
*/
class DLLExport StartsWithValidator : public Kernel::StringListValidator {
public:
  StartsWithValidator() = default;
  StartsWithValidator(const std::vector<std::string> &values);
  StartsWithValidator(const std::set<std::string> &values);
  IValidator_sptr clone() const override;

  /**
   * Constructor
   * @param values :: An array with the allowed values
   */
  template <std::size_t SIZE>
  StartsWithValidator(const std::array<std::string, SIZE> &values)
      : Kernel::StringListValidator(values) {}

protected:
  std::string checkValidity(const std::string &value) const override;
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_STARTSWITHVALIDATOR_H_*/
