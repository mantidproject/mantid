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
  SpinStateValidator(std::unordered_set<int> allowedNumbersOfSpins, const bool acceptSingleStates = false);
  Kernel::IValidator_sptr clone() const override;

  static const std::string ZERO_ONE;
  static const std::string ONE_ZERO;
  static const std::string ZERO_ZERO;
  static const std::string ONE_ONE;
  static const std::string ZERO;
  static const std::string ONE;

  static bool anyOfIsInSet(const std::vector<std::string> &anyOf, const std::unordered_set<std::string> &set);
  static bool setContains(const std::unordered_set<std::string> &set, const std::string &s) {
    return set.find(s) != set.cend();
  }

private:
  std::string checkValidity(const std::string &input) const override;
  std::unordered_set<int> m_allowedNumbersOfSpins = {1, 2, 3, 4};
  bool m_acceptSingleStates = false;
};
} // namespace Mantid::Algorithms