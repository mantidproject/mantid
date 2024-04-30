// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FittingPresenter.h"
#include "IFitOutput.h"
#include "InelasticFitPropertyBrowser.h"

namespace MantidQt::CustomInterfaces::Inelastic {

FittingPresenter::FittingPresenter(IFitTab *tab, InelasticFitPropertyBrowser *browser,
                                   std::unique_ptr<FittingModel> model)
    : m_tab(tab), m_fitPropertyBrowser(browser), m_model(std::move(model)) {}

void FittingPresenter::validate(UserInputValidator &validator) { m_model->validate(validator); }

void FittingPresenter::setFitFunction(Mantid::API::MultiDomainFunction_sptr function) {
  m_model->setFitFunction(std::move(function));
}

void FittingPresenter::setFittingMode(FittingMode mode) { m_model->setFittingMode(mode); }

FittingMode FittingPresenter::getFittingMode() const { return m_model->getFittingMode(); }

void FittingPresenter::setErrorsEnabled(bool const enable) { m_fitPropertyBrowser->setErrorsEnabled(enable); }

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

void FittingPresenter::cleanFailedRun(const Mantid::API::IAlgorithm_sptr &fittingAlgorithm) {
  m_model->cleanFailedRun(fittingAlgorithm);
}

void FittingPresenter::cleanFailedSingleRun(const Mantid::API::IAlgorithm_sptr &fittingAlgorithm,
                                            WorkspaceID workspaceID) {
  m_model->cleanFailedSingleRun(fittingAlgorithm, workspaceID);
}

void FittingPresenter::removeFittingData() { m_model->removeFittingData(); }

void FittingPresenter::addDefaultParameters() { m_model->addDefaultParameters(); }

void FittingPresenter::removeDefaultParameters() { m_model->removeDefaultParameters(); }

void FittingPresenter::addOutput(Mantid::API::IAlgorithm_sptr &fittingAlgorithm) {
  m_model->addOutput(fittingAlgorithm);
}

void FittingPresenter::addSingleFitOutput(const Mantid::API::IAlgorithm_sptr &fittingAlgorithm, WorkspaceID workspaceID,
                                          WorkspaceIndex spectrum) {
  m_model->addSingleFitOutput(fittingAlgorithm, workspaceID, spectrum);
}

std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> FittingPresenter::fitProperties() const {
  return m_fitPropertyBrowser->fitProperties(m_model->getFittingMode());
}

Mantid::API::WorkspaceGroup_sptr FittingPresenter::getResultWorkspace() const { return m_model->getResultWorkspace(); }

IFitOutput *FittingPresenter::getFitOutput() const { return m_model->getFitOutput(); }

Mantid::API::IAlgorithm_sptr FittingPresenter::getFittingAlgorithm(FittingMode mode) const {
  return m_model->getFittingAlgorithm(mode);
}

Mantid::API::IAlgorithm_sptr FittingPresenter::getSingleFit(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
  return m_model->getSingleFit(workspaceID, spectrum);
}

std::string FittingPresenter::getOutputBasename() const { return m_model->getOutputBasename(); }

IFitDataModel *FittingPresenter::getFitDataModel() const { return m_model->getFitDataModel(); }

bool FittingPresenter::isPreviouslyFit(WorkspaceID workspaceID, WorkspaceIndex spectrum) const {
  return m_model->isPreviouslyFit(workspaceID, spectrum);
}

void FittingPresenter::setFWHM(WorkspaceID WorkspaceID, double fwhm) { m_model->setFWHM(fwhm, WorkspaceID); }

void FittingPresenter::setBackground(WorkspaceID WorkspaceID, double background) {
  m_model->setBackground(background, WorkspaceID);
  m_fitPropertyBrowser->setBackgroundA0(background);
}

void FittingPresenter::updateFittingModeFromBrowser() {
  m_model->setFittingMode(m_fitPropertyBrowser->getFittingMode());
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

void FittingPresenter::updateFitBrowserParameterValuesFromAlg(const Mantid::API::IAlgorithm_sptr &fittingAlgorithm,
                                                              std::size_t const &numberOfDomains) {
  updateFitBrowserParameterValues();
  if (fittingAlgorithm) {
    QSignalBlocker blocker(m_fitPropertyBrowser);
    if (m_model->getFittingMode() == FittingMode::SEQUENTIAL) {
      auto const paramWsName = fittingAlgorithm->getPropertyValue("OutputParameterWorkspace");
      auto paramWs = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(paramWsName);
      auto rowCount = static_cast<int>(paramWs->rowCount());
      if (rowCount == static_cast<int>(numberOfDomains))
        m_fitPropertyBrowser->updateMultiDatasetParameters(*paramWs);
    } else {
      IFunction_sptr fun = fittingAlgorithm->getProperty("Function");
      if (fun->getNumberDomains() > 1)
        m_fitPropertyBrowser->updateMultiDatasetParameters(*fun);
      else
        m_fitPropertyBrowser->updateParameters(*fun);
    }
  }
}

void FittingPresenter::updateFitStatus(const Mantid::API::IAlgorithm_sptr &fittingAlgorithm,
                                       std::size_t const &numberOfDomains) {
  if (m_model->getFittingMode() == FittingMode::SIMULTANEOUS) {
    std::string fit_status = fittingAlgorithm->getProperty("OutputStatus");
    double chi2 = fittingAlgorithm->getProperty("OutputChiSquared");
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