// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "FQTemplatePresenter.h"
#include "FQTemplateBrowser.h"
#include "MantidQtWidgets/Common/EditLocalParameterDialog.h"
#include <math.h>

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

using namespace MantidWidgets;

/**
 * Constructor
 * @param parent :: The parent widget.
 */
FQTemplatePresenter::FQTemplatePresenter(FQTemplateBrowser *view)
    : QObject(view), m_view(view) {
  connect(m_view, SIGNAL(localParameterButtonClicked(const QString &)), this,
      SLOT(editLocalParameter(const QString &)));
  connect(m_view, SIGNAL(parameterValueChanged(const QString &, double)), this,
      SLOT(viewChangedParameterValue(const QString &, double)));
}

void FQTemplatePresenter::setFitType(const QString &name) {
  m_view->clear();
  m_model.removeFitType();

  if (name == "None") {
    // do nothing
  } else if (name == "Gaussian") {
    m_view->addGaussian();
    m_model.setFitType("FQGauss");
  } else if (name == "Peters") {
    m_view->addPeters();
    m_model.setFitType("FQPeters");
  } else if (name == "Yi") {
    m_view->addYi();
    m_model.setFitType("FQYi");
  } else {
    throw std::logic_error("Browser doesn't support fit type " +
                           name.toStdString());
  }
  setErrorsEnabled(false);
  updateView();
  emit functionStructureChanged();
}

void FQTemplatePresenter::setNumberOfDatasets(int n) {
  m_model.setNumberDomains(n);
}

int FQTemplatePresenter::getNumberOfDatasets() const {
  return m_model.getNumberDomains();
}

int FQTemplatePresenter::getCurrentDataset() {
  return m_model.currentDomainIndex();
}

void FQTemplatePresenter::setFunction(const QString &funStr) {
  m_model.setFunctionString(funStr);
  m_view->clear();
  setErrorsEnabled(false);
  if (m_model.hasGaussianType()) {
    m_view->addGaussian();
  }
  if (m_model.hasPetersType()) {
    m_view->addPeters();
  }
  if (m_model.hasYiType()) {
    m_view->addYi();
  }
  updateViewParameterNames();
  updateViewParameters();
  emit functionStructureChanged();
}

IFunction_sptr FQTemplatePresenter::getGlobalFunction() const {
  return m_model.getFitFunction();
}

IFunction_sptr FQTemplatePresenter::getFunction() const {
  return m_model.getCurrentFunction();
}

QStringList FQTemplatePresenter::getGlobalParameters() const {
  return m_model.getGlobalParameters();
}

QStringList FQTemplatePresenter::getLocalParameters() const {
  return m_model.getLocalParameters();
}

void FQTemplatePresenter::setGlobalParameters(const QStringList &globals) {
  m_model.setGlobalParameters(globals);
  m_view->setGlobalParametersQuiet(globals);
}

void FQTemplatePresenter::setGlobal(const QString &parName, bool on) {
  m_model.setGlobal(parName, on);
  m_view->setGlobalParametersQuiet(m_model.getGlobalParameters());
}

void FQTemplatePresenter::updateMultiDatasetParameters(const IFunction &fun) {
  m_model.updateMultiDatasetParameters(fun);
  updateViewParameters();
}

void FQTemplatePresenter::updateMultiDatasetParameters(
    const ITableWorkspace &paramTable) {
  m_model.updateMultiDatasetParameters(paramTable);
  updateViewParameters();
}

void FQTemplatePresenter::updateParameters(const IFunction &fun) {
  m_model.updateParameters(fun);
  updateViewParameters();
}

void FQTemplatePresenter::setCurrentDataset(int i) {
  m_model.setCurrentDomainIndex(i);
  updateViewParameters();
}

void FQTemplatePresenter::setDatasetNames(const QStringList &names) {
  m_model.setDatasetNames(names);
}

void FQTemplatePresenter::setViewParameterDescriptions() {
  m_view->updateParameterDescriptions(m_model.getParameterDescriptionMap());
}

void FQTemplatePresenter::setErrorsEnabled(bool enabled) {
  m_view->setErrorsEnabled(enabled);
}

void FQTemplatePresenter::updateParameterEstimationData(
    DataForParameterEstimationCollection &&data) {
  m_model.updateParameterEstimationData(std::move(data));
}

