// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/System.h"
#include "MantidMDAlgorithms/BooleanBinaryOperationMD.h"

namespace Mantid {
namespace MDAlgorithms {

/** EqualToMD : boolean operation on MDHistoWorkspaces

  @date 2011-11-08
*/
class DLLExport EqualToMD : public BooleanBinaryOperationMD {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"GreaterThanMD", "LessThanMD", "NotMD"}; }

private:
  void initExtraProperties() override;
  bool acceptScalar() const override { return true; }
  void execHistoHisto(Mantid::DataObjects::MDHistoWorkspace_sptr out,
                      Mantid::DataObjects::MDHistoWorkspace_const_sptr operand) override;
  void execHistoScalar(Mantid::DataObjects::MDHistoWorkspace_sptr out,
                       Mantid::DataObjects::WorkspaceSingleValue_const_sptr scalar) override;
};

} // namespace MDAlgorithms
} // namespace Mantid
