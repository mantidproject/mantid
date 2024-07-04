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

const std::string SpinStateValidator::MINUS_PLUS = "-+";
const std::string SpinStateValidator::PLUS_MINUS = "+-";
const std::string SpinStateValidator::MINUS_MINUS = "--";
const std::string SpinStateValidator::PLUS_PLUS = "++";
const std::string SpinStateValidator::MINUS = "-";
const std::string SpinStateValidator::PLUS = "+";

namespace SpinStateStrings {
static const std::unordered_set<std::string> ALLOWED_PAIR_FLIP_CONFIG{
    SpinStateValidator::ZERO_ZERO, SpinStateValidator::ZERO_ONE, SpinStateValidator::ONE_ZERO,
    SpinStateValidator::ONE_ONE};

static const std::unordered_set<std::string> ALLOWED_SINGLE_FLIP_CONFIG{SpinStateValidator::ZERO,
                                                                        SpinStateValidator::ONE};

static const std::unordered_set<std::string> ALLOWED_PAIR_SPIN_STATES{
    SpinStateValidator::MINUS_PLUS, SpinStateValidator::PLUS_MINUS, SpinStateValidator::MINUS_MINUS,
    SpinStateValidator::PLUS_PLUS};

static const std::unordered_set<std::string> ALLOWED_SINGLE_SPIN_STATES{SpinStateValidator::MINUS,
                                                                        SpinStateValidator::PLUS};

} // namespace SpinStateStrings

SpinStateValidator::SpinStateValidator(std::unordered_set<int> allowedNumbersOfSpins, const bool acceptSingleStates,
                                       const bool useFlipperConfig, const bool optional)
    : TypedValidator<std::string>(), m_allowedNumbersOfSpins(std::move(allowedNumbersOfSpins)),
      m_acceptSingleStates(acceptSingleStates), m_useFlipperConfig(useFlipperConfig), m_optional(optional) {}

Kernel::IValidator_sptr SpinStateValidator::clone() const {
  return std::make_shared<SpinStateValidator>(m_allowedNumbersOfSpins, m_acceptSingleStates);
}

std::string SpinStateValidator::checkValidity(const std::string &input) const {
  if (input.empty()) {
    if (m_optional) {
      return "";
    }
    return m_useFlipperConfig ? "Enter a spin state string, it should be a comma-separated list, e.g. 01, 11, 10, 00"
                              : "Enter a spin state string, it should be a comma-separated list, e.g. --, ++, -+, +-";
  }
  auto const &allowedPairs =
      m_useFlipperConfig ? SpinStateStrings::ALLOWED_PAIR_FLIP_CONFIG : SpinStateStrings::ALLOWED_PAIR_SPIN_STATES;
  auto const &allowedSingles =
      m_useFlipperConfig ? SpinStateStrings::ALLOWED_SINGLE_FLIP_CONFIG : SpinStateStrings::ALLOWED_SINGLE_SPIN_STATES;

  std::vector<std::string> spinStates = PolarizationCorrectionsHelpers::splitSpinStateString(input);

  int numberSpinStates = static_cast<int>(spinStates.size());
  if (m_allowedNumbersOfSpins.find(numberSpinStates) == m_allowedNumbersOfSpins.cend())
    return "The number of spin states specified is not an allowed value";

  // First check that the spin states are valid entries
  if (std::any_of(spinStates.cbegin(), spinStates.cend(), [&](std::string s) {
        const bool isPair = setContains(allowedPairs, s);
        const bool isSingle = m_acceptSingleStates && setContains(allowedSingles, s);
        return !isPair && !isSingle;
      })) {
    if (m_useFlipperConfig) {
      return m_acceptSingleStates
                 ? "The spin states must either be one or two digits, with each being either a zero or one"
                 : "The spin states must consist of two digits, either a zero or a one.";
    }
    return m_acceptSingleStates
               ? "The spin states must either be one or two characters, with each being either a plus (up) or minus "
                 "(down)."
               : "The spin states must consist of two characters, either a minus (down) or a plus (up).";
  }

  // Single digits can't mix with pairs
  if (m_acceptSingleStates) {
    bool containsAnySingles = SpinStateValidator::anyOfIsInSet(spinStates, allowedSingles);

    bool containsAnyPairs = SpinStateValidator::anyOfIsInSet(spinStates, allowedPairs);
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