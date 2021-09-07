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

using ResolutionCollectionType = IndexCollectionType<WorkspaceID, std::weak_ptr<Mantid::API::MatrixWorkspace>>;
using ExtendedResolutionType = IndexCollectionType<WorkspaceID, std::string>;

class MANTIDQT_INDIRECT_DLL ConvFitModel : public IndirectFittingModel {
public:
  ConvFitModel();
  ~ConvFitModel() = default;

  boost::optional<double> getInstrumentResolution(WorkspaceID workspaceID) const;

  void setTemperature(const boost::optional<double> &temperature);

  void addOutput(Mantid::API::IAlgorithm_sptr fitAlgorithm) override;

  std::vector<std::pair<std::string, size_t>> getResolutionsForFit() const override;

private:
  Mantid::API::IAlgorithm_sptr sequentialFitAlgorithm() const override;
  Mantid::API::IAlgorithm_sptr simultaneousFitAlgorithm() const override;
  Mantid::API::MultiDomainFunction_sptr getMultiDomainFunction() const override;
  std::unordered_map<std::string, ParameterValue> createDefaultParameters(WorkspaceID workspaceID) const override;
  std::unordered_map<std::string, std::string> mapDefaultParameterNames() const override;
  void addSampleLogs();

  void setParameterNameChanges(const Mantid::API::IFunction &model, boost::optional<std::size_t> backgroundIndex);

  ResolutionCollectionType m_resolution;
  std::unordered_map<std::string, std::string> m_parameterNameChanges;
  boost::optional<double> m_temperature;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
