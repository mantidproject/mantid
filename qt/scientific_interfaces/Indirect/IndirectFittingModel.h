// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "IIndirectFitDataTableModel.h"
#include "IIndirectFitOutput.h"
#include "IIndirectFittingModel.h"
#include "IndirectFitData.h"
#include "IndirectWorkspaceNames.h"
#include "ParameterEstimation.h"

#include "DllConfig.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/FittingMode.h"
#include "MantidQtWidgets/Common/FunctionModelSpectra.h"
#include "MantidQtWidgets/Common/IndexTypes.h"

#include <boost/optional.hpp>
#include <boost/variant.hpp>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
using namespace MantidWidgets;

extern std::unordered_map<FittingMode, std::string> fitModeToName;

class IndirectFittingModel;

using DefaultParametersType = IndexCollectionType<TableDatasetIndex, std::unordered_map<std::string, ParameterValue>>;

/*
    IndirectFittingModel - Provides methods for specifying and
    performing a QENS fit, as well as accessing the results of the fit.
*/
class MANTIDQT_INDIRECT_DLL IndirectFittingModel : public IIndirectFittingModel {
public:
  IndirectFittingModel();
  virtual ~IndirectFittingModel() = default;

  // Functions that interact with IndirectFitDataTableModel
  virtual void addWorkspace(const std::string &workspaceName) override;
  void addWorkspace(const std::string &workspaceName, const std::string &spectra) override;
  void addWorkspace(const std::string &workspaceName, const FunctionModelSpectra &spectra) override;
  virtual void addWorkspace(Mantid::API::MatrixWorkspace_sptr workspace, const FunctionModelSpectra &spectra) override;
  virtual bool hasWorkspace(std::string const &workspaceName) const override;
  virtual Mantid::API::MatrixWorkspace_sptr getWorkspace(TableDatasetIndex index) const override;
  virtual void removeWorkspace(TableDatasetIndex index) override;
  void setSpectra(const std::string &spectra, TableDatasetIndex dataIndex) override;
  void setSpectra(FunctionModelSpectra &&spectra, TableDatasetIndex dataIndex) override;
  void setSpectra(const FunctionModelSpectra &spectra, TableDatasetIndex dataIndex) override;
  FunctionModelSpectra getSpectra(TableDatasetIndex index) const override;

  virtual bool isMultiFit() const override;
  virtual TableDatasetIndex getNumberOfWorkspaces() const override;
  size_t getNumberOfSpectra(TableDatasetIndex index) const override;
  size_t getNumberOfDomains() const override;
  FitDomainIndex getDomainIndex(TableDatasetIndex dataIndex, WorkspaceIndex spectrum) const override;
  std::vector<double> getQValuesForData() const override;
  virtual std::vector<std::pair<std::string, size_t>> getResolutionsForFit() const override;
  void clearWorkspaces() override;
  void clear() override;

  void setStartX(double startX, TableDatasetIndex dataIndex, WorkspaceIndex spectrum) override;
  virtual void setStartX(double startX, TableDatasetIndex dataIndex) override;
  void setEndX(double endX, TableDatasetIndex dataIndex, WorkspaceIndex spectrum) override;
  virtual void setEndX(double endX, TableDatasetIndex dataIndex) override;
  virtual std::pair<double, double> getFittingRange(TableDatasetIndex dataIndex,
                                                    WorkspaceIndex spectrum) const override;
  void setExcludeRegion(const std::string &exclude, TableDatasetIndex dataIndex, WorkspaceIndex spectrum) override;
  virtual std::string getExcludeRegion(TableDatasetIndex dataIndex, WorkspaceIndex index) const override;

  // Functions concerned with naming

  // IIndirectFittingModel
  bool isPreviouslyFit(TableDatasetIndex dataIndex, WorkspaceIndex spectrum) const override;

  virtual boost::optional<std::string> isInvalidFunction() const override;
  std::vector<std::string> getFitParameterNames() const override;
  void setFitFunction(Mantid::API::MultiDomainFunction_sptr function) override;
  virtual Mantid::API::MultiDomainFunction_sptr getFitFunction() const override;
  void setDefaultParameterValue(const std::string &name, double value, TableDatasetIndex dataIndex) override;
  std::unordered_map<std::string, ParameterValue> getParameterValues(TableDatasetIndex dataIndex,
                                                                     WorkspaceIndex spectrum) const override;
  std::unordered_map<std::string, ParameterValue> getFitParameters(TableDatasetIndex dataIndex,
                                                                   WorkspaceIndex spectrum) const override;
  std::unordered_map<std::string, ParameterValue> getDefaultParameters(TableDatasetIndex dataIndex) const override;

