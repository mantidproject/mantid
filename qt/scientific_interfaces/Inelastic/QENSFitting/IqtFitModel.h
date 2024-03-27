// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "FittingModel.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

class MANTIDQT_INELASTIC_DLL IqtFitModel : public FittingModel {
public:
  IqtFitModel();
  void setFitFunction(Mantid::API::MultiDomainFunction_sptr function) override;

private:
  Mantid::API::IAlgorithm_sptr sequentialFitAlgorithm() const override;
  Mantid::API::IAlgorithm_sptr simultaneousFitAlgorithm() const override;
  std::unordered_map<std::string, ParameterValue> createDefaultParameters(WorkspaceID workspaceID) const override;

  bool m_constrainIntensities;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