void FQTemplatePresenter::updateViewParameters() {
  static std::map<FQFunctionModel::ParamID,
      void (FQTemplateBrowser::*)(double, double)>
      setters{
      {FQFunctionModel::ParamID::GAUSSIAN_HEIGHT,
                                          &FQTemplateBrowser::setGaussianHeight},
      {FQFunctionModel::ParamID::GAUSSIAN_FQ,
                                          &FQTemplateBrowser::setGaussianFQ},
      {FQFunctionModel::ParamID::PETERS_HEIGHT,
                                          &FQTemplateBrowser::setPetersHeight},
      {FQFunctionModel::ParamID::PETERS_FQ,
                                          &FQTemplateBrowser::setPetersFQ},
      {FQFunctionModel::ParamID::PETERS_BETA,
                                          &FQTemplateBrowser::setPetersBeta},
      {FQFunctionModel::ParamID::YI_HEIGHT,
                                          &FQTemplateBrowser::setYiHeight},
      {FQFunctionModel::ParamID::YI_FQ, &FQTemplateBrowser::setYiFQ},
      {FQFunctionModel::ParamID::YI_SIGMA,
                                          &FQTemplateBrowser::setYiSigma}};
  const auto values = m_model.getCurrentValues();
  const auto errors = m_model.getCurrentErrors();
  for (auto const &name : values.keys()) {
    (m_view->*setters.at(name))(values[name], errors[name]);
  }
}

QStringList FQTemplatePresenter::getDatasetNames() const {
  return m_model.getDatasetNames();
}

double FQTemplatePresenter::getLocalParameterValue(const QString &parName,
                                                    int i) const {
  return m_model.getLocalParameterValue(parName, i);
}

bool FQTemplatePresenter::isLocalParameterFixed(const QString &parName,
                                                 int i) const {
  return m_model.isLocalParameterFixed(parName, i);
}

QString FQTemplatePresenter::getLocalParameterTie(const QString &parName,
                                                   int i) const {
  return m_model.getLocalParameterTie(parName, i);
}

QString
FQTemplatePresenter::getLocalParameterConstraint(const QString &parName,
                                                  int i) const {
  return m_model.getLocalParameterConstraint(parName, i);
}

void FQTemplatePresenter::setLocalParameterValue(const QString &parName, int i,
                                                  double value) {
  m_model.setLocalParameterValue(parName, i, value);
}

void FQTemplatePresenter::setLocalParameterTie(const QString &parName, int i,
                                                const QString &tie) {
  m_model.setLocalParameterTie(parName, i, tie);
}

void FQTemplatePresenter::updateViewParameterNames() {
  m_view->updateParameterNames(m_model.getParameterNameMap());
}

void FQTemplatePresenter::updateView() {
  updateViewParameterNames();
  updateViewParameters();
}

void FQTemplatePresenter::setLocalParameterFixed(const QString &parName, int i,
                                                  bool fixed) {
  m_model.setLocalParameterFixed(parName, i, fixed);
}

void FQTemplatePresenter::editLocalParameter(const QString &parName) {
  auto const wsNames = getDatasetNames();
  QList<double> values;
  QList<bool> fixes;
  QStringList ties;
  QStringList constraints;
  const int n = wsNames.size();
  for (auto i = 0; i < n; ++i) {
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

void FQTemplatePresenter::editLocalParameterFinish(int result) {
  if (result == QDialog::Accepted) {
    const auto parName = m_editLocalParameterDialog->getParameterName();
    const auto values = m_editLocalParameterDialog->getValues();
    const auto fixes = m_editLocalParameterDialog->getFixes();
    const auto ties = m_editLocalParameterDialog->getTies();
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

void FQTemplatePresenter::viewChangedParameterValue(const QString &parName,
                                                     double value) {
  if (parName.isEmpty())
    return;
  if (m_model.isGlobal(parName)) {
    const auto n = getNumberOfDatasets();
    for (int i = 0; i < n; ++i) {
      setLocalParameterValue(parName, i, value);
    }
  } else {
    const auto i = m_model.currentDomainIndex();
    const auto oldValue = m_model.getLocalParameterValue(parName, i);
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