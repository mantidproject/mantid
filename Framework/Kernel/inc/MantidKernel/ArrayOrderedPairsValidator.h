// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/IValidator.h"
#include "MantidKernel/TypedValidator.h"
#include <string>
#include <vector>

namespace Mantid {
namespace Kernel {
/** @class ArrayOrderedPairsValidator ArrayOrderedPairsValidator.h
   Kernel/ArrayOrderedPairsValidator.h

    ArrayOrderedPairsValidator validates that an array contains a sequence of
    ordered pairs of numbers.
*/
template <typename TYPE> class MANTID_KERNEL_DLL ArrayOrderedPairsValidator : public TypedValidator<std::vector<TYPE>> {
public:
  /// Clone the current state
  IValidator_sptr clone() const override;

private:
  std::string checkValidity(const std::vector<TYPE> &value) const override;
};

} // namespace Kernel
} // namespace Mantid
