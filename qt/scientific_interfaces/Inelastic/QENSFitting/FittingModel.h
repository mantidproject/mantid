// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "FitPlotModel.h"
#include "IFitOutput.h"
#include "IFittingModel.h"
#include "MantidQtWidgets/Spectroscopy/IDataModel.h"
#include "ParameterEstimation.h"

#include "DllConfig.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/FunctionModelSpectra.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <optional>

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {
using namespace MantidWidgets;

extern std::unordered_map<FittingMode, std::string> fitModeToName;

using DefaultParametersType = IndexCollectionType<WorkspaceID, std::unordered_map<std::string, ParameterValue>>;

static const std::string SIM_STRING = "sim";
static const std::string SEQ_STRING = "seq";
static const std::string IQT_STRING = "Iqt";
static const std::string CONVOLUTION_STRING = "Conv";
static const std::string MSD_STRING = "Msd";
static const std::string FUNCTIONQ_STRING = "FQ";
static const std::string MULTI_STRING = "Multi";

/*
    FittingModel - Provides methods for specifying and
    performing a QENS fit, as well as accessing the results of the fit.
*/
class MANTIDQT_INELASTIC_DLL FittingModel : public IFittingModel {
public:
  FittingModel();
  virtual ~FittingModel() override = default;

  void validate(IUserInputValidator *validator) const override;

  // Functions that interact with FitDataModel
  void clearWorkspaces() override;
  Mantid::API::MatrixWorkspace_sptr getWorkspace(WorkspaceID workspaceID) const override;
  WorkspaceID getNumberOfWorkspaces() const override;
  bool isMultiFit() const override;

  // IFittingModel
  bool isPreviouslyFit(WorkspaceID workspaceID, WorkspaceIndex spectrum) const override;

  virtual std::optional<std::string> isInvalidFunction() const override;
  std::vector<std::string> getFitParameterNames() const override;
  void setFitFunction(Mantid::API::MultiDomainFunction_sptr function) override;
  void setFWHM(double fwhm, WorkspaceID WorkspaceID) override;
  void setBackground(double fwhm, WorkspaceID WorkspaceID) override;
  virtual Mantid::API::MultiDomainFunction_sptr getFitFunction() const override;
  void setDefaultParameterValue(const std::string &name, double value, WorkspaceID workspaceID) override;
  std::unordered_map<std::string, ParameterValue> getParameterValues(WorkspaceID workspaceID,
                                                                     WorkspaceIndex spectrum) const override;
  std::unordered_map<std::string, ParameterValue> getFitParameters(WorkspaceID workspaceID,
                                                                   WorkspaceIndex spectrum) const override;
  std::unordered_map<std::string, ParameterValue> getDefaultParameters(WorkspaceID workspaceID) const override;

  // IFitOutput
  virtual void addOutput(Mantid::API::IAlgorithm_sptr fitAlgorithm) override;
  IFitOutput *getFitOutput() const override;

  // Generic
  void setFittingMode(FittingMode mode) override;
  FittingMode getFittingMode() const override;

  void updateFitTypeString() override;
  std::optional<ResultLocationNew> getResultLocation(WorkspaceID workspaceID, WorkspaceIndex spectrum) const override;
  Mantid::API::WorkspaceGroup_sptr getResultWorkspace() const override;
  Mantid::API::WorkspaceGroup_sptr getResultGroup() const override;
  Mantid::API::IAlgorithm_sptr getFittingAlgorithm(FittingMode mode) const override;
  Mantid::API::IAlgorithm_sptr getSingleFittingAlgorithm() const override;
  Mantid::API::IFunction_sptr getSingleFunction(WorkspaceID workspaceID, WorkspaceIndex spectrum) const override;
  std::optional<std::string> getOutputBasename() const override;

  void cleanFailedRun(const Mantid::API::IAlgorithm_sptr &fittingAlgorithm) override;
  void removeFittingData() override;
  void addDefaultParameters() override;
  void removeDefaultParameters() override;

  IDataModel *getFitDataModel() const override;
  IFitPlotModel *getFitPlotModel() const override;

  // Used for testing purposes
  [[nodiscard]] const inline std::string &getFitString() const noexcept { return m_fitString; }

protected:
  std::string createOutputName(const std::string &fitMode, const std::string &workspaceName,
                               const std::string &spectra) const;
  Mantid::API::IAlgorithm_sptr createSimultaneousFit(const Mantid::API::MultiDomainFunction_sptr &function) const;
  virtual Mantid::API::MultiDomainFunction_sptr getMultiDomainFunction() const;
  virtual std::unordered_map<std::string, std::string> mapDefaultParameterNames() const;
  std::string m_fitType;
  std::string m_fitString;

  std::unique_ptr<IDataModel> m_fitDataModel;
  std::unique_ptr<IFitPlotModel> m_fitPlotModel;

private:
  void removeWorkspaceFromFittingData(WorkspaceID const &workspaceIndex);

  Mantid::API::IAlgorithm_sptr createSequentialFit(Mantid::API::IFunction_sptr function) const;
  Mantid::API::IAlgorithm_sptr createSequentialFit(const Mantid::API::IFunction_sptr function,
                                                   const std::string &input) const;
  virtual Mantid::API::IAlgorithm_sptr sequentialFitAlgorithm() const;
  virtual Mantid::API::IAlgorithm_sptr simultaneousFitAlgorithm() const;
  virtual std::optional<std::string> sequentialFitOutputName() const;
  virtual std::optional<std::string> simultaneousFitOutputName() const;
  virtual std::string singleFitOutputName(std::string workspaceName, WorkspaceIndex spectrum) const;
  virtual std::unordered_map<std::string, ParameterValue> createDefaultParameters(WorkspaceID workspaceID) const;

  virtual std::string getResultXAxisUnit() const;
  virtual std::string getResultLogName() const;

  bool isPreviousModelSelected() const;

  bool m_previousModelSelected;
  FittingMode m_fittingMode;
  std::unique_ptr<IFitOutput> m_fitOutput;
  Mantid::API::MultiDomainFunction_sptr m_activeFunction;
  // stores the single domain function
  Mantid::API::IFunction_sptr m_fitFunction;
  DefaultParametersType m_defaultParameters;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
