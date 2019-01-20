// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITTINGMODEL_H_
#define MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITTINGMODEL_H_

#include "IndirectFitData.h"
#include "IndirectFitOutput.h"

#include "DllConfig.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IAlgorithm.h"

#include <boost/optional.hpp>
#include <boost/variant.hpp>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

enum class FittingMode { SEQUENTIAL, SIMULTANEOUS };

class IndirectFittingModel;

struct PrivateFittingData {
  friend class IndirectFittingModel;

public:
  PrivateFittingData();
  PrivateFittingData &operator=(PrivateFittingData &&fittingData);

private:
  PrivateFittingData(PrivateFittingData &&privateData);
  PrivateFittingData(std::vector<std::unique_ptr<IndirectFitData>> &&data);
  std::vector<std::unique_ptr<IndirectFitData>> m_data;
};

/*
    IndirectFittingModel - Provides methods for specifying and
    performing a QENS fit, as well as accessing the results of the fit.
*/
class MANTIDQT_INDIRECT_DLL IndirectFittingModel {
public:
  IndirectFittingModel();
  virtual ~IndirectFittingModel() = default;

  virtual Mantid::API::MatrixWorkspace_sptr
  getWorkspace(std::size_t index) const;
  Spectra getSpectra(std::size_t index) const;
  virtual std::pair<double, double> getFittingRange(std::size_t dataIndex,
                                                    std::size_t spectrum) const;
  virtual std::string getExcludeRegion(std::size_t dataIndex,
                                       std::size_t index) const;
  virtual std::string createDisplayName(const std::string &formatString,
                                        const std::string &rangeDelimiter,
                                        std::size_t dataIndex) const;
  std::string createOutputName(const std::string &formatString,
                               const std::string &rangeDelimiter,
                               std::size_t dataIndex) const;
  virtual bool isMultiFit() const;
  bool isPreviouslyFit(std::size_t dataIndex, std::size_t spectrum) const;
  bool hasZeroSpectra(std::size_t dataIndex) const;
  virtual boost::optional<std::string> isInvalidFunction() const;
  virtual std::size_t numberOfWorkspaces() const;
  std::size_t getNumberOfSpectra(std::size_t index) const;
  std::vector<std::string> getFitParameterNames() const;
  virtual Mantid::API::IFunction_sptr getFittingFunction() const;

  virtual std::vector<std::string> getSpectrumDependentAttributes() const = 0;

  void setFittingData(PrivateFittingData &&fittingData);
  void setSpectra(const std::string &spectra, std::size_t dataIndex);
  void setSpectra(Spectra &&spectra, std::size_t dataIndex);
  void setSpectra(const Spectra &spectra, std::size_t dataIndex);
  virtual void setStartX(double startX, std::size_t dataIndex,
                         std::size_t spectrum);
  virtual void setEndX(double endX, std::size_t dataIndex,
                       std::size_t spectrum);
  virtual void setExcludeRegion(const std::string &exclude,
                                std::size_t dataIndex, std::size_t spectrum);

  virtual void addWorkspace(const std::string &workspaceName);
  void addWorkspace(const std::string &workspaceName,
                    const std::string &spectra);
  void addWorkspace(const std::string &workspaceName, const Spectra &spectra);
  virtual void addWorkspace(Mantid::API::MatrixWorkspace_sptr workspace,
                            const Spectra &spectra);
  virtual void removeWorkspace(std::size_t index);
  virtual PrivateFittingData clearWorkspaces();
  void setFittingMode(FittingMode mode);
  virtual void setFitFunction(Mantid::API::IFunction_sptr function);
  virtual void setDefaultParameterValue(const std::string &name, double value,
                                        std::size_t dataIndex);
  void addSingleFitOutput(Mantid::API::IAlgorithm_sptr fitAlgorithm,
                          std::size_t index);
  virtual void addOutput(Mantid::API::IAlgorithm_sptr fitAlgorithm);

