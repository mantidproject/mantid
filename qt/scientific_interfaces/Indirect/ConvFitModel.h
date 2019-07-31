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

using ResolutionCollectionType =
    IndexCollectionType<DatasetIndex,
                        boost::weak_ptr<Mantid::API::MatrixWorkspace>>;
using ExtendedResolutionType = IndexCollectionType<DatasetIndex, std::string>;

class DLLExport ConvFitModel : public IndirectFittingModel {
public:
  using IndirectFittingModel::addWorkspace;

  ConvFitModel();
  ~ConvFitModel() override;

  Mantid::API::MultiDomainFunction_sptr getFittingFunction() const override;
  boost::optional<double> getInstrumentResolution(DatasetIndex dataIndex) const;
  std::size_t getNumberHistograms(DatasetIndex index) const;
  Mantid::API::MatrixWorkspace_sptr getResolution(DatasetIndex index) const;

  std::vector<std::string> getSpectrumDependentAttributes() const override;

  void setFitFunction(Mantid::API::MultiDomainFunction_sptr function) override;
  void setTemperature(const boost::optional<double> &temperature);

  void addWorkspace(Mantid::API::MatrixWorkspace_sptr workspace,
                    const Spectra &spectra) override;
  void removeWorkspace(DatasetIndex index) override;
  void setResolution(const std::string &name, DatasetIndex index);
  void setResolution(Mantid::API::MatrixWorkspace_sptr resolution,
                     DatasetIndex index);
  void setFitTypeString(const std::string &fitType);

  void addOutput(Mantid::API::IAlgorithm_sptr fitAlgorithm) override;

private:
  Mantid::API::IAlgorithm_sptr sequentialFitAlgorithm() const override;
  Mantid::API::IAlgorithm_sptr simultaneousFitAlgorithm() const override;
  std::string sequentialFitOutputName() const override;
  std::string simultaneousFitOutputName() const override;
  std::string singleFitOutputName(DatasetIndex index,
                                  WorkspaceIndex spectrum) const override;
  Mantid::API::MultiDomainFunction_sptr getMultiDomainFunction() const override;
  std::unordered_map<std::string, ParameterValue>
  createDefaultParameters(DatasetIndex index) const override;
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
                  WorkspaceIndex spectrum) const override;

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
                 IndirectFitData *fitData,
                 WorkspaceIndex spectrum) const override;
  void addExtendedResolution(DatasetIndex index);
  void addSampleLogs();

  void setParameterNameChanges(const Mantid::API::IFunction &model,
                               boost::optional<std::size_t> backgroundIndex);

  ResolutionCollectionType m_resolution;
  ExtendedResolutionType m_extendedResolution;
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
