#ifndef MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITTINGMODEL_H_
#define MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITTINGMODEL_H_

#include "IndirectFitData.h"
#include "IndirectFitOutput.h"

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
  PrivateFittingData(std::vector<std::unique_ptr<IndirectFitData>> &&data);
  std::vector<std::unique_ptr<IndirectFitData>> m_data;
};

class DLLExport IndirectFittingModel {
public:
  IndirectFittingModel();
  virtual ~IndirectFittingModel() = default;

  Mantid::API::MatrixWorkspace_sptr getWorkspace(std::size_t index) const;
  Spectra getSpectra(std::size_t index) const;
  std::pair<double, double> getFittingRange(std::size_t dataIndex,
                                            std::size_t spectrum) const;
  std::string getExcludeRegion(std::size_t dataIndex, std::size_t index) const;
  std::string createDisplayName(const std::string &formatString,
                                const std::string &rangeDelimiter,
                                std::size_t dataIndex) const;
  std::string createOutputName(const std::string &formatString,
                               const std::string &rangeDelimiter,
                               std::size_t dataIndex) const;
  bool isMultiFit() const;
  bool isPreviouslyFit(std::size_t dataIndex, std::size_t spectrum) const;
  bool hasZeroSpectra(std::size_t dataIndex) const;
  virtual boost::optional<std::string> isInvalidFunction() const;
  std::size_t numberOfWorkspaces() const;
  std::size_t getNumberOfSpectra(std::size_t index) const;
  std::vector<std::string> getFitParameterNames() const;
  virtual Mantid::API::IFunction_sptr getFittingFunction() const;

  void setFittingData(PrivateFittingData &&fittingData);
  void setSpectra(Spectra &&spectra, std::size_t dataIndex);
  void setSpectra(const Spectra &spectra, std::size_t dataIndex);
  void setStartX(double startX, std::size_t dataIndex, std::size_t spectrum);
  void setEndX(double endX, std::size_t dataIndex, std::size_t spectrum);
  void setExcludeRegion(const std::string &exclude, std::size_t dataIndex,
                        std::size_t index);

  void addWorkspace(const std::string &workspaceName);
  void addWorkspace(const std::string &workspaceName, const Spectra &spectra);
  virtual void addWorkspace(Mantid::API::MatrixWorkspace_sptr workspace,
                            const Spectra &spectra);
  virtual void removeWorkspace(std::size_t index);
  PrivateFittingData clearWorkspaces();
  void setFittingMode(FittingMode mode);
  virtual void setFitFunction(Mantid::API::IFunction_sptr function);
  void setDefaultParameterValue(const std::string &name, double value,
                                std::size_t dataIndex);
  void addSingleFitOutput(Mantid::API::IAlgorithm_sptr fitAlgorithm,
                          std::size_t index);
  virtual void addOutput(Mantid::API::IAlgorithm_sptr fitAlgorithm);

  template <typename F> void applySpectra(std::size_t index, const F &functor);

  FittingMode getFittingMode() const;
  std::unordered_map<std::string, ParameterValue>
  getParameterValues(std::size_t dataIndex, std::size_t spectrum) const;
  std::unordered_map<std::string, ParameterValue>
  getFitParameters(std::size_t dataIndex, std::size_t spectrum) const;
  std::unordered_map<std::string, ParameterValue>
  getDefaultParameters(std::size_t dataIndex) const;
  boost::optional<ResultLocation> getResultLocation(std::size_t dataIndex,
                                                    std::size_t spectrum) const;
  Mantid::API::MatrixWorkspace_sptr getResultWorkspace() const;
  Mantid::API::WorkspaceGroup_sptr getResultGroup() const;
  Mantid::API::IAlgorithm_sptr getFittingAlgorithm() const;
  Mantid::API::IAlgorithm_sptr getSingleFit(std::size_t dataIndex,
                                            std::size_t spectrum) const;

  void saveResult() const;
  void cleanFailedRun(Mantid::API::IAlgorithm_sptr fittingAlgorithm);
  void cleanFailedSingleRun(Mantid::API::IAlgorithm_sptr fittingAlgorithm,
                            std::size_t index);

protected:
  Mantid::API::IAlgorithm_sptr
  createSequentialFit(Mantid::API::IFunction_sptr function) const;
  Mantid::API::IAlgorithm_sptr
  createSimultaneousFit(Mantid::API::IFunction_sptr function) const;
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

  bool isPreviousModelSelected() const;

  virtual IndirectFitOutput
  createFitOutput(Mantid::API::WorkspaceGroup_sptr resultGroup,
                  Mantid::API::ITableWorkspace_sptr parameterTable,
                  Mantid::API::MatrixWorkspace_sptr resultWorkspace,
                  const FitDataIterator &fitDataBegin,
                  const FitDataIterator &fitDataEnd) const;
  virtual IndirectFitOutput
  createFitOutput(Mantid::API::WorkspaceGroup_sptr resultGroup,
                  Mantid::API::ITableWorkspace_sptr parameterTable,
                  Mantid::API::MatrixWorkspace_sptr resultWorkspace,
                  IndirectFitData *fitData, std::size_t spectrum) const;

  void addOutput(Mantid::API::IAlgorithm_sptr fitAlgorithm,
                 const FitDataIterator &fitDataBegin,
                 const FitDataIterator &fitDataEnd);
  void addOutput(Mantid::API::WorkspaceGroup_sptr resultGroup,
                 Mantid::API::ITableWorkspace_sptr parameterTable,
                 Mantid::API::MatrixWorkspace_sptr resultWorkspace,
                 const FitDataIterator &fitDataBegin,
                 const FitDataIterator &fitDataEnd);
  void addOutput(Mantid::API::WorkspaceGroup_sptr resultGroup,
                 Mantid::API::ITableWorkspace_sptr parameterTable,
                 Mantid::API::MatrixWorkspace_sptr resultWorkspace,
                 IndirectFitData *fitData, std::size_t spectrum);

  virtual void addOutput(IndirectFitOutput *fitOutput,
                         Mantid::API::WorkspaceGroup_sptr resultGroup,
                         Mantid::API::ITableWorkspace_sptr parameterTable,
                         Mantid::API::MatrixWorkspace_sptr resultWorkspace,
                         const FitDataIterator &fitDataBegin,
                         const FitDataIterator &fitDataEnd) const;
  virtual void addOutput(IndirectFitOutput *fitOutput,
                         Mantid::API::WorkspaceGroup_sptr resultGroup,
                         Mantid::API::ITableWorkspace_sptr parameterTable,
                         Mantid::API::MatrixWorkspace_sptr resultWorkspace,
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
void IndirectFittingModel::applySpectra(std::size_t index, const F &functor) {
  m_fittingData[index]->applySpectra(functor);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
