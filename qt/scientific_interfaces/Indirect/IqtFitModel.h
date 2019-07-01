// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_IQTFITMODEL_H_
#define MANTIDQTCUSTOMINTERFACESIDA_IQTFITMODEL_H_

#include "IndirectFittingModel.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class DLLExport IqtFitModel : public IndirectFittingModel {
public:
  IqtFitModel();

  Mantid::API::IAlgorithm_sptr getFittingAlgorithm() const override;
  std::vector<std::string> getSpectrumDependentAttributes() const override;
  void setFitTypeString(const std::string &fitType);
  void setFitFunction(Mantid::API::MultiDomainFunction_sptr function) override;

private:
  Mantid::API::MultiDomainFunction_sptr getMultiDomainFunction() const override;
  Mantid::API::IAlgorithm_sptr sequentialFitAlgorithm() const override;
  Mantid::API::IAlgorithm_sptr simultaneousFitAlgorithm() const override;
  std::string sequentialFitOutputName() const override;
  std::string simultaneousFitOutputName() const override;
  std::string singleFitOutputName(DatasetIndex index,
    WorkspaceIndex spectrum) const override;
  std::unordered_map<std::string, ParameterValue>
  createDefaultParameters(DatasetIndex index) const override;
  Mantid::API::MultiDomainFunction_sptr
  createFunctionWithGlobalBeta(Mantid::API::IFunction_sptr function) const;

  bool m_makeBetaGlobal;
  bool m_constrainIntensities;
  std::string m_fitType;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
