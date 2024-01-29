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
ConvTemplatePresenter::ConvTemplatePresenter(ConvTemplateBrowser *view,
                                             std::unique_ptr<ConvFunctionModel> functionModel)
    : QObject(view), m_view(view), m_model(std::move(functionModel)) {
  m_view->subscribePresenter(this);
}

// This function creates a Qt thread to run the model updates
// This was found to be necessary to allow the processing of the GUI thread to
// continue which is necessary to stop the int manager from self-incrementing
// itself due to an internal timer occurring within the class
void ConvTemplatePresenter::setSubType(size_t subTypeIndex, int typeIndex) {
  if (subTypeIndex == SubTypeIndex::Fit) {
    m_model->setFitType(static_cast<FitType>(typeIndex));
  } else if (subTypeIndex == SubTypeIndex::Lorentzian) {
    m_model->setLorentzianType(static_cast<LorentzianType>(typeIndex));
  } else {
    m_model->setBackground(static_cast<BackgroundType>(typeIndex));
  }
  m_view->setSubType(subTypeIndex, typeIndex);
  setErrorsEnabled(false);
  updateViewParameterNames();
  updateViewParameters();
  m_view->emitFunctionStructureChanged();
}

void ConvTemplatePresenter::setDeltaFunction(bool on) {
  if (on == m_model->hasDeltaFunction())
    return;
  m_model->setDeltaFunction(on);
  if (on)
    m_view->addDeltaFunction();
  else
    m_view->removeDeltaFunction();

  setErrorsEnabled(false);
  updateViewParameterNames();
  updateViewParameters();
  m_view->emitFunctionStructureChanged();
}

void ConvTemplatePresenter::setTempCorrection(bool on) {
  if (on == m_model->hasTempCorrection())
    return;
  double temp = m_model->getTempValue();
  if (on) {
    bool ok;
    temp = QInputDialog::getDouble(m_view, "Temperature", "Set Temperature", temp, 0.0,
                                   std::numeric_limits<double>::max(), 3, &ok);
    if (!ok)
      return;
  }
  m_model->setTempCorrection(on, temp);
  if (on)
    m_view->addTempCorrection(temp);
  else
    m_view->removeTempCorrection();

  setErrorsEnabled(false);
  updateViewParameterNames();
  updateViewParameters();
  m_view->emitFunctionStructureChanged();
}

void ConvTemplatePresenter::setNumberOfDatasets(int n) { m_model->setNumberDomains(n); }

int ConvTemplatePresenter::getNumberOfDatasets() const { return m_model->getNumberDomains(); }

void ConvTemplatePresenter::setFunction(std::string const &funStr) {
  m_model->setFunctionString(funStr);

  m_view->updateTemperatureCorrectionAndDelta(m_model->hasTempCorrection(), m_model->hasDeltaFunction());

  m_view->setSubType(SubTypeIndex::Lorentzian, static_cast<int>(m_model->getLorentzianType()));
  m_view->setSubType(SubTypeIndex::Fit, static_cast<int>(m_model->getFitType()));
  m_view->setSubType(SubTypeIndex::Background, static_cast<int>(m_model->getBackgroundType()));

  m_view->setInt(SubTypeIndex::Lorentzian, static_cast<int>(m_model->getLorentzianType()));
  m_view->setEnum(SubTypeIndex::Fit, static_cast<int>(m_model->getFitType()));
  m_view->setEnum(SubTypeIndex::Background, static_cast<int>(m_model->getBackgroundType()));

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

void ConvTemplatePresenter::updateMultiDatasetParameters(const ITableWorkspace &paramTable) {
  m_model->updateMultiDatasetParameters(paramTable);
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

void ConvTemplatePresenter::setQValues(const std::vector<double> &qValues) { m_model->setQValues(qValues); }

void ConvTemplatePresenter::setErrorsEnabled(bool enabled) { m_view->setErrorsEnabled(enabled); }

void ConvTemplatePresenter::setResolution(const std::vector<std::pair<std::string, size_t>> &fitResolutions) {
  m_model->setResolution(fitResolutions);
}

void ConvTemplatePresenter::updateViewParameters() {
  auto values = m_model->getCurrentValues();
  auto errors = m_model->getCurrentErrors();
  for (auto const id : values.keys()) {
    m_view->setParameterValueQuiet(id, values[id], errors[id]);
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

void ConvTemplatePresenter::updateViewParameterNames() { m_view->updateParameterNames(m_model->getParameterNameMap()); }

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

  m_editLocalParameterDialog =
      new EditLocalParameterDialog(m_view, parameterName, datasetNames, domainNames, values, fixes, ties, constraints);
  connect(m_editLocalParameterDialog, SIGNAL(finished(int)), this, SLOT(editLocalParameterFinish(int)));
  m_editLocalParameterDialog->open();
}

void ConvTemplatePresenter::editLocalParameterFinish(int result) {
  if (result == QDialog::Accepted) {
    auto parName = m_editLocalParameterDialog->getParameterName();
    auto values = m_editLocalParameterDialog->getValues();
    auto fixes = m_editLocalParameterDialog->getFixes();
    auto ties = m_editLocalParameterDialog->getTies();
    auto constraints = m_editLocalParameterDialog->getConstraints();
    assert(values.size() == getNumberOfDatasets());
    for (int i = 0; i < values.size(); ++i) {
      setLocalParameterValue(parName, i, values[i]);
      if (!ties[i].isEmpty()) {
        setLocalParameterTie(parName, i, ties[i].toStdString());
      } else if (fixes[i]) {
        setLocalParameterFixed(parName, i, fixes[i]);
      } else {
        setLocalParameterTie(parName, i, "");
      }
      m_model->setLocalParameterConstraint(parName, i, constraints[i].toStdString());
    }
  }
  m_editLocalParameterDialog = nullptr;
  updateViewParameters();
  m_view->emitFunctionStructureChanged();
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
  return m_model->getEstimationDataSelector();
}

void ConvTemplatePresenter::updateParameterEstimationData(DataForParameterEstimationCollection &&data) {
  m_model->updateParameterEstimationData(std::move(data));
}

void ConvTemplatePresenter::estimateFunctionParameters() {
  m_model->estimateFunctionParameters();
  updateViewParameters();
}

} // namespace MantidQt::CustomInterfaces::IDA
