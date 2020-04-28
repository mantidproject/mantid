// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DistributedAlgorithm.h"
#include "MantidKernel/Unit.h"
#include "MantidReflectometry/DllConfig.h"

namespace Mantid {
namespace Reflectometry {

using namespace API;

class MANTID_REFLECTOMETRY_DLL ConvertSingleSpectrumLambdaToQ
    : public DistributedAlgorithm {
public:
  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override {
    return "ConvertSingleSpectrumLambdaToQ";
  }
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Performs a unit change on a single spectrum lambda workspace";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override { return "Transforms\\Units"; }

private:
  const std::string workspaceMethodName() const override {
    return "convertSingleSpectrumLambdaToQ";
  }
  const std::string workspaceMethodInputProperty() const override {
    return "InputWorkspace";
  }

  // Overridden Algorithm methods
  void init() override;
  void exec() override;

  void setupMemberVariables(const MatrixWorkspace_const_sptr inputWs);
  /// Checks that the workspace satisfies the condition for this algorithm
  void checkSingleSpectrumLambda();
  /// Executes the main part of the algorithm that handles the unit conversion
  MatrixWorkspace_sptr
  executeUnitConversion(const MatrixWorkspace_sptr inputWs);
  /// Performs the unit conversion on a workspace in lambda to a workspace in Q
  MatrixWorkspace_sptr transform(const MatrixWorkspace_sptr inputWs);
  /// Reverses the workspace if Y and E values are in descending order
  void reverse(MatrixWorkspace_sptr inputWs);

  std::size_t m_numberOfSpectra{0};
  double m_theta{0};
  Kernel::Unit_const_sptr m_inputUnit;
  Kernel::Unit_sptr m_outputUnit;
};

} // namespace Reflectometry
} // namespace Mantid
