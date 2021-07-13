// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidQtWidgets/Common/FittingMode.h"
#include "MantidQtWidgets/Common/IndexTypes.h"
#include "ParameterEstimation.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
using namespace MantidWidgets;

extern std::unordered_map<FittingMode, std::string> fitModeToName;
/*
    IIndirectFitData - Specifies an interface for updating, querying and
   accessing the raw data in IndirectFitAnalysisTabs
*/
class MANTIDQT_INDIRECT_DLL IIndirectFittingModel {
public:
  virtual ~IIndirectFittingModel(){};
  virtual bool isPreviouslyFit(TableDatasetIndex dataIndex, WorkspaceIndex spectrum) const = 0;
  virtual boost::optional<std::string> isInvalidFunction() const = 0;
  virtual std::vector<std::string> getFitParameterNames() const = 0;
  virtual Mantid::API::MultiDomainFunction_sptr getFitFunction() const = 0;
  virtual std::unordered_map<std::string, ParameterValue> getParameterValues(TableDatasetIndex dataIndex,
                                                                             WorkspaceIndex spectrum) const = 0;

  virtual void setFitFunction(Mantid::API::MultiDomainFunction_sptr function) = 0;
  virtual void setDefaultParameterValue(const std::string &name, double value, TableDatasetIndex dataIndex) = 0;

  // IIndirectFittingModel
  virtual std::unordered_map<std::string, ParameterValue> getFitParameters(TableDatasetIndex dataIndex,
                                                                           WorkspaceIndex spectrum) const = 0;
  virtual std::unordered_map<std::string, ParameterValue> getDefaultParameters(TableDatasetIndex dataIndex) const = 0;

  // Functions that interact with IndirectFitDataTableModel
  virtual bool hasWorkspace(std::string const &workspaceName) const = 0;
  virtual Mantid::API::MatrixWorkspace_sptr getWorkspace(TableDatasetIndex index) const = 0;
  virtual FunctionModelSpectra getSpectra(TableDatasetIndex index) const = 0;
  virtual bool isMultiFit() const = 0;
  virtual TableDatasetIndex getNumberOfWorkspaces() const = 0;
  virtual size_t getNumberOfSpectra(TableDatasetIndex index) const = 0;
  virtual size_t getNumberOfDomains() const = 0;
  virtual FitDomainIndex getDomainIndex(TableDatasetIndex dataIndex, WorkspaceIndex spectrum) const = 0;
  virtual std::vector<double> getQValuesForData() const = 0;
  virtual std::vector<std::pair<std::string, size_t>> getResolutionsForFit() const = 0;
  virtual void clearWorkspaces() = 0;
  virtual void clear() = 0;

  virtual void setSpectra(const std::string &spectra, TableDatasetIndex dataIndex) = 0;
  virtual void setSpectra(FunctionModelSpectra &&spectra, TableDatasetIndex dataIndex) = 0;
  virtual void setSpectra(const FunctionModelSpectra &spectra, TableDatasetIndex dataIndex) = 0;
  virtual void addWorkspace(const std::string &workspaceName) = 0;
  virtual void addWorkspace(const std::string &workspaceName, const std::string &spectra) = 0;
  virtual void addWorkspace(const std::string &workspaceName, const FunctionModelSpectra &spectra) = 0;
  virtual void addWorkspace(Mantid::API::MatrixWorkspace_sptr workspace, const FunctionModelSpectra &spectra) = 0;
  virtual void removeWorkspace(TableDatasetIndex index) = 0;
  virtual std::pair<double, double> getFittingRange(TableDatasetIndex dataIndex, WorkspaceIndex spectrum) const = 0;
  virtual std::string getExcludeRegion(TableDatasetIndex dataIndex, WorkspaceIndex index) const = 0;
  virtual void setStartX(double startX, TableDatasetIndex dataIndex, WorkspaceIndex spectrum) = 0;
  virtual void setStartX(double startX, TableDatasetIndex dataIndex) = 0;
  virtual void setEndX(double endX, TableDatasetIndex dataIndex, WorkspaceIndex spectrum) = 0;
  virtual void setEndX(double endX, TableDatasetIndex dataIndex) = 0;
  virtual void setExcludeRegion(const std::string &exclude, TableDatasetIndex dataIndex, WorkspaceIndex spectrum) = 0;

  // IIndirectFitOutput
  virtual void addSingleFitOutput(const Mantid::API::IAlgorithm_sptr &fitAlgorithm, TableDatasetIndex index,
                                  WorkspaceIndex spectrum) = 0;
  virtual void addOutput(Mantid::API::IAlgorithm_sptr fitAlgorithm) = 0;

  // Generic
  virtual void setFittingMode(FittingMode mode) = 0;
  virtual FittingMode getFittingMode() const = 0;

  virtual void setFitTypeString(const std::string &fitType) = 0;
  virtual boost::optional<ResultLocationNew> getResultLocation(TableDatasetIndex dataIndex,
                                                               WorkspaceIndex spectrum) const = 0;
  virtual Mantid::API::WorkspaceGroup_sptr getResultWorkspace() const = 0;
  virtual Mantid::API::WorkspaceGroup_sptr getResultGroup() const = 0;
  virtual Mantid::API::IAlgorithm_sptr getFittingAlgorithm() const = 0;
  virtual Mantid::API::IAlgorithm_sptr getSingleFit(TableDatasetIndex dataIndex, WorkspaceIndex spectrum) const = 0;
  virtual Mantid::API::IFunction_sptr getSingleFunction(TableDatasetIndex dataIndex, WorkspaceIndex spectrum) const = 0;
  virtual std::string getOutputBasename() const = 0;
  virtual std::string createDisplayName(TableDatasetIndex dataIndex) const = 0;

  virtual void cleanFailedRun(const Mantid::API::IAlgorithm_sptr &fittingAlgorithm) = 0;
  virtual void cleanFailedSingleRun(const Mantid::API::IAlgorithm_sptr &fittingAlgorithm, TableDatasetIndex index) = 0;
  virtual DataForParameterEstimationCollection
  getDataForParameterEstimation(const EstimationDataSelector &selector) const = 0;
  virtual void removeFittingData() = 0;

  virtual IIndirectFitDataTableModel *getFitDataModel() = 0;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
