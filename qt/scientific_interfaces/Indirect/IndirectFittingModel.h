#ifndef MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITTINGMODEL_H_
#define MANTIDQTCUSTOMINTERFACESIDA_INDIRECTFITTINGMODEL_H_

#include "IndirectFitData.h"
#include "IndirectFitOutput.h"

#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IFunction.h"

#include <boost/optional.hpp>
#include <boost/variant.hpp>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

enum class FittingMode { SEQUENTIAL, SIMULTANEOUS };

class DLLExport IndirectFittingModel {
public:
  IndirectFittingModel();
  virtual ~IndirectFittingModel() = default;

  Mantid::API::MatrixWorkspace_sptr getWorkspace(std::size_t index) const;
  const Spectra &getSpectra(std::size_t index) const;
  std::string getExcludeRegion(std::size_t index) const;
  std::vector<std::string>
  inputDisplayNames(const std::string &formatString,
                    const std::string &rangeDelimiter) const;
  std::string inputDisplayName(const std::string &formatString,
                               const std::string &rangeDelimiter,
                               std::size_t dataIndex) const;
  bool isMultiFit() const;
  bool isPreviousFitSelected() const;
  std::size_t numberOfSpectra() const;
  std::vector<std::string> getFitParameterNames() const;
  Mantid::API::IFunction_sptr getFittingFunction() const;

  virtual void setSpectra(const Spectra &spectra, std::size_t dataIndex);
  void setExcludeRegion(const std::string &exclude, std::size_t dataIndex,
                        std::size_t index);

  void addWorkspace(const std::string &workspaceName);
  virtual void addWorkspace(Mantid::API::MatrixWorkspace_sptr workspace,
                            const Spectra &spectra);
  virtual void removeWorkspace(std::size_t index);
  void clearWorkspaces();
  void setFittingMode(FittingMode mode);
  virtual void setFitFunction(Mantid::API::IFunction_sptr model,
                              Mantid::API::IFunction_sptr background);
  virtual void setFitFunction(Mantid::API::IFunction_sptr function);
  void setDefaultParameterValue(const std::string &name, double value,
                                std::size_t dataIndex);
  void addOutput(const std::string &outputBaseName);
  void addOutput(Mantid::API::WorkspaceGroup_sptr resultGroup,
                 Mantid::API::ITableWorkspace_sptr parameterTable,
                 Mantid::API::MatrixWorkspace_sptr resultWorkspace);

  template <typename F> void applySpectra(std::size_t index, const F &functor) {
    m_fittingData[index]->applySpectra(functor);
  }

  FittingMode fittingMode() const;
  std::unordered_map<std::string, ParameterValue>
  getParameterValues(std::size_t dataIndex, std::size_t spectrum) const;
  std::unordered_map<std::string, ParameterValue>
  getFitParameters(std::size_t dataIndex, std::size_t spectrum) const;
  virtual std::unordered_map<std::string, ParameterValue>
  getDefaultParameters(std::size_t dataIndex) const;
  boost::optional<ResultLocation> getResultLocation(std::size_t dataIndex,
                                                    std::size_t spectrum) const;
  Mantid::API::MatrixWorkspace_sptr getResultWorkspace() const;
  Mantid::API::WorkspaceGroup_sptr getResultGroup() const;
  Mantid::API::IAlgorithm_sptr getFittingAlgorithm() const;
  Mantid::API::IAlgorithm_sptr
  getSingleFitAlgorithm(std::size_t dataIndex, std::size_t spectrum) const;

  void saveResult() const;

protected:
  std::size_t numberOfWorkspaces() const;
  Mantid::API::IAlgorithm_sptr
  createSequentialFit(Mantid::API::IFunction_sptr function) const;
  Mantid::API::IAlgorithm_sptr
  createSimultaneousFit(Mantid::API::IFunction_sptr function) const;

private:
  Mantid::API::IAlgorithm_sptr
  createSequentialFit(Mantid::API::IFunction_sptr function,
                      const std::string &input,
                      IndirectFitData *initialFitData) const;
  virtual Mantid::API::IAlgorithm_sptr sequentialFitAlgorithm() const = 0;
  virtual Mantid::API::IAlgorithm_sptr simultaneousFitAlgorithm() const = 0;
  virtual std::string sequentialFitOutputName() const = 0;
  virtual std::string simultaneousFitOutputName() const = 0;

  bool isPreviousModelSelected() const;

  IndirectFitOutput
  createFitOutput(Mantid::API::WorkspaceGroup_sptr resultGroup,
                  Mantid::API::ITableWorkspace_sptr parameterTable,
                  Mantid::API::MatrixWorkspace_sptr resultWorkspace) const;

  virtual IndirectFitOutput createFitOutput(
      Mantid::API::WorkspaceGroup_sptr resultGroup,
      Mantid::API::ITableWorkspace_sptr parameterTable,
      Mantid::API::MatrixWorkspace_sptr resultWorkspace,
      const std::vector<std::unique_ptr<IndirectFitData>> &fittingData) const;
  virtual void addOutput(
      IndirectFitOutput *fitOutput,
      Mantid::API::WorkspaceGroup_sptr resultGroup,
      Mantid::API::ITableWorkspace_sptr parameterTable,
      Mantid::API::MatrixWorkspace_sptr resultWorkspace,
      const std::vector<std::unique_ptr<IndirectFitData>> &fittingData) const;

  std::unique_ptr<IndirectFitOutput> m_fitOutput;
  std::vector<std::unique_ptr<IndirectFitData>> m_fittingData;
  Mantid::API::IFunction_sptr m_activeFunction;
  Mantid::API::IFunction_sptr m_fitFunction;
  std::vector<std::unordered_map<std::string, ParameterValue>>
      m_defaultParameters;
  bool m_previousModelSelected;
  FittingMode m_fittingMode;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif
