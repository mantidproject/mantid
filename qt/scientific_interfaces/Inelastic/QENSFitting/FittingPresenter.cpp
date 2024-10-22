// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FittingPresenter.h"
#include "FitTab.h"
#include "FittingModel.h"
#include "IFitOutput.h"
#include "InelasticFitPropertyBrowser.h"

namespace MantidQt::CustomInterfaces::Inelastic {

FittingPresenter::FittingPresenter(IFitTab *tab, IInelasticFitPropertyBrowser *browser,
                                   std::unique_ptr<IFittingModel> model,
                                   std::unique_ptr<MantidQt::API::IAlgorithmRunner> algorithmRunner)
    : m_tab(tab), m_fitPropertyBrowser(browser), m_model(std::move(model)),
      m_algorithmRunner(std::move(algorithmRunner)) {
  m_fitPropertyBrowser->subscribePresenter(this);
  m_algorithmRunner->subscribe(this);
}

void FittingPresenter::notifyFunctionChanged() { m_tab->handleFunctionChanged(); }

void FittingPresenter::validate(IUserInputValidator *validator) { m_model->validate(validator); }

void FittingPresenter::setFitFunction(Mantid::API::MultiDomainFunction_sptr function) {
  m_model->setFitFunction(std::move(function));
}

void FittingPresenter::setFitEnabled(bool const enable) { m_fitPropertyBrowser->setFitEnabled(enable); }

void FittingPresenter::setCurrentDataset(FitDomainIndex index) { m_fitPropertyBrowser->setCurrentDataset(index); }

MultiDomainFunction_sptr FittingPresenter::fitFunction() const { return m_fitPropertyBrowser->getFitFunction(); }

std::string FittingPresenter::minimizer() const { return m_fitPropertyBrowser->minimizer(); }

EstimationDataSelector FittingPresenter::getEstimationDataSelector() const {
  return m_fitPropertyBrowser->getEstimationDataSelector();
}

void FittingPresenter::updateParameterEstimationData(DataForParameterEstimationCollection &&data) {
  m_fitPropertyBrowser->updateParameterEstimationData(std::move(data));
}

void FittingPresenter::estimateFunctionParameters(WorkspaceID workspaceID, WorkspaceIndex spectrum) {
  if (!m_model->isPreviouslyFit(workspaceID, spectrum)) {
    m_fitPropertyBrowser->estimateFunctionParameters();
  }
}

void FittingPresenter::updateFunctionBrowserData(int nData, const QList<MantidWidgets::FunctionModelDataset> &datasets,
                                                 const std::vector<double> &qValues,
                                                 const std::vector<std::pair<std::string, size_t>> &fitResolutions) {
  m_fitPropertyBrowser->updateFunctionBrowserData(nData, datasets, qValues, fitResolutions);
}

void FittingPresenter::updateFunctionListInBrowser(const std::map<std::string, std::string> &functionStrings) {
  m_fitPropertyBrowser->updateFunctionListInBrowser(functionStrings);
}

void FittingPresenter::removeFittingData() { m_model->removeFittingData(); }

void FittingPresenter::addDefaultParameters() { m_model->addDefaultParameters(); }

void FittingPresenter::removeDefaultParameters() { m_model->removeDefaultParameters(); }

void FittingPresenter::runFit() {
  m_model->setFittingMode(m_fitPropertyBrowser->getFittingMode());
  executeFit(m_model->getFittingAlgorithm(m_model->getFittingMode()));
}

void FittingPresenter::runSingleFit() {
  m_model->setFittingMode(FittingMode::SIMULTANEOUS);
  executeFit(m_model->getSingleFittingAlgorithm());
}

void FittingPresenter::notifyBatchComplete(MantidQt::API::IConfiguredAlgorithm_sptr &lastAlgorithm, bool error) {
  m_fitPropertyBrowser->setErrorsEnabled(!error);
  if (!error) {
    updateFitBrowserParameterValuesFromAlg(lastAlgorithm->algorithm());
    m_model->setFitFunction(m_fitPropertyBrowser->getFitFunction());
    m_model->addOutput(lastAlgorithm->algorithm());
  } else {
    m_model->cleanFailedRun(lastAlgorithm->algorithm());
  }
  m_tab->handleFitComplete(error);
}

Mantid::API::WorkspaceGroup_sptr FittingPresenter::getResultWorkspace() const { return m_model->getResultWorkspace(); }

std::optional<std::string> FittingPresenter::getOutputBasename() const { return m_model->getOutputBasename(); }

IDataModel *FittingPresenter::getFitDataModel() const { return m_model->getFitDataModel(); }

IFitPlotModel *FittingPresenter::getFitPlotModel() const { return m_model->getFitPlotModel(); }

bool FittingPresenter::isPreviouslyFit(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
  return m_model->isPreviouslyFit(workspaceID, spectrum);
}

void FittingPresenter::setFWHM(WorkspaceID WorkspaceID, double fwhm) { m_model->setFWHM(fwhm, WorkspaceID); }

void FittingPresenter::setBackground(WorkspaceID WorkspaceID, double background) {
  m_model->setBackground(background, WorkspaceID);
  m_fitPropertyBrowser->setBackgroundA0(background);
}

void FittingPresenter::executeFit(Mantid::API::IAlgorithm_sptr fitAlgorithm) {
  auto properties = m_fitPropertyBrowser->fitProperties(m_model->getFittingMode());
  MantidQt::API::IConfiguredAlgorithm_sptr confAlg =
      std::make_shared<MantidQt::API::ConfiguredAlgorithm>(fitAlgorithm, std::move(properties));
  m_algorithmRunner->execute(std::move(confAlg));
}

void FittingPresenter::updateFitBrowserParameterValues(const std::unordered_map<std::string, ParameterValue> &params) {
  auto fun = m_model->getFitFunction();
  if (fun) {
    for (auto const &pair : params) {
      fun->setParameter(pair.first, pair.second.value);
    }
    if (fun->getNumberDomains() > 1) {
      m_fitPropertyBrowser->updateMultiDatasetParameters(*fun);
    } else {
      m_fitPropertyBrowser->updateParameters(*fun);
    }
  }
}

void FittingPresenter::updateFitBrowserParameterValuesFromAlg(const Mantid::API::IAlgorithm_sptr &fittingAlgorithm) {
  updateFitBrowserParameterValues();
  if (fittingAlgorithm) {
    if (m_model->getFittingMode() == FittingMode::SEQUENTIAL) {
      auto const paramWsName = fittingAlgorithm->getPropertyValue("OutputParameterWorkspace");
      auto paramWs = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(paramWsName);
      auto rowCount = static_cast<int>(paramWs->rowCount());
      if (rowCount == static_cast<int>(m_model->getFitDataModel()->getNumberOfDomains()))
        m_fitPropertyBrowser->updateMultiDatasetParameters(*paramWs);
    } else {
      IFunction_sptr fun = fittingAlgorithm->getProperty("Function");
      if (fun->getNumberDomains() > 1)
        m_fitPropertyBrowser->updateMultiDatasetParameters(*fun);
      else
        m_fitPropertyBrowser->updateParameters(*fun);
    }
    updateFitStatus(fittingAlgorithm);
  }
}

void FittingPresenter::updateFitStatus(const Mantid::API::IAlgorithm_sptr &fittingAlgorithm) {
  if (m_model->getFittingMode() == FittingMode::SIMULTANEOUS) {
    std::string fit_status = fittingAlgorithm->getProperty("OutputStatus");
    double chi2 = fittingAlgorithm->getProperty("OutputChiSquared");
    auto numberOfDomains = m_model->getFitDataModel()->getNumberOfDomains();
    const std::vector<std::string> status(numberOfDomains, fit_status);
    const std::vector<double> chiSquared(numberOfDomains, chi2);
    m_fitPropertyBrowser->updateFitStatusData(status, chiSquared);
  } else {
    const std::vector<std::string> status = fittingAlgorithm->getProperty("OutputStatus");
    const std::vector<double> chiSquared = fittingAlgorithm->getProperty("OutputChiSquared");
    m_fitPropertyBrowser->updateFitStatusData(status, chiSquared);
  }
}

void FittingPresenter::updateFitTypeString() { m_model->updateFitTypeString(); }

} // namespace MantidQt::CustomInterfaces::Inelastic
