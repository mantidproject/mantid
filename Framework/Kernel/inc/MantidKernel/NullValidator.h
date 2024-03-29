// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/IValidator.h"

namespace Mantid {
namespace Kernel {
/** @class NullValidator NullValidator.h Kernel/NullValidator.h

    NullValidator is a validator that doesn't.

    @author Nick Draper, Tessella Support Services plc
    @date 28/11/2007
*/
class MANTID_KERNEL_DLL NullValidator final : public IValidator {
public:
  IValidator_sptr clone() const override;

private:
  std::string check(const boost::any &) const override;
};

} // namespace Kernel
} // namespace Mantid
