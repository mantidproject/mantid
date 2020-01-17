// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ConvTemplatePresenter.h"
#include "ConvTemplateBrowser.h"
#include "MantidQtWidgets/Common/EditLocalParameterDialog.h"

#include <cmath>
#include <iostream>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

using namespace MantidWidgets;

/**
 * Constructor
 * @param parent :: The parent widget.
 */
ConvTemplatePresenter::ConvTemplatePresenter(ConvTemplateBrowser *view)
    : QObject(view), m_view(view) {
  connect(m_view, SIGNAL(localParameterButtonClicked(const QString &)), this,
          SLOT(editLocalParameter(const QString &)));
  connect(m_view, SIGNAL(parameterValueChanged(const QString &, double)), this,
          SLOT(viewChangedParameterValue(const QString &, double)));
}

void ConvTemplatePresenter::setSubType(size_t subTypeIndex, int typeIndex) {
  if (subTypeIndex == 0) {
    m_model.setFitType(static_cast<FitType>(typeIndex));
  } else {
    m_model.setBackground(static_cast<BackgroundType>(typeIndex));
  }
  m_view->setSubType(subTypeIndex, typeIndex);
  setErrorsEnabled(false);
  updateViewParameterNames();
  updateViewParameters();
  emit functionStructureChanged();
}

void ConvTemplatePresenter::setDeltaFunction(bool on) {
  if (on == m_model.hasDeltaFunction())
    return;
  m_model.setDeltaFunction(on);
  setErrorsEnabled(false);
  updateViewParameterNames();
  updateViewParameters();
  emit functionStructureChanged();
}

void ConvTemplatePresenter::setNumberOfDatasets(int n) {
  m_model.setNumberDomains(n);
}

int ConvTemplatePresenter::getNumberOfDatasets() const {
  return m_model.getNumberDomains();
}

void ConvTemplatePresenter::setFunction(const QString &funStr) {
  m_model.setFunctionString(funStr);
  m_view->clear();
  setErrorsEnabled(false);
  // if (m_model.hasBackground()) {
  //  m_view->addFlatBackground();
  //}
  // if (m_model.hasStretchExponential()) {
  //  m_view->addStretchExponential();
  //}
  // auto const nExp = m_model.getNumberOfExponentials();
  // if (nExp > 0) {
  //  m_view->addExponentialOne();
  //}
  // if (nExp > 1) {
  //  m_view->addExponentialTwo();
  //}
  updateViewParameterNames();
  updateViewParameters();
  emit functionStructureChanged();
}

int ConvTemplatePresenter::getCurrentDataset() {
  return m_model.currentDomainIndex();
}

IFunction_sptr ConvTemplatePresenter::getGlobalFunction() const {
  return m_model.getFitFunction();
}

IFunction_sptr ConvTemplatePresenter::getFunction() const {
  return m_model.getCurrentFunction();
}

QStringList ConvTemplatePresenter::getGlobalParameters() const {
  return m_model.getGlobalParameters();
}

QStringList ConvTemplatePresenter::getLocalParameters() const {
  return m_model.getLocalParameters();
}

void ConvTemplatePresenter::setGlobalParameters(const QStringList &globals) {
  m_model.setGlobalParameters(globals);
  // if (m_model.hasStretchExponential()) {
  //  m_view->setGlobalParametersQuiet(globals);
  //}
}

void ConvTemplatePresenter::setGlobal(const QString &parName, bool on) {
  auto globals = m_model.getGlobalParameters();
  if (on) {
    if (!globals.contains(parName)) {
      globals.push_back(parName);
    }
  } else if (globals.contains(parName)) {
    globals.removeOne(parName);
  }
  setGlobalParameters(globals);
}

void ConvTemplatePresenter::updateMultiDatasetParameters(const IFunction &fun) {
  m_model.updateMultiDatasetParameters(fun);
  updateViewParameters();
}

void ConvTemplatePresenter::updateMultiDatasetParameters(
    const ITableWorkspace &paramTable) {
  m_model.updateMultiDatasetParameters(paramTable);
  updateViewParameters();
}

void ConvTemplatePresenter::updateParameters(const IFunction &fun) {
  m_model.updateParameters(fun);
  updateViewParameters();
}

void ConvTemplatePresenter::setCurrentDataset(int i) {
  m_model.setCurrentDomainIndex(i);
  updateViewParameters();
}

void ConvTemplatePresenter::setDatasetNames(const QStringList &names) {
  m_model.setDatasetNames(names);
}

void ConvTemplatePresenter::setBackgroundA0(double value) {
  m_model.setBackgroundA0(value);
  updateViewParameters();
};

void ConvTemplatePresenter::setErrorsEnabled(bool enabled) {
  m_view->setErrorsEnabled(enabled);
}

