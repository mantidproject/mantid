// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid::Algorithms {

/** FindReflectometryLines3: Finds a fractional workspace index corresponding to
 * a reflected or direct line in a line-detector workspace. */
class MANTID_ALGORITHMS_DLL FindReflectometryLines3 final : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::string category() const override;
  const std::string summary() const override;
  const std::vector<std::string> seeAlso() const override;

  static bool fitStatusIsAccepted(const std::string &fitStatus, bool acceptChangesInFunction,
                                  bool acceptChangesInParameters);

private:
  void init() override;
  std::map<std::string, std::string> validateInputs() override;
  void exec() override;

  API::MatrixWorkspace_sptr createProfile(const API::MatrixWorkspace_sptr &inputWorkspace);
};

} // namespace Mantid::Algorithms
