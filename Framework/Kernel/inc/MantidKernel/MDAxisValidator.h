// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace Mantid {
namespace Kernel {

/** @class MDAxisValidator MDAxisValidator.h Kernel/MDAxisValidator.h

    MDAxisValidator is a class that checks the number of MD axes match the
    number of workspace dimensions,
    refactoring out the common validation code from several MD algorithms
    into a common class.
*/
class MANTID_KERNEL_DLL MDAxisValidator {
public:
  MDAxisValidator(const std::vector<int> &axes, const size_t nDimensions, const bool checkIfEmpty);
  virtual ~MDAxisValidator() = default;
  virtual std::map<std::string, std::string> validate() const;

private:
  std::vector<int> m_axes;
  size_t m_wsDimensions;
  bool m_emptyCheck;
};

} // namespace Kernel
} // namespace Mantid
