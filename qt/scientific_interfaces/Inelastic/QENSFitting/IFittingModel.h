// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "IFitOutput.h"
#include "MantidAPI/IFunction_fwd.h"
#include "MantidQtWidgets/Common/FittingMode.h"
#include "MantidQtWidgets/Common/IndexTypes.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <optional>

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {
using namespace MantidWidgets;

class IDataModel;
class IFitPlotModel;

extern std::unordered_map<FittingMode, std::string> fitModeToName;
/*
    IFitData - Specifies an interface for updating, querying and
   accessing the raw data in Tabs
*/
class MANTIDQT_INELASTIC_DLL IFittingModel {
public:
  virtual ~IFittingModel(){};
  virtual bool isPreviouslyFit(WorkspaceID workspaceID, WorkspaceIndex spectrum) const = 0;
  virtual std::optional<std::string> isInvalidFunction() const = 0;
  virtual std::vector<std::string> getFitParameterNames() const = 0;
  virtual Mantid::API::MultiDomainFunction_sptr getFitFunction() const = 0;
  virtual std::unordered_map<std::string, ParameterValue> getParameterValues(WorkspaceID workspaceID,
                                                                             WorkspaceIndex spectrum) const = 0;

  virtual void setFitFunction(Mantid::API::MultiDomainFunction_sptr function) = 0;
  virtual void setFWHM(double fwhm, WorkspaceID WorkspaceID) = 0;
  virtual void setBackground(double fwhm, WorkspaceID WorkspaceID) = 0;
  virtual void setDefaultParameterValue(const std::string &name, double value, WorkspaceID workspaceID) = 0;

  // IFittingModel
  virtual std::unordered_map<std::string, ParameterValue> getFitParameters(WorkspaceID workspaceID,
                                                                           WorkspaceIndex spectrum) const = 0;
  virtual std::unordered_map<std::string, ParameterValue> getDefaultParameters(WorkspaceID workspaceID) const = 0;

  virtual void validate(MantidQt::CustomInterfaces::IUserInputValidator *validator) const = 0;

  // Functions that interact with FitDataModel
  virtual void clearWorkspaces() = 0;
  virtual Mantid::API::MatrixWorkspace_sptr getWorkspace(WorkspaceID workspaceID) const = 0;
  virtual WorkspaceID getNumberOfWorkspaces() const = 0;
  virtual bool isMultiFit() const = 0;

  // IFitOutput
  virtual void addOutput(Mantid::API::IAlgorithm_sptr fitAlgorithm) = 0;
  virtual IFitOutput *getFitOutput() const = 0;

  // Generic
  virtual void setFittingMode(FittingMode mode) = 0;
  virtual FittingMode getFittingMode() const = 0;

  virtual void updateFitTypeString() = 0;
  virtual std::optional<ResultLocationNew> getResultLocation(WorkspaceID workspaceID,
                                                             WorkspaceIndex spectrum) const = 0;
  virtual Mantid::API::WorkspaceGroup_sptr getResultWorkspace() const = 0;
  virtual Mantid::API::WorkspaceGroup_sptr getResultGroup() const = 0;
  virtual Mantid::API::IAlgorithm_sptr getFittingAlgorithm(FittingMode mode) const = 0;
  virtual Mantid::API::IAlgorithm_sptr getSingleFittingAlgorithm() const = 0;
  virtual Mantid::API::IFunction_sptr getSingleFunction(WorkspaceID workspaceID, WorkspaceIndex spectrum) const = 0;
  virtual std::optional<std::string> getOutputBasename() const = 0;

  virtual void cleanFailedRun(const Mantid::API::IAlgorithm_sptr &fittingAlgorithm) = 0;
  virtual void removeFittingData() = 0;
  virtual void addDefaultParameters() = 0;
  virtual void removeDefaultParameters() = 0;
  virtual IDataModel *getFitDataModel() const = 0;
  virtual IFitPlotModel *getFitPlotModel() const = 0;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
