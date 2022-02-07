// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"
#include "MantidMDAlgorithms/BinaryOperationMD.h"

namespace Mantid {
namespace MDAlgorithms {

/** BooleanBinaryOperationMD : base class for boolean-type operations on
  MDHistoWorkspaces

  @date 2011-11-08
*/
class DLLExport BooleanBinaryOperationMD : public BinaryOperationMD {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override;

  int version() const override;

protected:
  /// Return true if the algorithm can operate on a scalar.
  virtual bool acceptScalar() const { return true; }
  bool commutative() const override;
  void checkInputs() override;
  void execEvent() override;
  void execHistoScalar(Mantid::DataObjects::MDHistoWorkspace_sptr out,
                       Mantid::DataObjects::WorkspaceSingleValue_const_sptr scalar) override;

  /// Run the algorithm with a MDHisotWorkspace as output and operand
  void execHistoHisto(Mantid::DataObjects::MDHistoWorkspace_sptr out,
                      Mantid::DataObjects::MDHistoWorkspace_const_sptr operand) override = 0;
};

} // namespace MDAlgorithms
} // namespace Mantid
