// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IndirectFittingModel.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL IqtFitModel : public IndirectFittingModel {
public:
  IqtFitModel();
  void setFitFunction(Mantid::API::MultiDomainFunction_sptr function) override;

private:
  Mantid::API::IAlgorithm_sptr sequentialFitAlgorithm() const override;
  Mantid::API::IAlgorithm_sptr simultaneousFitAlgorithm() const override;
  std::unordered_map<std::string, ParameterValue> createDefaultParameters(TableDatasetIndex index) const override;

  bool m_constrainIntensities;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
