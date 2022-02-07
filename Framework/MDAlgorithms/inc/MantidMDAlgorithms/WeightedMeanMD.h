// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidMDAlgorithms/BinaryOperationMD.h"

namespace Mantid {
namespace MDAlgorithms {

/** WeightedMeanMD : Find the weighted mean of two MDWorkspaces

  @date 2012-06-26
*/
class DLLExport WeightedMeanMD : public BinaryOperationMD {
public:
  /// Algorithm's name for identification
  const std::string name() const override { return "WeightedMeanMD"; };
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Find weighted mean of two MDHistoWorkspaces."; }

  /// Algorithm's version for identification
  int version() const override { return 1; };
  const std::vector<std::string> seeAlso() const override { return {"WeightedMean"}; }

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
};

} // namespace MDAlgorithms
} // namespace Mantid
