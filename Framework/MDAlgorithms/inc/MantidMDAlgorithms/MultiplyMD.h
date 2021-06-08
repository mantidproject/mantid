// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidMDAlgorithms/BinaryOperationMD.h"

namespace Mantid {
namespace MDAlgorithms {

/** MultiplyMD : multiplication operation for MDWorkspaces

  @date 2011-11-07
*/
class DLLExport MultiplyMD : public BinaryOperationMD {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Multiply a MDHistoWorkspace by another one or a scalar."; }

  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"MinusMD", "PlusMD", "DivideMD", "PowerMD"}; }

private:
  /// Is the operation commutative?
  bool commutative() const override;

  /// Check the inputs and throw if the algorithm cannot be run
  void checkInputs() override;

  /// Run the algorithm with an MDEventWorkspace as output
  void execEvent() override;

  /// Run the algorithm with a MDHisotWorkspace as output and operand
  void execHistoHisto(Mantid::DataObjects::MDHistoWorkspace_sptr out,
                      Mantid::DataObjects::MDHistoWorkspace_const_sptr operand) override;

  /// Run the algorithm with a MDHisotWorkspace as output, scalar and operand
  void execHistoScalar(Mantid::DataObjects::MDHistoWorkspace_sptr out,
                       Mantid::DataObjects::WorkspaceSingleValue_const_sptr scalar) override;

  template <typename MDE, size_t nd>
  void execEventScalar(typename Mantid::DataObjects::MDEventWorkspace<MDE, nd>::sptr ws);
};

} // namespace MDAlgorithms
} // namespace Mantid
