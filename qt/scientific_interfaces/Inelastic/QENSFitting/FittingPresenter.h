// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllConfig.h"
#include "IFitOutput.h"
#include "IFittingModel.h"
#include "ParameterEstimation.h"

#include "MantidAPI/AlgorithmRuntimeProps.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/AlgorithmRunner.h"
#include "MantidQtWidgets/Common/ConfiguredAlgorithm.h"
#include "MantidQtWidgets/Common/FunctionModelDataset.h"
#include "MantidQtWidgets/Common/IAlgorithmRunnerSubscriber.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace MantidQt {
namespace CustomInterfaces {
namespace Inelastic {

class IDataModel;
class IFitPlotModel;
class IFitOutput;
class IFitTab;
class IInelasticFitPropertyBrowser;

class MANTIDQT_INELASTIC_DLL IFittingPresenter {
public:
  virtual void notifyFunctionChanged() = 0;
};

class MANTIDQT_INELASTIC_DLL FittingPresenter : public IFittingPresenter,
                                                public MantidQt::API::IAlgorithmRunnerSubscriber {
public:
  FittingPresenter(IFitTab *tab, IInelasticFitPropertyBrowser *browser, std::unique_ptr<IFittingModel> model,
                   std::unique_ptr<MantidQt::API::IAlgorithmRunner> algorithmRunner);

  void notifyFunctionChanged() override;

  void validate(IUserInputValidator *validator);

  void setFitFunction(Mantid::API::MultiDomainFunction_sptr function);

  void setFitEnabled(bool const enable);
  void setCurrentDataset(FitDomainIndex index);
  Mantid::API::MultiDomainFunction_sptr fitFunction() const;
  std::string minimizer() const;

  EstimationDataSelector getEstimationDataSelector() const;
  void updateParameterEstimationData(DataForParameterEstimationCollection &&data);
  void estimateFunctionParameters(WorkspaceID workspaceID, WorkspaceIndex spectrum);

  void updateFunctionBrowserData(int nData, const QList<MantidWidgets::FunctionModelDataset> &datasets,
                                 const std::vector<double> &qValues,
                                 const std::vector<std::pair<std::string, size_t>> &fitResolutions);
  void updateFunctionListInBrowser(const std::map<std::string, std::string> &functionStrings);

  void removeFittingData();
  void addDefaultParameters();
  void removeDefaultParameters();

  void runFit();
  void runSingleFit();
  void notifyBatchComplete(MantidQt::API::IConfiguredAlgorithm_sptr &lastAlgorithm, bool error) override;

  Mantid::API::WorkspaceGroup_sptr getResultWorkspace() const;
  std::optional<std::string> getOutputBasename() const;

  IDataModel *getFitDataModel() const;
  IFitPlotModel *getFitPlotModel() const;

  bool isPreviouslyFit(WorkspaceID workspaceID, WorkspaceIndex spectrum) const;

  void setFWHM(WorkspaceID WorkspaceID, double fwhm);
  void setBackground(WorkspaceID WorkspaceID, double background);

  void updateFitBrowserParameterValues(const std::unordered_map<std::string, ParameterValue> &params =
                                           std::unordered_map<std::string, ParameterValue>());
  void updateFitTypeString();

private:
  void executeFit(Mantid::API::IAlgorithm_sptr fitAlgorithm);

  void updateFitBrowserParameterValuesFromAlg(const Mantid::API::IAlgorithm_sptr &fittingAlgorithm);
  void updateFitStatus(const Mantid::API::IAlgorithm_sptr &fittingAlgorithm);

  IFitTab *m_tab;
  IInelasticFitPropertyBrowser *m_fitPropertyBrowser;
  std::unique_ptr<IFittingModel> m_model;
  std::unique_ptr<MantidQt::API::IAlgorithmRunner> m_algorithmRunner;
};

} // namespace Inelastic
} // namespace CustomInterfaces
} // namespace MantidQt
