// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"
#include "MantidMDAlgorithms/UnaryOperationMD.h"

namespace Mantid {
namespace MDAlgorithms {

/** PowerMD : raise a MDHistoWorkspace to a power.

  @date 2011-11-08
*/
class DLLExport PowerMD : public UnaryOperationMD {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Raise a MDHistoWorkspace to a power"; }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"MinusMD", "MultiplyMD", "DivideMD", "PlusMD", "LogarithmMD", "ExponentialMD"};
  }

private:
  void initExtraProperties() override;

  /// Check the inputs and throw if the algorithm cannot be run
  void checkInputs() override;

  /// Run the algorithm on a MDEventWorkspace
  void execEvent(Mantid::API::IMDEventWorkspace_sptr out) override;

  /// Run the algorithm with a MDHistoWorkspace
  void execHisto(Mantid::DataObjects::MDHistoWorkspace_sptr out) override;
};

} // namespace MDAlgorithms
} // namespace Mantid