  // IIndirectFitOutput
  void addSingleFitOutput(const Mantid::API::IAlgorithm_sptr &fitAlgorithm, TableDatasetIndex index,
                          WorkspaceIndex spectrum) override;
  virtual void addOutput(Mantid::API::IAlgorithm_sptr fitAlgorithm) override;

  // Generic
  void setFittingMode(FittingMode mode) override;
  FittingMode getFittingMode() const override;

  void setFitTypeString(const std::string &fitType) override;
  boost::optional<ResultLocationNew> getResultLocation(TableDatasetIndex dataIndex,
                                                       WorkspaceIndex spectrum) const override;
  Mantid::API::WorkspaceGroup_sptr getResultWorkspace() const override;
  Mantid::API::WorkspaceGroup_sptr getResultGroup() const override;
  virtual Mantid::API::IAlgorithm_sptr getFittingAlgorithm() const override;
  Mantid::API::IAlgorithm_sptr getSingleFit(TableDatasetIndex dataIndex, WorkspaceIndex spectrum) const override;
  Mantid::API::IFunction_sptr getSingleFunction(TableDatasetIndex dataIndex, WorkspaceIndex spectrum) const override;
  std::string getOutputBasename() const override;
  virtual std::string createDisplayName(TableDatasetIndex dataIndex) const override;

  void cleanFailedRun(const Mantid::API::IAlgorithm_sptr &fittingAlgorithm) override;
  void cleanFailedSingleRun(const Mantid::API::IAlgorithm_sptr &fittingAlgorithm, TableDatasetIndex index) override;
  DataForParameterEstimationCollection
  getDataForParameterEstimation(const EstimationDataSelector &selector) const override;
  void removeFittingData() override;

  IIndirectFitDataTableModel *getFitDataModel() override;

protected:
  std::string createOutputName(const std::string &fitMode) const;
  Mantid::API::IAlgorithm_sptr getFittingAlgorithm(FittingMode mode) const;
  Mantid::API::IAlgorithm_sptr createSequentialFit(Mantid::API::IFunction_sptr function) const;
  Mantid::API::IAlgorithm_sptr createSimultaneousFit(const Mantid::API::MultiDomainFunction_sptr &function) const;
  virtual Mantid::API::MultiDomainFunction_sptr getMultiDomainFunction() const;
  virtual std::unordered_map<std::string, std::string> mapDefaultParameterNames() const;
  std::string m_fitType = "FitType";
  std::string m_fitString = "FitString";

  std::unique_ptr<IIndirectFitDataTableModel> m_fitDataModel;

private:
  std::vector<std::string> getWorkspaceNames() const;
  std::vector<double> getExcludeRegionVector(TableDatasetIndex dataIndex, WorkspaceIndex index) const;

  void removeWorkspaceFromFittingData(TableDatasetIndex const &index);

  Mantid::API::IAlgorithm_sptr createSequentialFit(const Mantid::API::IFunction_sptr &function,
                                                   const std::string &input) const;
  virtual Mantid::API::IAlgorithm_sptr sequentialFitAlgorithm() const;
  virtual Mantid::API::IAlgorithm_sptr simultaneousFitAlgorithm() const;
  virtual std::string sequentialFitOutputName() const;
  virtual std::string simultaneousFitOutputName() const;
  virtual std::string singleFitOutputName(TableDatasetIndex index, WorkspaceIndex spectrum) const;
  virtual std::unordered_map<std::string, ParameterValue> createDefaultParameters(TableDatasetIndex index) const;

  virtual std::string getResultXAxisUnit() const;
  virtual std::string getResultLogName() const;

  bool isPreviousModelSelected() const;

  bool m_previousModelSelected;
  FittingMode m_fittingMode;
  std::unique_ptr<IIndirectFitOutput> m_fitOutput;
  Mantid::API::MultiDomainFunction_sptr m_activeFunction;
  // stores the single domain function
  Mantid::API::IFunction_sptr m_fitFunction;
  DefaultParametersType m_defaultParameters;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
