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
const std::string SpinStateValidator::ZERO = "0";
const std::string SpinStateValidator::ONE = "1";

namespace SpinStateStrings {
static const std::unordered_set<std::string> ALLOWED_PAIR_SPIN_STATES{
    SpinStateValidator::ZERO_ZERO, SpinStateValidator::ZERO_ONE, SpinStateValidator::ONE_ZERO,
    SpinStateValidator::ONE_ONE};
static const std::unordered_set<std::string> ALLOWED_SINGLE_SPIN_STATES{SpinStateValidator::ZERO,
                                                                        SpinStateValidator::ONE};
} // namespace SpinStateStrings

SpinStateValidator::SpinStateValidator(std::unordered_set<int> allowedNumbersOfSpins, const bool acceptSingleStates)
    : TypedValidator<std::string>(), m_allowedNumbersOfSpins(std::move(allowedNumbersOfSpins)),
      m_acceptSingleStates(acceptSingleStates) {}

Kernel::IValidator_sptr SpinStateValidator::clone() const {
  return std::make_shared<SpinStateValidator>(m_allowedNumbersOfSpins, m_acceptSingleStates);
}

std::string SpinStateValidator::checkValidity(const std::string &input) const {
  if (input.empty())
    return "Enter a spin state string, it should be a comma-separated list of spin states, e.g. 01, 11, 10, 00";

  std::vector<std::string> spinStates = PolarizationCorrectionsHelpers::splitSpinStateString(input);

  int numberSpinStates = static_cast<int>(spinStates.size());
  if (m_allowedNumbersOfSpins.find(numberSpinStates) == m_allowedNumbersOfSpins.cend())
    return "The number of spin states specified is not an allowed value";

  // First check that the spin states are valid entries
  if (std::any_of(spinStates.cbegin(), spinStates.cend(), [this](std::string s) {
        const bool isPair = setContains(SpinStateStrings::ALLOWED_PAIR_SPIN_STATES, s);
        const bool isSingle = m_acceptSingleStates && setContains(SpinStateStrings::ALLOWED_SINGLE_SPIN_STATES, s);
        return !isPair && !isSingle;
      })) {
    return m_acceptSingleStates
               ? "The spin states must either be one or two digits, with each being either a zero or one"
               : "The spin states must consist of two digits, either a zero or a one.";
  }

  // Single digits can't mix with pairs
  if (m_acceptSingleStates) {
    bool containsAnySingles =
        SpinStateValidator::anyOfIsInSet(spinStates, SpinStateStrings::ALLOWED_SINGLE_SPIN_STATES);

    bool containsAnyPairs = SpinStateValidator::anyOfIsInSet(spinStates, SpinStateStrings::ALLOWED_PAIR_SPIN_STATES);
    if (!(containsAnyPairs ^ containsAnySingles)) {
      return "Single and paired spin states cannot be mixed";
    }
  }

  // Check that each spin state only appears once
  std::sort(spinStates.begin(), spinStates.end());
  auto it = std::unique(spinStates.begin(), spinStates.end());
  auto numberOfUniqueStates = static_cast<int>(std::distance(spinStates.begin(), it));
  if (numberOfUniqueStates < numberSpinStates)
    return "Each spin state must only appear once";

  return "";
}

bool SpinStateValidator::anyOfIsInSet(const std::vector<std::string> &anyOf,
                                      const std::unordered_set<std::string> &set) {
  return std::any_of(anyOf.cbegin(), anyOf.cend(), [&set](const std::string &s) { return setContains(set, s); });
}
} // namespace Mantid::Algorithms