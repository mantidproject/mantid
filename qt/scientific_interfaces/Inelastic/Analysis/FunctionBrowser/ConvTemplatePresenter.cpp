// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ConvTemplatePresenter.h"
#include "ConvTemplateBrowser.h"
#include "MantidQtWidgets/Common/EditLocalParameterDialog.h"
#include <QInputDialog>
#include <QtConcurrentRun>
#include <cmath>
#include <float.h>

namespace MantidQt::CustomInterfaces::IDA {

namespace {
class ScopedDisable {
  FunctionTemplateBrowser *m_browser;

public:
  // Disables the function browser and re-enables when leaving scope
  ScopedDisable(FunctionTemplateBrowser *browser) : m_browser(browser) { m_browser->setDisabled(true); }
  ~ScopedDisable() { m_browser->setDisabled(false); }
};
} // namespace

using namespace MantidWidgets;

/**
 * Constructor
 * @param parent :: The parent widget.
 */
ConvTemplatePresenter::ConvTemplatePresenter(ConvTemplateBrowser *view, std::unique_ptr<ConvFunctionModel> model)
    : FunctionTemplatePresenter(view, std::move(model)) {
  m_view->subscribePresenter(this);
}

ConvTemplateBrowser *ConvTemplatePresenter::view() const { return dynamic_cast<ConvTemplateBrowser *>(m_view); }

ConvFunctionModel *ConvTemplatePresenter::model() const { return dynamic_cast<ConvFunctionModel *>(m_model.get()); }

// This function creates a Qt thread to run the model updates
// This was found to be necessary to allow the processing of the GUI thread to
// continue which is necessary to stop the int manager from self-incrementing
// itself due to an internal timer occurring within the class
void ConvTemplatePresenter::setSubType(size_t subTypeIndex, int typeIndex) {
  if (subTypeIndex == SubTypeIndex::Fit) {
    model()->setFitType(static_cast<FitType>(typeIndex));
  } else if (subTypeIndex == SubTypeIndex::Lorentzian) {
    model()->setLorentzianType(static_cast<LorentzianType>(typeIndex));
  } else {
    model()->setBackground(static_cast<BackgroundType>(typeIndex));
  }
  view()->setSubType(subTypeIndex, typeIndex);
  setErrorsEnabled(false);
  updateViewParameterNames();
  updateViewParameters();
  m_view->emitFunctionStructureChanged();
}

void ConvTemplatePresenter::setDeltaFunction(bool on) {
  if (on == model()->hasDeltaFunction())
    return;
  model()->setDeltaFunction(on);
  if (on)
    view()->addDeltaFunction();
  else
    view()->removeDeltaFunction();

  setErrorsEnabled(false);
  updateViewParameterNames();
  updateViewParameters();
  m_view->emitFunctionStructureChanged();
}

void ConvTemplatePresenter::setTempCorrection(bool on) {
  if (on == model()->hasTempCorrection())
    return;
  double temp = model()->getTempValue();
  if (on) {
    bool ok;
    temp = QInputDialog::getDouble(m_view, "Temperature", "Set Temperature", temp, 0.0,
                                   std::numeric_limits<double>::max(), 3, &ok);
    if (!ok)
      return;
  }
  model()->setTempCorrection(on, temp);
  if (on)
    view()->addTempCorrection(temp);
  else
    view()->removeTempCorrection();

  setErrorsEnabled(false);
  updateViewParameterNames();
  updateViewParameters();
  m_view->emitFunctionStructureChanged();
}

void ConvTemplatePresenter::setNumberOfDatasets(int n) { m_model->setNumberDomains(n); }

int ConvTemplatePresenter::getNumberOfDatasets() const { return m_model->getNumberDomains(); }

void ConvTemplatePresenter::setFunction(std::string const &funStr) {
  m_model->setFunctionString(funStr);

  auto convView = view();
  auto convModel = model();
  convView->updateTemperatureCorrectionAndDelta(convModel->hasTempCorrection(), convModel->hasDeltaFunction());

  convView->setSubType(SubTypeIndex::Lorentzian, static_cast<int>(convModel->getLorentzianType()));
  convView->setSubType(SubTypeIndex::Fit, static_cast<int>(convModel->getFitType()));
  convView->setSubType(SubTypeIndex::Background, static_cast<int>(convModel->getBackgroundType()));

  convView->setInt(SubTypeIndex::Lorentzian, static_cast<int>(convModel->getLorentzianType()));
  convView->setEnum(SubTypeIndex::Fit, static_cast<int>(convModel->getFitType()));
  convView->setEnum(SubTypeIndex::Background, static_cast<int>(convModel->getBackgroundType()));

  setErrorsEnabled(false);
  updateViewParameterNames();
  updateViewParameters();
  m_view->emitFunctionStructureChanged();
}

int ConvTemplatePresenter::getCurrentDataset() { return m_model->currentDomainIndex(); }

IFunction_sptr ConvTemplatePresenter::getGlobalFunction() const { return m_model->getFitFunction(); }

IFunction_sptr ConvTemplatePresenter::getFunction() const { return m_model->getCurrentFunction(); }

std::vector<std::string> ConvTemplatePresenter::getGlobalParameters() const { return m_model->getGlobalParameters(); }

std::vector<std::string> ConvTemplatePresenter::getLocalParameters() const { return m_model->getLocalParameters(); }

void ConvTemplatePresenter::setGlobalParameters(std::vector<std::string> const &globals) {
  m_model->setGlobalParameters(globals);
}

void ConvTemplatePresenter::setGlobal(std::string const &parameterName, bool on) {
  auto globals = m_model->getGlobalParameters();
  auto const findIter = std::find(globals.cbegin(), globals.cend(), parameterName);
  if (on) {
    if (findIter == globals.cend()) {
      globals.emplace_back(parameterName);
    }
  } else if (findIter != globals.cend()) {
    globals.erase(findIter);
  }
  setGlobalParameters(globals);
}

void ConvTemplatePresenter::updateMultiDatasetParameters(const IFunction &fun) {
  m_model->updateMultiDatasetParameters(fun);
  updateViewParameters();
}

void ConvTemplatePresenter::updateMultiDatasetParameters(const ITableWorkspace &table) {
  model()->updateMultiDatasetParameters(table);
  updateViewParameters();
}

void ConvTemplatePresenter::updateParameters(const IFunction &fun) {
  m_model->updateParameters(fun);
  updateViewParameters();
}

void ConvTemplatePresenter::setCurrentDataset(int i) {
  m_model->setCurrentDomainIndex(i);
  updateViewParameters();
}

void ConvTemplatePresenter::setDatasets(const QList<FunctionModelDataset> &datasets) { m_model->setDatasets(datasets); }

void ConvTemplatePresenter::setBackgroundA0(double value) {
  m_model->setBackgroundA0(value);
  updateViewParameters();
}

void ConvTemplatePresenter::setQValues(const std::vector<double> &qValues) { model()->setQValues(qValues); }

void ConvTemplatePresenter::setErrorsEnabled(bool enabled) { m_view->setErrorsEnabled(enabled); }

void ConvTemplatePresenter::setResolution(const std::vector<std::pair<std::string, size_t>> &fitResolutions) {
  model()->setResolution(fitResolutions);
}

void ConvTemplatePresenter::updateViewParameters() {
  auto values = model()->getCurrentValues();
  auto errors = model()->getCurrentErrors();
  for (auto const id : values.keys()) {
    view()->setParameterValueQuiet(id, values[id], errors[id]);
  }
}

QStringList ConvTemplatePresenter::getDatasetNames() const { return m_model->getDatasetNames(); }

QStringList ConvTemplatePresenter::getDatasetDomainNames() const { return m_model->getDatasetDomainNames(); }

double ConvTemplatePresenter::getLocalParameterValue(std::string const &parameterName, int i) const {
  return m_model->getLocalParameterValue(parameterName, i);
}

bool ConvTemplatePresenter::isLocalParameterFixed(std::string const &parameterName, int i) const {
  return m_model->isLocalParameterFixed(parameterName, i);
}

std::string ConvTemplatePresenter::getLocalParameterTie(std::string const &parameterName, int i) const {
  return m_model->getLocalParameterTie(parameterName, i);
}

std::string ConvTemplatePresenter::getLocalParameterConstraint(std::string const &parameterName, int i) const {
  return m_model->getLocalParameterConstraint(parameterName, i);
}

void ConvTemplatePresenter::setLocalParameterValue(std::string const &parameterName, int i, double value) {
  m_model->setLocalParameterValue(parameterName, i, value);
}

void ConvTemplatePresenter::setLocalParameterTie(std::string const &parameterName, int i, std::string const &tie) {
  m_model->setLocalParameterTie(parameterName, i, tie);
}

void ConvTemplatePresenter::updateViewParameterNames() { m_view->updateParameterNames(model()->getParameterNameMap()); }

void ConvTemplatePresenter::setLocalParameterFixed(std::string const &parameterName, int i, bool fixed) {
  m_model->setLocalParameterFixed(parameterName, i, fixed);
}

void ConvTemplatePresenter::handleEditLocalParameter(std::string const &parameterName) {
  auto const datasetNames = getDatasetNames();
  auto const domainNames = getDatasetDomainNames();
  QList<double> values;
  QList<bool> fixes;
  QStringList ties;
  QStringList constraints;
  const int n = domainNames.size();
  for (int i = 0; i < n; ++i) {
    const double value = getLocalParameterValue(parameterName, i);
    values.push_back(value);
    const bool fixed = isLocalParameterFixed(parameterName, i);
    fixes.push_back(fixed);
    const auto tie = getLocalParameterTie(parameterName, i);
    ties.push_back(QString::fromStdString(tie));
    const auto constraint = getLocalParameterConstraint(parameterName, i);
    constraints.push_back(QString::fromStdString(constraint));
  }
  m_view->openEditLocalParameterDialog(parameterName, datasetNames, domainNames, values, fixes, ties, constraints);
}

void ConvTemplatePresenter::handleEditLocalParameterFinished(std::string const &parameterName,
                                                             QList<double> const &values, QList<bool> const &fixes,
                                                             QStringList const &ties, QStringList const &constraints) {
  assert(values.size() == getNumberOfDatasets());
  for (int i = 0; i < values.size(); ++i) {
    setLocalParameterValue(parameterName, i, values[i]);
    if (!ties[i].isEmpty()) {
      setLocalParameterTie(parameterName, i, ties[i].toStdString());
    } else if (fixes[i]) {
      setLocalParameterFixed(parameterName, i, fixes[i]);
    } else {
      setLocalParameterTie(parameterName, i, "");
    }
    m_model->setLocalParameterConstraint(parameterName, i, constraints[i].toStdString());
  }
  updateViewParameters();
}

void ConvTemplatePresenter::handleParameterValueChanged(std::string const &parameterName, double value) {
  if (parameterName.empty())
    return;
  if (m_model->isGlobal(parameterName)) {
    auto const n = getNumberOfDatasets();
    for (int i = 0; i < n; ++i) {
      setLocalParameterValue(parameterName, i, value);
    }
  } else {
    auto const i = m_model->currentDomainIndex();
    auto const oldValue = m_model->getLocalParameterValue(parameterName, i);
    if (fabs(value - oldValue) > 1e-6) {
      setErrorsEnabled(false);
    }
    setLocalParameterValue(parameterName, i, value);
  }
  m_view->emitFunctionStructureChanged();
}

EstimationDataSelector ConvTemplatePresenter::getEstimationDataSelector() const {
  return model()->getEstimationDataSelector();
}

void ConvTemplatePresenter::updateParameterEstimationData(DataForParameterEstimationCollection &&data) {
  model()->updateParameterEstimationData(std::move(data));
}

void ConvTemplatePresenter::estimateFunctionParameters() {
  model()->estimateFunctionParameters();
  updateViewParameters();
}

} // namespace MantidQt::CustomInterfaces::IDA
