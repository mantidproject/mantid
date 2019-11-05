// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITTINGMODEL_H_
#define MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITTINGMODEL_H_

#include "IndexTypes.h"
#include "IndirectFitData.h"
#include "IndirectFitOutput.h"
#include "ParameterEstimation.h"

#include "DllConfig.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IFunction_fwd.h"

#include <boost/optional.hpp>
#include <boost/variant.hpp>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

enum class FittingMode { SEQUENTIAL, SIMULTANEOUS };

class IndirectFittingModel;

using IndirectFitDataCollectionType =
    IndexCollectionType<TableDatasetIndex, std::unique_ptr<IndirectFitData>>;
using DefaultParametersType =
    IndexCollectionType<TableDatasetIndex,
                        std::unordered_map<std::string, ParameterValue>>;

struct PrivateFittingData {
  friend class IndirectFittingModel;

public:
  PrivateFittingData();
  PrivateFittingData &operator=(PrivateFittingData &&fittingData);

private:
  PrivateFittingData(PrivateFittingData &&privateData);
  PrivateFittingData(IndirectFitDataCollectionType &&data);
  IndirectFitDataCollectionType m_data;
};

/*
    IndirectFittingModel - Provides methods for specifying and
    performing a QENS fit, as well as accessing the results of the fit.
*/
class MANTIDQT_INDIRECT_DLL IndirectFittingModel {
public:
  IndirectFittingModel();
  virtual ~IndirectFittingModel() = default;

  virtual bool hasWorkspace(std::string const &workspaceName) const;
  virtual Mantid::API::MatrixWorkspace_sptr
  getWorkspace(TableDatasetIndex index) const;
  Spectra getSpectra(TableDatasetIndex index) const;
  virtual std::pair<double, double>
  getFittingRange(TableDatasetIndex dataIndex, WorkspaceIndex spectrum) const;
  virtual std::string getExcludeRegion(TableDatasetIndex dataIndex,
                                       WorkspaceIndex index) const;
  virtual std::string createDisplayName(const std::string &formatString,
                                        const std::string &rangeDelimiter,
                                        TableDatasetIndex dataIndex) const;
  std::string createOutputName(const std::string &formatString,
                               const std::string &rangeDelimiter,
                               TableDatasetIndex dataIndex) const;
  virtual bool isMultiFit() const;
  bool isPreviouslyFit(TableDatasetIndex dataIndex,
                       WorkspaceIndex spectrum) const;
  virtual boost::optional<std::string> isInvalidFunction() const;
  virtual TableDatasetIndex numberOfWorkspaces() const;
  TableRowIndex getNumberOfSpectra(TableDatasetIndex index) const;
  TableRowIndex getNumberOfDomains() const;
  virtual TableRowIndex getDomainIndex(TableDatasetIndex dataIndex,
                                       WorkspaceIndex spectrum) const;
  std::vector<std::string> getFitParameterNames() const;
  virtual Mantid::API::MultiDomainFunction_sptr getFittingFunction() const;

  virtual std::vector<std::string> getSpectrumDependentAttributes() const = 0;

  void setFittingData(PrivateFittingData &&fittingData);
  void setSpectra(const std::string &spectra, TableDatasetIndex dataIndex);
  void setSpectra(Spectra &&spectra, TableDatasetIndex dataIndex);
  void setSpectra(const Spectra &spectra, TableDatasetIndex dataIndex);
  virtual void setStartX(double startX, TableDatasetIndex dataIndex,
                         WorkspaceIndex spectrum);
  virtual void setStartX(double startX, TableDatasetIndex dataIndex);
  virtual void setEndX(double endX, TableDatasetIndex dataIndex,
                       WorkspaceIndex spectrum);
  virtual void setEndX(double endX, TableDatasetIndex dataIndex);
  virtual void setExcludeRegion(const std::string &exclude,
                                TableDatasetIndex dataIndex,
                                WorkspaceIndex spectrum);

  virtual void addWorkspace(const std::string &workspaceName);
  void addWorkspace(const std::string &workspaceName,
                    const std::string &spectra);
  void addWorkspace(const std::string &workspaceName, const Spectra &spectra);
  virtual void addWorkspace(Mantid::API::MatrixWorkspace_sptr workspace,
                            const Spectra &spectra);
  virtual void removeWorkspace(TableDatasetIndex index);
  virtual PrivateFittingData clearWorkspaces();
  void setFittingMode(FittingMode mode);
  virtual void setFitFunction(Mantid::API::MultiDomainFunction_sptr function);
  virtual void setDefaultParameterValue(const std::string &name, double value,
                                        TableDatasetIndex dataIndex);
  void addSingleFitOutput(Mantid::API::IAlgorithm_sptr fitAlgorithm,
                          TableDatasetIndex index);
  virtual void addOutput(Mantid::API::IAlgorithm_sptr fitAlgorithm);

  template <typename F>
  void applySpectra(TableDatasetIndex index, const F &functor) const;

  FittingMode getFittingMode() const;
  std::unordered_map<std::string, ParameterValue>
  getParameterValues(TableDatasetIndex dataIndex,
                     WorkspaceIndex spectrum) const;
  std::unordered_map<std::string, ParameterValue>
  getFitParameters(TableDatasetIndex dataIndex, WorkspaceIndex spectrum) const;
  std::unordered_map<std::string, ParameterValue>
  getDefaultParameters(TableDatasetIndex dataIndex) const;
  boost::optional<ResultLocationNew>
  getResultLocation(TableDatasetIndex dataIndex, WorkspaceIndex spectrum) const;
  Mantid::API::WorkspaceGroup_sptr getResultWorkspace() const;
  Mantid::API::WorkspaceGroup_sptr getResultGroup() const;
  virtual Mantid::API::IAlgorithm_sptr getFittingAlgorithm() const;
  Mantid::API::IAlgorithm_sptr getSingleFit(TableDatasetIndex dataIndex,
                                            WorkspaceIndex spectrum) const;
  Mantid::API::IFunction_sptr getSingleFunction(TableDatasetIndex dataIndex,
                                                WorkspaceIndex spectrum) const;
  std::string getOutputBasename() const;

