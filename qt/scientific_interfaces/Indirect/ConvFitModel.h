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

using ResolutionCollectionType = IndexCollectionType<TableDatasetIndex, std::weak_ptr<Mantid::API::MatrixWorkspace>>;
using ExtendedResolutionType = IndexCollectionType<TableDatasetIndex, std::string>;

class MANTIDQT_INDIRECT_DLL ConvFitModel : public IndirectFittingModel {
public:
  ConvFitModel();
  ~ConvFitModel() override;

  Mantid::API::MultiDomainFunction_sptr getFittingFunction() const override;
  boost::optional<double> getInstrumentResolution(TableDatasetIndex dataIndex) const;
  std::size_t getNumberHistograms(TableDatasetIndex index) const;

  void setFitFunction(Mantid::API::MultiDomainFunction_sptr function) override;
  void setTemperature(const boost::optional<double> &temperature);
  void removeWorkspace(TableDatasetIndex index) override;
  void setResolution(const std::string &name, TableDatasetIndex index);

  void addOutput(Mantid::API::IAlgorithm_sptr fitAlgorithm) override;

  std::vector<std::pair<std::string, size_t>> getResolutionsForFit() const override;

private:
  Mantid::API::IAlgorithm_sptr sequentialFitAlgorithm() const override;
  Mantid::API::IAlgorithm_sptr simultaneousFitAlgorithm() const override;
  Mantid::API::MultiDomainFunction_sptr getMultiDomainFunction() const override;
  std::unordered_map<std::string, ParameterValue> createDefaultParameters(TableDatasetIndex index) const override;
  std::unordered_map<std::string, std::string> mapDefaultParameterNames() const override;
  void addSampleLogs();

  void setParameterNameChanges(const Mantid::API::IFunction &model, boost::optional<std::size_t> backgroundIndex);

  ResolutionCollectionType m_resolution;
  ExtendedResolutionType m_extendedResolution;
  std::unordered_map<std::string, std::string> m_parameterNameChanges;
  boost::optional<double> m_temperature;
  boost::optional<std::size_t> m_backgroundIndex;
  std::string m_backgroundString;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
