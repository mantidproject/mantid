// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/PolarizationCorrections/SpinStateValidator.h"
#include "MantidAlgorithms/PolarizationCorrections/PolarizationCorrectionsHelpers.h"
#include <boost/algorithm/string.hpp>

namespace Mantid::Algorithms {

const std::string SpinStateValidator::ZERO_ONE = "01";
const std::string SpinStateValidator::ONE_ZERO = "10";
const std::string SpinStateValidator::ZERO_ZERO = "00";
const std::string SpinStateValidator::ONE_ONE = "11";

namespace SpinStateStrings {
static const std::unordered_set<std::string> ALLOWED_SPIN_STATES{
    SpinStateValidator::ZERO_ZERO, SpinStateValidator::ZERO_ONE, SpinStateValidator::ONE_ZERO,
    SpinStateValidator::ONE_ONE};
} // namespace SpinStateStrings

SpinStateValidator::SpinStateValidator(std::unordered_set<int> allowedNumbersOfSpins)
    : TypedValidator<std::string>(), m_allowedNumbersOfSpins(std::move(allowedNumbersOfSpins)) {}

Kernel::IValidator_sptr SpinStateValidator::clone() const {
  return std::make_shared<SpinStateValidator>(m_allowedNumbersOfSpins);
}

std::string SpinStateValidator::checkValidity(const std::string &input) const {
  if (input.empty())
    return "Enter a spin state string, it should be a comma-separated list of spin states, e.g. 01, 11";

  std::vector<std::string> spinStates = PolarizationCorrectionsHelpers::SplitSpinStateString(input);

  int numberSpinStates = static_cast<int>(spinStates.size());
  if (m_allowedNumbersOfSpins.find(numberSpinStates) == m_allowedNumbersOfSpins.cend())
    return "The number of spin states specified is not an allowed value";

  if (std::any_of(spinStates.cbegin(), spinStates.cend(), [](std::string s) {
        return SpinStateStrings::ALLOWED_SPIN_STATES.find(s) == SpinStateStrings::ALLOWED_SPIN_STATES.cend();
      })) {
    return "The spin states must consist of two digits, either a zero or a one.";
  }

  // Check that each spin state only appears once
  std::sort(spinStates.begin(), spinStates.end());
  auto it = std::unique(spinStates.begin(), spinStates.end());
  auto numberOfUniqueStates = static_cast<int>(std::distance(spinStates.begin(), it));
  if (numberOfUniqueStates < numberSpinStates)
    return "Each spin state must only appear once";

  return "";
}
} // namespace Mantid::Algorithms