  void cleanFailedRun(Mantid::API::IAlgorithm_sptr fittingAlgorithm);
  void cleanFailedSingleRun(Mantid::API::IAlgorithm_sptr fittingAlgorithm,
                            TableDatasetIndex index);
  DataForParameterEstimationCollection
  getDataForParameterEstimation(EstimationDataSelector selector) const;

protected:
  Mantid::API::IAlgorithm_sptr getFittingAlgorithm(FittingMode mode) const;
  Mantid::API::IAlgorithm_sptr
  createSequentialFit(Mantid::API::IFunction_sptr function) const;
  Mantid::API::IAlgorithm_sptr
  createSimultaneousFit(Mantid::API::MultiDomainFunction_sptr function) const;
  Mantid::API::IAlgorithm_sptr createSimultaneousFitWithEqualRange(
      Mantid::API::IFunction_sptr function) const;
  virtual Mantid::API::MultiDomainFunction_sptr getMultiDomainFunction() const;
  virtual std::unordered_map<std::string, std::string>
  mapDefaultParameterNames() const;
  std::string createSingleFitOutputName(const std::string &formatString,
                                        TableDatasetIndex index,
                                        WorkspaceIndex spectrum) const;
  void addNewWorkspace(Mantid::API::MatrixWorkspace_sptr workspace,
                       const Spectra &spectra);
  void removeFittingData(TableDatasetIndex index);

private:
  std::vector<std::string> getWorkspaceNames() const;

  void removeWorkspaceFromFittingData(TableDatasetIndex const &index);

  Mantid::API::IAlgorithm_sptr
  createSequentialFit(Mantid::API::IFunction_sptr function,
                      const std::string &input,
                      IndirectFitData *initialFitData) const;
  virtual Mantid::API::IAlgorithm_sptr sequentialFitAlgorithm() const;
  virtual Mantid::API::IAlgorithm_sptr simultaneousFitAlgorithm() const;
  virtual std::string sequentialFitOutputName() const = 0;
  virtual std::string simultaneousFitOutputName() const = 0;
  virtual std::string singleFitOutputName(TableDatasetIndex index,
                                          WorkspaceIndex spectrum) const = 0;
  virtual std::unordered_map<std::string, ParameterValue>
  createDefaultParameters(TableDatasetIndex index) const;

  virtual std::string getResultXAxisUnit() const;
  virtual std::string getResultLogName() const;

  bool isPreviousModelSelected() const;

  virtual IndirectFitOutput
  createFitOutput(Mantid::API::WorkspaceGroup_sptr resultGroup,
                  Mantid::API::ITableWorkspace_sptr parameterTable,
                  Mantid::API::WorkspaceGroup_sptr resultWorkspace,
                  const FitDataIterator &fitDataBegin,
                  const FitDataIterator &fitDataEnd) const;
  virtual IndirectFitOutput
  createFitOutput(Mantid::API::WorkspaceGroup_sptr resultGroup,
                  Mantid::API::ITableWorkspace_sptr parameterTable,
                  Mantid::API::WorkspaceGroup_sptr resultWorkspace,
                  IndirectFitData *fitData, WorkspaceIndex spectrum) const;

  void addOutput(Mantid::API::IAlgorithm_sptr fitAlgorithm,
                 const FitDataIterator &fitDataBegin,
                 const FitDataIterator &fitDataEnd);
  void addOutput(Mantid::API::WorkspaceGroup_sptr resultGroup,
                 Mantid::API::ITableWorkspace_sptr parameterTable,
                 Mantid::API::WorkspaceGroup_sptr resultWorkspace,
                 const FitDataIterator &fitDataBegin,
                 const FitDataIterator &fitDataEnd);
  void addOutput(Mantid::API::WorkspaceGroup_sptr resultGroup,
                 Mantid::API::ITableWorkspace_sptr parameterTable,
                 Mantid::API::WorkspaceGroup_sptr resultWorkspace,
                 IndirectFitData *fitData, WorkspaceIndex spectrum);

  virtual void addOutput(IndirectFitOutput *fitOutput,
                         Mantid::API::WorkspaceGroup_sptr resultGroup,
                         Mantid::API::ITableWorkspace_sptr parameterTable,
                         Mantid::API::WorkspaceGroup_sptr resultWorkspace,
                         const FitDataIterator &fitDataBegin,
                         const FitDataIterator &fitDataEnd) const;
  virtual void addOutput(IndirectFitOutput *fitOutput,
                         Mantid::API::WorkspaceGroup_sptr resultGroup,
                         Mantid::API::ITableWorkspace_sptr parameterTable,
                         Mantid::API::WorkspaceGroup_sptr resultWorkspace,
                         IndirectFitData *fitData,
                         WorkspaceIndex spectrum) const;

  std::unique_ptr<IndirectFitOutput> m_fitOutput;
  IndirectFitDataCollectionType m_fittingData;
  Mantid::API::MultiDomainFunction_sptr m_activeFunction;
  Mantid::API::IFunction_sptr m_fitFunction;
  DefaultParametersType m_defaultParameters;
  bool m_previousModelSelected;
  FittingMode m_fittingMode;
};

template <typename F>
void IndirectFittingModel::applySpectra(TableDatasetIndex index,
                                        const F &functor) const {
  if (m_fittingData.size() > m_fittingData.zero())
    m_fittingData[index]->applySpectra(functor);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
