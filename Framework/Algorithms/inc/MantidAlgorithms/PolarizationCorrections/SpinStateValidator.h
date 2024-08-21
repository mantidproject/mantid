// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidKernel/IValidator.h"
#include "MantidKernel/TypedValidator.h"
#include <unordered_set>

namespace Mantid::Algorithms {

/*
Will check that a string matches the form 01,00 or 00,10,11,01, for example. This is used for
specifying the order of input workspaces relative to spin states. There is also a method to
extract the relevant workspace for a given spin state from a group workspace, given a
particular ordering.
*/
class MANTID_ALGORITHMS_DLL SpinStateValidator : public Kernel::TypedValidator<std::string> {
public:
  SpinStateValidator(std::unordered_set<int> allowedNumbersOfSpins, const bool acceptSingleStates = false,
                     const char paraIndicator = '0', const char antiIndicator = '1', const bool optional = false);
  Kernel::IValidator_sptr clone() const override;

  static bool anyOfIsInSet(const std::vector<std::string> &anyOf, const std::unordered_set<std::string> &set);
  static bool setContains(const std::unordered_set<std::string> &set, const std::string &s) {
    return set.find(s) != set.cend();
  }

private:
  std::string checkValidity(const std::string &input) const override;
  std::unordered_set<int> m_allowedNumbersOfSpins = {1, 2, 3, 4};
  const std::unordered_set<std::string> getAllowedPairStates() const;
  const std::unordered_set<std::string> getAllowedSingleStates() const;
  bool m_acceptSingleStates = false;
  const std::string m_para;
  const std::string m_anti;
  bool m_optional = false;
};
} // namespace Mantid::Algorithms
