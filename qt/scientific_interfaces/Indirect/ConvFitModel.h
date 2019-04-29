// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_CONVFITMODEL_H_
#define MANTIDQTCUSTOMINTERFACESIDA_CONVFITMODEL_H_

#include "IndirectFittingModel.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class DLLExport ConvFitModel : public IndirectFittingModel {
public:
  using IndirectFittingModel::addWorkspace;

  ConvFitModel();
  ~ConvFitModel() override;

  Mantid::API::IFunction_sptr getFittingFunction() const override;
  boost::optional<double> getInstrumentResolution(std::size_t dataIndex) const;
  std::size_t getNumberHistograms(std::size_t index) const;
  Mantid::API::MatrixWorkspace_sptr getResolution(std::size_t index) const;

  std::vector<std::string> getSpectrumDependentAttributes() const override;

  void setFitFunction(Mantid::API::IFunction_sptr function) override;
  void setTemperature(const boost::optional<double> &temperature);

  void addWorkspace(Mantid::API::MatrixWorkspace_sptr workspace,
                    const Spectra &spectra) override;
  void removeWorkspace(std::size_t index) override;
  void setResolution(const std::string &name, std::size_t index);
  void setResolution(Mantid::API::MatrixWorkspace_sptr resolution,
                     std::size_t index);
  void setFitTypeString(const std::string &fitType);

  void addOutput(Mantid::API::IAlgorithm_sptr fitAlgorithm) override;

private:
  Mantid::API::IAlgorithm_sptr sequentialFitAlgorithm() const override;
  Mantid::API::IAlgorithm_sptr simultaneousFitAlgorithm() const override;
  std::string sequentialFitOutputName() const override;
  std::string simultaneousFitOutputName() const override;
  std::string singleFitOutputName(std::size_t index,
                                  std::size_t spectrum) const override;
  Mantid::API::CompositeFunction_sptr getMultiDomainFunction() const override;
  std::unordered_map<std::string, ParameterValue>
  createDefaultParameters(std::size_t index) const override;
  std::unordered_map<std::string, std::string>
  mapDefaultParameterNames() const override;

  IndirectFitOutput
  createFitOutput(Mantid::API::WorkspaceGroup_sptr resultGroup,
                  Mantid::API::ITableWorkspace_sptr parameterTable,
                  Mantid::API::WorkspaceGroup_sptr resultWorkspace,
                  const FitDataIterator &fitDataBegin,
                  const FitDataIterator &fitDataEnd) const override;
  IndirectFitOutput
  createFitOutput(Mantid::API::WorkspaceGroup_sptr resultGroup,
                  Mantid::API::ITableWorkspace_sptr parameterTable,
                  Mantid::API::WorkspaceGroup_sptr resultWorkspace,
                  IndirectFitData *fitData,
                  std::size_t spectrum) const override;

  void addOutput(IndirectFitOutput *fitOutput,
                 Mantid::API::WorkspaceGroup_sptr resultGroup,
                 Mantid::API::ITableWorkspace_sptr parameterTable,
                 Mantid::API::WorkspaceGroup_sptr resultWorkspace,
                 const FitDataIterator &fitDataBegin,
                 const FitDataIterator &fitDataEnd) const override;
  void addOutput(IndirectFitOutput *fitOutput,
                 Mantid::API::WorkspaceGroup_sptr resultGroup,
                 Mantid::API::ITableWorkspace_sptr parameterTable,
                 Mantid::API::WorkspaceGroup_sptr resultWorkspace,
                 IndirectFitData *fitData, std::size_t spectrum) const override;
  void addExtendedResolution(std::size_t index);
  void addSampleLogs();

  void setParameterNameChanges(const Mantid::API::IFunction &model,
                               boost::optional<std::size_t> backgroundIndex);

  std::vector<boost::weak_ptr<Mantid::API::MatrixWorkspace>> m_resolution;
  std::vector<std::string> m_extendedResolution;
  std::unordered_map<std::string, std::string> m_parameterNameChanges;
  boost::optional<double> m_temperature;
  boost::optional<std::size_t> m_backgroundIndex;
  std::string m_backgroundString;
  std::string m_fitType;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
