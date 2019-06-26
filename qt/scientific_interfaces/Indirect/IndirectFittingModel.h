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
    IndexCollectionType<DatasetIndex, std::unique_ptr<IndirectFitData>>;
using DefaultParametersType =
    IndexCollectionType<DatasetIndex,
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
  getWorkspace(DatasetIndex index) const;
  Spectra getSpectra(DatasetIndex index) const;
  virtual std::pair<double, double>
  getFittingRange(DatasetIndex dataIndex, WorkspaceIndex spectrum) const;
  virtual std::string getExcludeRegion(DatasetIndex dataIndex,
                                       WorkspaceIndex index) const;
  virtual std::string createDisplayName(const std::string &formatString,
                                        const std::string &rangeDelimiter,
                                        DatasetIndex dataIndex) const;
  std::string createOutputName(const std::string &formatString,
                               const std::string &rangeDelimiter,
                               DatasetIndex dataIndex) const;
  virtual bool isMultiFit() const;
  bool isPreviouslyFit(DatasetIndex dataIndex, WorkspaceIndex spectrum) const;
  bool hasZeroSpectra(DatasetIndex dataIndex) const;
  virtual boost::optional<std::string> isInvalidFunction() const;
  virtual DatasetIndex numberOfWorkspaces() const;
  SpectrumRowIndex getNumberOfSpectra(DatasetIndex index) const;
  SpectrumRowIndex getNumberOfDomains() const;
  SpectrumRowIndex getDomainIndex(DatasetIndex dataIndex,
                                  WorkspaceIndex spectrum) const;
  std::vector<std::string> getFitParameterNames() const;
  virtual Mantid::API::MultiDomainFunction_sptr getFittingFunction() const;

  virtual std::vector<std::string> getSpectrumDependentAttributes() const = 0;

  void setFittingData(PrivateFittingData &&fittingData);
  void setSpectra(const std::string &spectra, DatasetIndex dataIndex);
  void setSpectra(Spectra &&spectra, DatasetIndex dataIndex);
  void setSpectra(const Spectra &spectra, DatasetIndex dataIndex);
  virtual void setStartX(double startX, DatasetIndex dataIndex,
                         WorkspaceIndex spectrum);
  virtual void setStartX(double startX, DatasetIndex dataIndex);
  virtual void setEndX(double endX, DatasetIndex dataIndex,
                       WorkspaceIndex spectrum);
  virtual void setEndX(double endX, DatasetIndex dataIndex);
  virtual void setExcludeRegion(const std::string &exclude,
                                DatasetIndex dataIndex,
                                WorkspaceIndex spectrum);

  virtual void addWorkspace(const std::string &workspaceName);
  void addWorkspace(const std::string &workspaceName,
                    const std::string &spectra);
  void addWorkspace(const std::string &workspaceName, const Spectra &spectra);
  virtual void addWorkspace(Mantid::API::MatrixWorkspace_sptr workspace,
                            const Spectra &spectra);
  virtual void removeWorkspace(DatasetIndex index);
  virtual PrivateFittingData clearWorkspaces();
  void setFittingMode(FittingMode mode);
  virtual void setFitFunction(Mantid::API::MultiDomainFunction_sptr function);
  virtual void setDefaultParameterValue(const std::string &name, double value,
                                        DatasetIndex dataIndex);
  void addSingleFitOutput(Mantid::API::IAlgorithm_sptr fitAlgorithm,
                          DatasetIndex index);
  virtual void addOutput(Mantid::API::IAlgorithm_sptr fitAlgorithm);

  template <typename F>
  void applySpectra(DatasetIndex index, const F &functor) const;

  FittingMode getFittingMode() const;
  std::unordered_map<std::string, ParameterValue>
  getParameterValues(DatasetIndex dataIndex, WorkspaceIndex spectrum) const;
  std::unordered_map<std::string, ParameterValue>
  getFitParameters(DatasetIndex dataIndex, WorkspaceIndex spectrum) const;
  std::unordered_map<std::string, ParameterValue>
  getDefaultParameters(DatasetIndex dataIndex) const;
  boost::optional<ResultLocation>
  getResultLocation(DatasetIndex dataIndex, WorkspaceIndex spectrum) const;
  Mantid::API::WorkspaceGroup_sptr getResultWorkspace() const;
  Mantid::API::WorkspaceGroup_sptr getResultGroup() const;
  virtual Mantid::API::IAlgorithm_sptr getFittingAlgorithm() const;
  Mantid::API::IAlgorithm_sptr getSingleFit(DatasetIndex dataIndex,
                                            WorkspaceIndex spectrum) const;
  Mantid::API::IFunction_sptr getSingleFunction(DatasetIndex dataIndex,
                                                WorkspaceIndex spectrum) const;
  std::string getOutputBasename() const;

  void cleanFailedRun(Mantid::API::IAlgorithm_sptr fittingAlgorithm);
  void cleanFailedSingleRun(Mantid::API::IAlgorithm_sptr fittingAlgorithm,
                            DatasetIndex index);
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
                                        DatasetIndex index,
                                        WorkspaceIndex spectrum) const;
  void addNewWorkspace(Mantid::API::MatrixWorkspace_sptr workspace,
                       const Spectra &spectra);
  void removeFittingData(DatasetIndex index);

private:
  std::vector<std::string> getWorkspaceNames() const;

  void removeWorkspaceFromFittingData(DatasetIndex const &index);

  Mantid::API::IAlgorithm_sptr
  createSequentialFit(Mantid::API::IFunction_sptr function,
                      const std::string &input,
                      IndirectFitData *initialFitData) const;
  virtual Mantid::API::IAlgorithm_sptr sequentialFitAlgorithm() const;
  virtual Mantid::API::IAlgorithm_sptr simultaneousFitAlgorithm() const;
  virtual std::string sequentialFitOutputName() const = 0;
  virtual std::string simultaneousFitOutputName() const = 0;
  virtual std::string singleFitOutputName(DatasetIndex index,
                                          WorkspaceIndex spectrum) const = 0;
  virtual std::unordered_map<std::string, ParameterValue>
  createDefaultParameters(DatasetIndex index) const;

  virtual std::string getResultXAxisUnit() const;

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
void IndirectFittingModel::applySpectra(DatasetIndex index,
                                        const F &functor) const {
  if (m_fittingData.size() > m_fittingData.zero())
    m_fittingData[index]->applySpectra(functor);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