  template <typename F>
  void applySpectra(std::size_t index, const F &functor) const;

  FittingMode getFittingMode() const;
  std::unordered_map<std::string, ParameterValue>
  getParameterValues(std::size_t dataIndex, std::size_t spectrum) const;
  std::unordered_map<std::string, ParameterValue>
  getFitParameters(std::size_t dataIndex, std::size_t spectrum) const;
  std::unordered_map<std::string, ParameterValue>
  getDefaultParameters(std::size_t dataIndex) const;
  boost::optional<ResultLocation> getResultLocation(std::size_t dataIndex,
                                                    std::size_t spectrum) const;
  Mantid::API::WorkspaceGroup_sptr getResultWorkspace() const;
  Mantid::API::WorkspaceGroup_sptr getResultGroup() const;
  virtual Mantid::API::IAlgorithm_sptr getFittingAlgorithm() const;
  Mantid::API::IAlgorithm_sptr getSingleFit(std::size_t dataIndex,
                                            std::size_t spectrum) const;
  std::string getOutputBasename() const;

  void cleanFailedRun(Mantid::API::IAlgorithm_sptr fittingAlgorithm);
  void cleanFailedSingleRun(Mantid::API::IAlgorithm_sptr fittingAlgorithm,
                            std::size_t index);

protected:
  Mantid::API::IAlgorithm_sptr getFittingAlgorithm(FittingMode mode) const;
  Mantid::API::IAlgorithm_sptr
  createSequentialFit(Mantid::API::IFunction_sptr function) const;
  Mantid::API::IAlgorithm_sptr
  createSimultaneousFit(Mantid::API::IFunction_sptr function) const;
  Mantid::API::IAlgorithm_sptr createSimultaneousFitWithEqualRange(
      Mantid::API::IFunction_sptr function) const;
  virtual Mantid::API::CompositeFunction_sptr getMultiDomainFunction() const;
  virtual std::unordered_map<std::string, std::string>
  mapDefaultParameterNames() const;
  std::string createSingleFitOutputName(const std::string &formatString,
                                        std::size_t index,
                                        std::size_t spectrum) const;
  void addNewWorkspace(Mantid::API::MatrixWorkspace_sptr workspace,
                       const Spectra &spectra);
  void removeFittingData(std::size_t index);

private:
  void removeWorkspaceFromFittingData(std::size_t const &index);

  Mantid::API::IAlgorithm_sptr
  createSequentialFit(Mantid::API::IFunction_sptr function,
                      const std::string &input,
                      IndirectFitData *initialFitData) const;
  virtual Mantid::API::IAlgorithm_sptr sequentialFitAlgorithm() const;
  virtual Mantid::API::IAlgorithm_sptr simultaneousFitAlgorithm() const;
  virtual std::string sequentialFitOutputName() const = 0;
  virtual std::string simultaneousFitOutputName() const = 0;
  virtual std::string singleFitOutputName(std::size_t index,
                                          std::size_t spectrum) const = 0;
  virtual std::unordered_map<std::string, ParameterValue>
  createDefaultParameters(std::size_t index) const;

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
                  IndirectFitData *fitData, std::size_t spectrum) const;

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
                 IndirectFitData *fitData, std::size_t spectrum);

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
                         IndirectFitData *fitData, std::size_t spectrum) const;

  std::unique_ptr<IndirectFitOutput> m_fitOutput;
  std::vector<std::unique_ptr<IndirectFitData>> m_fittingData;
  Mantid::API::IFunction_sptr m_activeFunction;
  Mantid::API::IFunction_sptr m_fitFunction;
  std::vector<std::unordered_map<std::string, ParameterValue>>
      m_defaultParameters;
  bool m_previousModelSelected;
  FittingMode m_fittingMode;
};

template <typename F>
void IndirectFittingModel::applySpectra(std::size_t index,
                                        const F &functor) const {
  if (m_fittingData.size() > 0)
    m_fittingData[index]->applySpectra(functor);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