void ConvTemplatePresenter::setResolution(std::string const &name,
                                          TableDatasetIndex const &index) {
  m_model.setResolution(name, index);
}

void ConvTemplatePresenter::setResolution(
    const std::vector<std::pair<std::string, int>> &fitResolutions) {
  m_model.setResolution(fitResolutions);
}

void ConvTemplatePresenter::updateViewParameters() {
  auto values = m_model.getCurrentValues();
  auto errors = m_model.getCurrentErrors();
  for (auto const id : values.keys()) {
    m_view->setParameterValueQuiet(id, values[id], errors[id]);
  }
}

QStringList ConvTemplatePresenter::getDatasetNames() const {
  return m_model.getDatasetNames();
}

double ConvTemplatePresenter::getLocalParameterValue(const QString &parName,
                                                     int i) const {
  return m_model.getLocalParameterValue(parName, i);
}

bool ConvTemplatePresenter::isLocalParameterFixed(const QString &parName,
                                                  int i) const {
  return m_model.isLocalParameterFixed(parName, i);
}

QString ConvTemplatePresenter::getLocalParameterTie(const QString &parName,
                                                    int i) const {
  return m_model.getLocalParameterTie(parName, i);
}

QString
ConvTemplatePresenter::getLocalParameterConstraint(const QString &parName,
                                                   int i) const {
  return m_model.getLocalParameterConstraint(parName, i);
}

void ConvTemplatePresenter::setLocalParameterValue(const QString &parName,
                                                   int i, double value) {
  m_model.setLocalParameterValue(parName, i, value);
}

void ConvTemplatePresenter::setLocalParameterTie(const QString &parName, int i,
                                                 const QString &tie) {
  m_model.setLocalParameterTie(parName, i, tie);
}

void ConvTemplatePresenter::updateViewParameterNames() {
  m_view->updateParameterNames(m_model.getParameterNameMap());
}

void ConvTemplatePresenter::setLocalParameterFixed(const QString &parName,
                                                   int i, bool fixed) {
  m_model.setLocalParameterFixed(parName, i, fixed);
}

void ConvTemplatePresenter::editLocalParameter(const QString &parName) {
  auto const wsNames = getDatasetNames();
  QList<double> values;
  QList<bool> fixes;
  QStringList ties;
  QStringList constraints;
  const int n = wsNames.size();
  for (int i = 0; i < n; ++i) {
    const double value = getLocalParameterValue(parName, i);
    values.push_back(value);
    const bool fixed = isLocalParameterFixed(parName, i);
    fixes.push_back(fixed);
    const auto tie = getLocalParameterTie(parName, i);
    ties.push_back(tie);
    const auto constraint = getLocalParameterConstraint(parName, i);
    constraints.push_back(constraint);
  }

  m_editLocalParameterDialog = new EditLocalParameterDialog(
      m_view, parName, wsNames, values, fixes, ties, constraints);
  connect(m_editLocalParameterDialog, SIGNAL(finished(int)), this,
          SLOT(editLocalParameterFinish(int)));
  m_editLocalParameterDialog->open();
}

void ConvTemplatePresenter::editLocalParameterFinish(int result) {
  if (result == QDialog::Accepted) {
    auto parName = m_editLocalParameterDialog->getParameterName();
    auto values = m_editLocalParameterDialog->getValues();
    auto fixes = m_editLocalParameterDialog->getFixes();
    auto ties = m_editLocalParameterDialog->getTies();
    assert(values.size() == getNumberOfDatasets());
    for (int i = 0; i < values.size(); ++i) {
      setLocalParameterValue(parName, i, values[i]);
      if (!ties[i].isEmpty()) {
        setLocalParameterTie(parName, i, ties[i]);
      } else if (fixes[i]) {
        setLocalParameterFixed(parName, i, fixes[i]);
      } else {
        setLocalParameterTie(parName, i, "");
      }
    }
  }
  m_editLocalParameterDialog = nullptr;
  updateViewParameters();
  emit functionStructureChanged();
}

void ConvTemplatePresenter::viewChangedParameterValue(const QString &parName,
                                                      double value) {
  if (parName.isEmpty())
    return;
  if (m_model.isGlobal(parName)) {
    auto const n = getNumberOfDatasets();
    for (int i = 0; i < n; ++i) {
      setLocalParameterValue(parName, i, value);
    }
  } else {
    auto const i = m_model.currentDomainIndex();
    auto const oldValue = m_model.getLocalParameterValue(parName, i);
    if (fabs(value - oldValue) > 1e-6) {
      setErrorsEnabled(false);
    }
    setLocalParameterValue(parName, i, value);
  }
  emit functionStructureChanged();
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
