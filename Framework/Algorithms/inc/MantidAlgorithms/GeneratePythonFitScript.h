// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidAlgorithms/DllConfig.h"

#include <string>
#include <vector>

namespace Mantid {
namespace Algorithms {

class MANTID_ALGORITHMS_DLL GeneratePythonFitScript : public API::Algorithm {

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

  std::size_t getNumberOfDomainsInFunction(Mantid::API::IFunction_sptr const &function) const;

  std::string generateFitScript(std::string const &fittingType) const;
  std::string generateVariableSetupCode(std::string const &filename) const;
  std::string generateSimultaneousFitCode() const;
  std::string generateFunctionString() const;

  void savePythonScript(std::string const &filepath, std::string const &contents) const;
};

} // namespace Algorithms
} // namespace Mantid
