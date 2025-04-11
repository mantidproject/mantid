// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAlgorithms/DllConfig.h"
#include "MantidDataObjects/TableWorkspace.h"

namespace Mantid {
namespace Algorithms {

/** CombineTableWorkspaces : Take a pair of table workspaces and combine them into a single table
 * by appending the rows of the second onto the first
 */
class MANTID_ALGORITHMS_DLL CombineTableWorkspaces final : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  std::map<std::string, std::string> validateInputs() override;
  static const std::map<std::string, int> allowedTypes();

private:
  void init() override;
  void exec() override;
};

} // namespace Algorithms
} // namespace Mantid
