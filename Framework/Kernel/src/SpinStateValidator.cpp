// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidKernel/SpinStateValidator.h"

#include "MantidKernel/StringTokenizer.h"
#include <boost/algorithm/string.hpp>
#include <sstream>

namespace Mantid::Kernel {

SpinStateValidator::SpinStateValidator(std::unordered_set<int> allowedNumbersOfSpins, const bool acceptSingleStates,
                                       const std::string &paraIndicator, const std::string &antiIndicator,
                                       const bool optional, const std::string &extraIndicator)
    : TypedValidator<std::string>(), m_allowedNumbersOfSpins(std::move(allowedNumbersOfSpins)),
      m_acceptSingleStates(acceptSingleStates), m_para(paraIndicator), m_anti(antiIndicator), m_optional(optional),
      m_extra(extraIndicator) {}

Kernel::IValidator_sptr SpinStateValidator::clone() const {
  return std::make_shared<SpinStateValidator>(m_allowedNumbersOfSpins, m_acceptSingleStates);
}

std::string SpinStateValidator::checkValidity(const std::string &input) const {
  if (input.empty()) {
    if (m_optional) {
      return "";
    }
    std::ostringstream msg;
    msg << "Enter a spin state string, it should be a comma-separated list, e.g. ";
    msg << m_para << m_anti << "," << m_para << m_para << "," << m_anti << m_para << ',' << m_anti << m_anti << ".";
    return msg.str();
  }

  const auto &allowedPairs = getAllowedPairStates();
  const auto &allowedSingles = getAllowedSingleStates();

  auto spinStates = StringTokenizer{input, ",", StringTokenizer::TOK_TRIM}.asVector();

  int numberSpinStates = static_cast<int>(spinStates.size());
  if (m_allowedNumbersOfSpins.find(numberSpinStates) == m_allowedNumbersOfSpins.cend())
    return "The number of spin states specified is not an allowed value";

  // First check that the spin states are valid entries
  if (std::any_of(spinStates.cbegin(), spinStates.cend(), [&](const std::string &spinState) {
        const bool isPair = setContains(allowedPairs, spinState);
        const bool isSingle = m_acceptSingleStates && setContains(allowedSingles, spinState);
        return !isPair && !isSingle;
      })) {
    std::ostringstream msg;
    msg << "The format for the spin states is invalid, every comma separated value should contain ";
    msg << (m_acceptSingleStates ? "either one or two spin states " : "two spin states ") << "from the set " << m_para
        << "," << m_anti << (!m_extra.empty() ? "," + m_extra : "") << ".";

    return msg.str();
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
  return std::any_of(anyOf.cbegin(), anyOf.cend(),
                     [&set](const std::string &stringPair) { return setContains(set, stringPair); });
}

const std::unordered_set<std::string> SpinStateValidator::getAllowedPairStates() const {
  std::unordered_set<std::string> allowedPairs = {m_para + m_para, m_para + m_anti, m_anti + m_para, m_anti + m_anti};
  if (!m_extra.empty()) {
    allowedPairs.insert(
        {m_extra + m_para, m_extra + m_anti, m_para + m_extra, m_extra + m_extra, m_para + m_extra, m_anti + m_extra});
  }
  return allowedPairs;
}

const std::unordered_set<std::string> SpinStateValidator::getAllowedSingleStates() const {
  std::unordered_set<std::string> allowedSinglePairs = {m_para, m_anti};
  if (!m_extra.empty()) {
    allowedSinglePairs.emplace(m_extra);
  }
  return allowedSinglePairs;
}

} // namespace Mantid::Kernel
