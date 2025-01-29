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

using ResolutionCollectionType = IndexCollectionType<WorkspaceID, std::weak_ptr<Mantid::API::MatrixWorkspace>>;
using ExtendedResolutionType = IndexCollectionType<WorkspaceID, std::string>;

class MANTIDQT_INELASTIC_DLL ConvolutionModel : public FittingModel {
public:
  ConvolutionModel();
  ~ConvolutionModel() override = default;

  std::optional<double> getInstrumentResolution(WorkspaceID workspaceID) const;

  void setTemperature(const std::optional<double> &temperature);

  void addOutput(Mantid::API::IAlgorithm_sptr fitAlgorithm) override;

private:
  Mantid::API::IAlgorithm_sptr sequentialFitAlgorithm() const override;
  Mantid::API::IAlgorithm_sptr simultaneousFitAlgorithm() const override;
  Mantid::API::MultiDomainFunction_sptr getMultiDomainFunction() const override;
  std::unordered_map<std::string, ParameterValue> createDefaultParameters(WorkspaceID workspaceID) const override;
  std::unordered_map<std::string, std::string> mapDefaultParameterNames() const override;
  void addSampleLogs();

  void setParameterNameChanges(const Mantid::API::IFunction &model, std::optional<std::size_t> backgroundIndex);

  ResolutionCollectionType m_resolution;
  std::unordered_map<std::string, std::string> m_parameterNameChanges;
  std::optional<double> m_temperature;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
