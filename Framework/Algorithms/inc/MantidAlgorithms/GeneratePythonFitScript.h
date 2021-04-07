// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/SerialAlgorithm.h"
#include "MantidAlgorithms/DllConfig.h"

#include <string>
#include <vector>

namespace Mantid {
namespace Algorithms {

class MANTID_ALGORITHMS_DLL GeneratePythonFitScript : public API::SerialAlgorithm {

public:
  std::string const name() const override;
  int version() const override;

  std::string const category() const override;
  std::string const summary() const override;

  std::vector<std::string> const seeAlso() const override;

private:
  void init() override;
  std::map<std::string, std::string> validateInputs() override;
  void exec() override;

  std::string generateVariableSetupCode(std::vector<std::string> const &inputWorkspaces,
                                        std::vector<std::size_t> const &workspaceIndices,
                                        std::vector<double> const &startXs, std::vector<double> const &endXs,
                                        Mantid::API::IFunction_const_sptr function) const;
  std::string generateLoadingWorkspacesCode() const;
  std::string generateSequentialFittingCode() const;
  std::string generateFitOptionsString() const;
  std::string generateCodeForTidyingFitOutput() const;
  std::string generateCodeForPlottingFitOutput() const;

  void savePythonScript(std::string const &contents) const;
};

} // namespace Algorithms
} // namespace Mantid
