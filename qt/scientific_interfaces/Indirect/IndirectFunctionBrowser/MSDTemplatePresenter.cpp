// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MSDTemplatePresenter.h"
#include "MSDTemplateBrowser.h"
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
MSDTemplatePresenter::MSDTemplatePresenter(MSDTemplateBrowser *view)
    : QObject(view), m_view(view) {
  connect(m_view, SIGNAL(localParameterButtonClicked(const QString &)), this,
          SLOT(editLocalParameter(const QString &)));
  connect(m_view, SIGNAL(parameterValueChanged(const QString &, double)), this,
          SLOT(viewChangedParameterValue(const QString &, double)));
}

void MSDTemplatePresenter::setFitType(const QString &name) {
  m_view->clear();
  m_model.removeFitType();

  if (name == "None") {
    // do nothing
  } else if (name == "Gaussian") {
    m_view->addGaussian();
    m_model.setFitType("MsdGauss");
  } else if (name == "Peters") {
    m_view->addPeters();
    m_model.setFitType("MsdPeters");
  } else if (name == "Yi") {
    m_view->addYi();
    m_model.setFitType("MsdYi");
  } else {
    throw std::logic_error("Browser doesn't support fit type " +
                           name.toStdString());
  }
  setErrorsEnabled(false);
  updateView();
  emit functionStructureChanged();
}

void MSDTemplatePresenter::setNumberOfDatasets(int n) {
  m_model.setNumberDomains(n);
}

int MSDTemplatePresenter::getNumberOfDatasets() const {
  return m_model.getNumberDomains();
}

int MSDTemplatePresenter::getCurrentDataset() {
  return m_model.currentDomainIndex();
}

void MSDTemplatePresenter::setFunction(const QString &funStr) {
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

IFunction_sptr MSDTemplatePresenter::getGlobalFunction() const {
  return m_model.getFitFunction();
}

IFunction_sptr MSDTemplatePresenter::getFunction() const {
  return m_model.getCurrentFunction();
}

QStringList MSDTemplatePresenter::getGlobalParameters() const {
  return m_model.getGlobalParameters();
}

QStringList MSDTemplatePresenter::getLocalParameters() const {
  return m_model.getLocalParameters();
}

void MSDTemplatePresenter::setGlobalParameters(const QStringList &globals) {
  m_model.setGlobalParameters(globals);
  m_view->setGlobalParametersQuiet(globals);
}

void MSDTemplatePresenter::setGlobal(const QString &parName, bool on) {
  m_model.setGlobal(parName, on);
  m_view->setGlobalParametersQuiet(m_model.getGlobalParameters());
}

void MSDTemplatePresenter::updateMultiDatasetParameters(const IFunction &fun) {
  m_model.updateMultiDatasetParameters(fun);
  updateViewParameters();
}

void MSDTemplatePresenter::updateMultiDatasetParameters(
    const ITableWorkspace &paramTable) {
  m_model.updateMultiDatasetParameters(paramTable);
  updateViewParameters();
}

void MSDTemplatePresenter::updateParameters(const IFunction &fun) {
  m_model.updateParameters(fun);
  updateViewParameters();
}

void MSDTemplatePresenter::setCurrentDataset(int i) {
  m_model.setCurrentDomainIndex(i);
  updateViewParameters();
}

void MSDTemplatePresenter::setDatasetNames(const QStringList &names) {
  m_model.setDatasetNames(names);
}

void MSDTemplatePresenter::setViewParameterDescriptions() {
  m_view->updateParameterDescriptions(m_model.getParameterDescriptionMap());
}

void MSDTemplatePresenter::setErrorsEnabled(bool enabled) {
  m_view->setErrorsEnabled(enabled);
}

void MSDTemplatePresenter::updateParameterEstimationData(
    DataForParameterEstimationCollection &&data) {
  m_model.updateParameterEstimationData(std::move(data));
}

void MSDTemplatePresenter::updateViewParameters() {
  static std::map<MSDFunctionModel::ParamID,
                  void (MSDTemplateBrowser::*)(double, double)>
      setters{
          {MSDFunctionModel::ParamID::GAUSSIAN_HEIGHT,
           &MSDTemplateBrowser::setGaussianHeight},
          {MSDFunctionModel::ParamID::GAUSSIAN_MSD,
           &MSDTemplateBrowser::setGaussianMsd},
          {MSDFunctionModel::ParamID::PETERS_HEIGHT,
           &MSDTemplateBrowser::setPetersHeight},
          {MSDFunctionModel::ParamID::PETERS_MSD,
           &MSDTemplateBrowser::setPetersMsd},
          {MSDFunctionModel::ParamID::PETERS_BETA,
           &MSDTemplateBrowser::setPetersBeta},
          {MSDFunctionModel::ParamID::YI_HEIGHT,
           &MSDTemplateBrowser::setYiHeight},
          {MSDFunctionModel::ParamID::YI_MSD, &MSDTemplateBrowser::setYiMsd},
          {MSDFunctionModel::ParamID::YI_SIGMA,
           &MSDTemplateBrowser::setYiSigma}};
  const auto values = m_model.getCurrentValues();
  const auto errors = m_model.getCurrentErrors();
  for (auto const &name : values.keys()) {
    (m_view->*setters.at(name))(values[name], errors[name]);
  }
}

QStringList MSDTemplatePresenter::getDatasetNames() const {
  return m_model.getDatasetNames();
}

double MSDTemplatePresenter::getLocalParameterValue(const QString &parName,
                                                    int i) const {
  return m_model.getLocalParameterValue(parName, i);
}

bool MSDTemplatePresenter::isLocalParameterFixed(const QString &parName,
                                                 int i) const {
  return m_model.isLocalParameterFixed(parName, i);
}

QString MSDTemplatePresenter::getLocalParameterTie(const QString &parName,
                                                   int i) const {
  return m_model.getLocalParameterTie(parName, i);
}

QString
MSDTemplatePresenter::getLocalParameterConstraint(const QString &parName,
                                                  int i) const {
  return m_model.getLocalParameterConstraint(parName, i);
}

void MSDTemplatePresenter::setLocalParameterValue(const QString &parName, int i,
                                                  double value) {
  m_model.setLocalParameterValue(parName, i, value);
}

void MSDTemplatePresenter::setLocalParameterTie(const QString &parName, int i,
                                                const QString &tie) {
  m_model.setLocalParameterTie(parName, i, tie);
}

void MSDTemplatePresenter::updateViewParameterNames() {
  m_view->updateParameterNames(m_model.getParameterNameMap());
}

void MSDTemplatePresenter::updateView() {
  updateViewParameterNames();
  updateViewParameters();
}

void MSDTemplatePresenter::setLocalParameterFixed(const QString &parName, int i,
                                                  bool fixed) {
  m_model.setLocalParameterFixed(parName, i, fixed);
}

void MSDTemplatePresenter::editLocalParameter(const QString &parName) {
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

void MSDTemplatePresenter::editLocalParameterFinish(int result) {
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

void MSDTemplatePresenter::viewChangedParameterValue(const QString &parName,
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
