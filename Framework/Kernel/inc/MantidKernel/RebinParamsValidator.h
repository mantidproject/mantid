// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_REBINPARAMSVALIDATOR_H_
#define MANTID_KERNEL_REBINPARAMSVALIDATOR_H_

#include "MantidKernel/TypedValidator.h"
#include <vector>

namespace Mantid {
namespace Kernel {
/** Validator to check the format of a vector providing the rebin
    parameters to an algorithm.

    @author Russell Taylor, Tessella plc
*/
class MANTID_KERNEL_DLL RebinParamsValidator
    : public TypedValidator<std::vector<double>> {
public:
  RebinParamsValidator(bool allowEmpty = false, bool allowRange = false);
  IValidator_sptr clone() const override;

private:
  std::string checkValidity(const std::vector<double> &value) const override;
  bool m_allowEmpty;
  bool m_allowRange;
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_REBINPARAMSVALIDATOR_H_*/
