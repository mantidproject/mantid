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
  virtual bool isPreviouslyFit(WorkspaceID workspaceID, WorkspaceIndex spectrum) const = 0;
  virtual boost::optional<std::string> isInvalidFunction() const = 0;
  virtual std::vector<std::string> getFitParameterNames() const = 0;
  virtual Mantid::API::MultiDomainFunction_sptr getFitFunction() const = 0;
  virtual std::unordered_map<std::string, ParameterValue> getParameterValues(WorkspaceID workspaceID,
                                                                             WorkspaceIndex spectrum) const = 0;

  virtual void setFitFunction(Mantid::API::MultiDomainFunction_sptr function) = 0;
  virtual void setDefaultParameterValue(const std::string &name, double value, WorkspaceID workspaceID) = 0;

  // IIndirectFittingModel
  virtual std::unordered_map<std::string, ParameterValue> getFitParameters(WorkspaceID workspaceID,
                                                                           WorkspaceIndex spectrum) const = 0;
  virtual std::unordered_map<std::string, ParameterValue> getDefaultParameters(WorkspaceID workspaceID) const = 0;

  // Functions that interact with IndirectFitDataTableModel
  virtual bool hasWorkspace(std::string const &workspaceName) const = 0;
  virtual Mantid::API::MatrixWorkspace_sptr getWorkspace(WorkspaceID workspaceID) const = 0;
  virtual FunctionModelSpectra getSpectra(WorkspaceID workspaceID) const = 0;
  virtual bool isMultiFit() const = 0;
  virtual WorkspaceID getNumberOfWorkspaces() const = 0;
  virtual size_t getNumberOfSpectra(WorkspaceID workspaceID) const = 0;
  virtual size_t getNumberOfDomains() const = 0;
  virtual FitDomainIndex getDomainIndex(WorkspaceID workspaceID, WorkspaceIndex spectrum) const = 0;
  virtual std::vector<double> getQValuesForData() const = 0;
  virtual std::vector<std::pair<std::string, size_t>> getResolutionsForFit() const = 0;
  virtual void clearWorkspaces() = 0;
  virtual void clear() = 0;

  virtual void setSpectra(const std::string &spectra, WorkspaceID workspaceID) = 0;
  virtual void setSpectra(FunctionModelSpectra &&spectra, WorkspaceID workspaceID) = 0;
  virtual void setSpectra(const FunctionModelSpectra &spectra, WorkspaceID workspaceID) = 0;
  virtual void addWorkspace(const std::string &workspaceName) = 0;
  virtual void addWorkspace(const std::string &workspaceName, const std::string &spectra) = 0;
  virtual void addWorkspace(const std::string &workspaceName, const FunctionModelSpectra &spectra) = 0;
  virtual void addWorkspace(Mantid::API::MatrixWorkspace_sptr workspace, const FunctionModelSpectra &spectra) = 0;
  virtual void removeWorkspace(WorkspaceID workspaceID) = 0;
  virtual std::pair<double, double> getFittingRange(WorkspaceID workspaceID, WorkspaceIndex spectrum) const = 0;
  virtual std::string getExcludeRegion(WorkspaceID workspaceID, WorkspaceIndex spectrum) const = 0;
  virtual void setStartX(double startX, WorkspaceID workspaceID, WorkspaceIndex spectrum) = 0;
  virtual void setStartX(double startX, WorkspaceID workspaceID) = 0;
  virtual void setEndX(double endX, WorkspaceID workspaceID, WorkspaceIndex spectrum) = 0;
  virtual void setEndX(double endX, WorkspaceID workspaceID) = 0;
  virtual void setExcludeRegion(const std::string &exclude, WorkspaceID workspaceID, WorkspaceIndex spectrum) = 0;

  // IIndirectFitOutput
  virtual void addSingleFitOutput(const Mantid::API::IAlgorithm_sptr &fitAlgorithm, WorkspaceID workspaceID,
                                  WorkspaceIndex spectrum) = 0;
  virtual void addOutput(Mantid::API::IAlgorithm_sptr fitAlgorithm) = 0;

  // Generic
  virtual void setFittingMode(FittingMode mode) = 0;
  virtual FittingMode getFittingMode() const = 0;

  virtual void setFitTypeString(const std::string &fitType) = 0;
  virtual boost::optional<ResultLocationNew> getResultLocation(WorkspaceID workspaceID,
                                                               WorkspaceIndex spectrum) const = 0;
  virtual Mantid::API::WorkspaceGroup_sptr getResultWorkspace() const = 0;
  virtual Mantid::API::WorkspaceGroup_sptr getResultGroup() const = 0;
  virtual Mantid::API::IAlgorithm_sptr getFittingAlgorithm() const = 0;
  virtual Mantid::API::IAlgorithm_sptr getSingleFit(WorkspaceID workspaceID, WorkspaceIndex spectrum) const = 0;
  virtual Mantid::API::IFunction_sptr getSingleFunction(WorkspaceID workspaceID, WorkspaceIndex spectrum) const = 0;
  virtual std::string getOutputBasename() const = 0;
  virtual std::string createDisplayName(WorkspaceID workspaceID) const = 0;

  virtual void cleanFailedRun(const Mantid::API::IAlgorithm_sptr &fittingAlgorithm) = 0;
  virtual void cleanFailedSingleRun(const Mantid::API::IAlgorithm_sptr &fittingAlgorithm, WorkspaceID workspaceID) = 0;
  virtual DataForParameterEstimationCollection
  getDataForParameterEstimation(const EstimationDataSelector &selector) const = 0;
  virtual void removeFittingData() = 0;

  virtual IIndirectFitDataTableModel *getFitDataModel() = 0;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
