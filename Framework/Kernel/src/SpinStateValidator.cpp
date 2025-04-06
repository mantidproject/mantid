// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidKernel/SpinStateValidator.h"

#include <MantidKernel/VectorHelper.h>
#include <boost/algorithm/string.hpp>

namespace Mantid::Kernel {

SpinStateValidator::SpinStateValidator(std::unordered_set<int> allowedNumbersOfSpins, const bool acceptSingleStates,
                                       const char paraIndicator, const char antiIndicator, const bool optional)
    : TypedValidator<std::string>(), m_allowedNumbersOfSpins(std::move(allowedNumbersOfSpins)),
      m_acceptSingleStates(acceptSingleStates), m_para(std::string(1, paraIndicator)),
      m_anti(std::string(1, antiIndicator)), m_optional(optional) {}

Kernel::IValidator_sptr SpinStateValidator::clone() const {
  return std::make_shared<SpinStateValidator>(m_allowedNumbersOfSpins, m_acceptSingleStates);
}

std::string SpinStateValidator::checkValidity(const std::string &input) const {
  if (input.empty()) {
    if (m_optional) {
      return "";
    }
    return "Enter a spin state string, it should be a comma-separated list, e.g. " + m_para + m_anti + ", " + m_anti +
           m_anti + ", " + m_anti + m_para + ", " + m_para + m_para + ".";
  }
  const auto &allowedPairs = getAllowedPairStates();
  const auto &allowedSingles = getAllowedSingleStates();

  std::vector<std::string> spinStates = VectorHelper::splitStringIntoVector<std::string>(input);

  int numberSpinStates = static_cast<int>(spinStates.size());
  if (m_allowedNumbersOfSpins.find(numberSpinStates) == m_allowedNumbersOfSpins.cend())
    return "The number of spin states specified is not an allowed value";

  // First check that the spin states are valid entries
  if (std::any_of(spinStates.cbegin(), spinStates.cend(), [&](const std::string &s) {
        const bool isPair = setContains(allowedPairs, s);
        const bool isSingle = m_acceptSingleStates && setContains(allowedSingles, s);
        return !isPair && !isSingle;
      })) {
    return m_acceptSingleStates
               ? "The spin states must either be one or two characters, with each being either a " + m_para + " or " +
                     m_anti + "."
               : "The spin states must consist of two characters, either a " + m_para + " or a " + m_anti + ".";
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

const std::unordered_set<std::string> SpinStateValidator::getAllowedPairStates() const {
  return {m_para + m_para, m_para + m_anti, m_anti + m_para, m_anti + m_anti};
}

const std::unordered_set<std::string> SpinStateValidator::getAllowedSingleStates() const { return {m_para, m_anti}; }

} // namespace Mantid::Kernel
