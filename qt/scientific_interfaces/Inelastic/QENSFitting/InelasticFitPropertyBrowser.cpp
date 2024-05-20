// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "InelasticFitPropertyBrowser.h"
#include "FitStatusWidget.h"
#include "FittingPresenter.h"
#include "FunctionBrowser/FunctionTemplateView.h"
#include "FunctionBrowser/ITemplatePresenter.h"
#include "FunctionBrowser/SingleFunctionTemplateView.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmProperties.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/WorkspaceGroup.h"

#include "MantidQtWidgets/Common/FitOptionsBrowser.h"
#include "MantidQtWidgets/Common/FittingMode.h"
#include "MantidQtWidgets/Common/FunctionBrowser.h"

#include <QAction>
#include <QCheckBox>
#include <QFormLayout>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSignalMapper>
#include <QSplitter>
#include <QStackedWidget>
#include <QVBoxLayout>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

namespace MantidQt::CustomInterfaces::Inelastic {

struct ScopedSignalBlocker {
  // block signals on construction
  ScopedSignalBlocker(QWidget *myObject) : m_object(myObject) { m_object->blockSignals(true); }
  // enable signals on destruction
  ~ScopedSignalBlocker() { m_object->blockSignals(false); }
  QWidget *m_object;
};

/**
 * Constructor
 * @param parent :: The parent widget - must be an ApplicationWindow
 * @param mantidui :: The UI form for MantidPlot
 */
InelasticFitPropertyBrowser::InelasticFitPropertyBrowser(QWidget *parent)
    : QDockWidget(parent), m_mainLayout(), m_functionBrowser(), m_fitOptionsBrowser(), m_templatePresenter(),
      m_fitStatusWidget(), m_functionWidget(), m_browserSwitcher(), m_fitStatus(), m_fitChiSquared(), m_presenter() {
  setFeatures(QDockWidget::DockWidgetFloatable);
  setWindowTitle("Fit Function");
}

void InelasticFitPropertyBrowser::subscribePresenter(IFittingPresenter *presenter) { m_presenter = presenter; }

void InelasticFitPropertyBrowser::initFunctionBrowser() {
  // this object is added as a child to the stacked widget m_templateBrowser
  // which is a child of this class so the lifetime of this pointer is handled
  // by Qt
  m_functionBrowser = new FunctionBrowser(nullptr, true);
  m_functionBrowser->setObjectName("functionBrowser");
  // Process internally
  connect(m_functionBrowser, SIGNAL(globalsChanged()), this, SLOT(updateFitType()));
  // Re-emit
  connect(m_functionBrowser, SIGNAL(functionStructureChanged()), this, SLOT(notifyFunctionChanged()));
  connect(m_functionBrowser, SIGNAL(parameterChanged(std::string const &, std::string const &)), this,
          SLOT(notifyFunctionChanged()));
  connect(m_functionBrowser, SIGNAL(globalsChanged()), this, SLOT(notifyFunctionChanged()));
  connect(m_functionBrowser, SIGNAL(localParameterButtonClicked(std::string const &)), this,
          SIGNAL(localParameterEditRequested(std::string const &)));
}

void InelasticFitPropertyBrowser::initFitOptionsBrowser() {
  // this object is added as a child to the stacked widget m_templateBrowser
  // which is a child of this class so the lifetime of this pointer is handled
  // by Qt
  m_fitOptionsBrowser = new FitOptionsBrowser(nullptr, FittingMode::SEQUENTIAL_AND_SIMULTANEOUS);
  m_fitOptionsBrowser->setObjectName("fitOptionsBrowser");
  m_fitOptionsBrowser->setCurrentFittingType(FittingMode::SEQUENTIAL);
}

void InelasticFitPropertyBrowser::setHiddenProperties(const std::vector<std::string> &hiddenProperties) {
  for (const auto &propertyName : hiddenProperties) {
    m_fitOptionsBrowser->addPropertyToBlacklist(QString::fromStdString(propertyName));
  }
}

bool InelasticFitPropertyBrowser::isFullFunctionBrowserActive() const { return m_functionWidget->currentIndex() == 1; }

MultiDomainFunction_sptr InelasticFitPropertyBrowser::getGlobalFunction() const {
  auto fun =
      isFullFunctionBrowserActive() ? m_functionBrowser->getGlobalFunction() : m_templatePresenter->getGlobalFunction();
  if (auto temp = std::dynamic_pointer_cast<MultiDomainFunction>(fun)) {
    return temp;
  } else if (fun) {
    MultiDomainFunction_sptr multiFunction = std::make_shared<MultiDomainFunction>();
    multiFunction->addFunction(std::move(fun));
    multiFunction->setDomainIndex(0, 0);
    return multiFunction;
  } else {
    return nullptr;
  }
}

IFunction_sptr InelasticFitPropertyBrowser::getSingleFunction() const {
  return isFullFunctionBrowserActive() ? m_functionBrowser->getFunction() : m_templatePresenter->getFunction();
}

std::vector<std::string> InelasticFitPropertyBrowser::getGlobalParameters() const {
  return isFullFunctionBrowserActive() ? m_functionBrowser->getGlobalParameters()
                                       : m_templatePresenter->getGlobalParameters();
}

std::vector<std::string> InelasticFitPropertyBrowser::getLocalParameters() const {
  return isFullFunctionBrowserActive() ? m_functionBrowser->getLocalParameters()
                                       : m_templatePresenter->getLocalParameters();
}

void InelasticFitPropertyBrowser::syncFullBrowserWithTemplate() {
  m_functionBrowser->blockSignals(true);
  if (auto const fun = m_templatePresenter->getFunction()) {
    m_functionBrowser->setFunction(fun);
    m_functionBrowser->updateMultiDatasetParameters(*m_templatePresenter->getGlobalFunction());
    m_functionBrowser->setGlobalParameters(m_templatePresenter->getGlobalParameters());
    m_functionBrowser->setCurrentDataset(m_templatePresenter->getCurrentDataset());
  } else {
    m_functionBrowser->clear();
  }
  m_functionBrowser->blockSignals(false);
}

void InelasticFitPropertyBrowser::syncTemplateBrowserWithFull() {
  m_templatePresenter->browser()->blockSignals(true);
  auto const funStr = m_functionBrowser->getFunctionString();
  if (auto const fun = m_functionBrowser->getGlobalFunction()) {
    m_templatePresenter->setFunction(funStr);
    m_templatePresenter->updateMultiDatasetParameters(*fun);
    m_templatePresenter->setGlobalParameters(m_functionBrowser->getGlobalParameters());
    m_templatePresenter->setCurrentDataset(m_functionBrowser->getCurrentDataset());
  } else {
    m_templatePresenter->setFunction("");
  }
  m_templatePresenter->browser()->blockSignals(false);
}

void InelasticFitPropertyBrowser::init() {
  initFunctionBrowser();
  initFitOptionsBrowser();

  auto w = new QWidget(this);
  m_mainLayout = new QVBoxLayout(w);
  m_mainLayout->setContentsMargins(0, 0, 0, 0);

  m_functionWidget = new QStackedWidget(this);
  if (m_templatePresenter) {
    m_functionWidget->insertWidget(0, m_templatePresenter->browser());
    m_browserSwitcher = new QCheckBox("See full function");
    m_browserSwitcher->setObjectName("browserSwitcher");
    connect(m_browserSwitcher, SIGNAL(clicked(bool)), this, SLOT(showFullFunctionBrowser(bool)));
    m_fitStatusWidget = new FitStatusWidget(w);
    m_fitStatusWidget->setObjectName("browserFitStatus");
    m_fitStatusWidget->hide();
    m_mainLayout->insertWidget(0, m_fitStatusWidget);
    m_mainLayout->insertWidget(1, m_browserSwitcher);
  }
  m_functionWidget->addWidget(m_functionBrowser);
  auto splitter = new QSplitter(Qt::Vertical);
  m_mainLayout->addWidget(splitter);
  splitter->addWidget(m_functionWidget);
  splitter->addWidget(m_fitOptionsBrowser);
  w->setLayout(m_mainLayout);
  setWidget(w);
}

void InelasticFitPropertyBrowser::setFunctionTemplatePresenter(std::unique_ptr<ITemplatePresenter> templatePresenter) {
  if (m_templatePresenter) {
    throw std::logic_error("Template presenter already set.");
  }
  m_templatePresenter = std::move(templatePresenter);
  m_templatePresenter->init();
  connect(m_templatePresenter->browser(), SIGNAL(functionStructureChanged()), this, SLOT(notifyFunctionChanged()));
}

void InelasticFitPropertyBrowser::notifyFunctionChanged() {
  m_presenter->notifyFunctionChanged();
  emit functionChanged();
}

void InelasticFitPropertyBrowser::setFunction(std::string const &funStr) {
  if (isFullFunctionBrowserActive()) {
    m_functionBrowser->setFunction(funStr);
  } else {
    m_templatePresenter->setFunction(funStr);
  }
}

MultiDomainFunction_sptr InelasticFitPropertyBrowser::getFitFunction() const {
  try {
    if (getNumberOfDatasets() > 0) {
      return getGlobalFunction();
    } else {
      auto multiDomainFunction = std::make_shared<MultiDomainFunction>();
      auto singleFunction = getSingleFunction();
      if (singleFunction) {
        multiDomainFunction->addFunction(singleFunction);
        multiDomainFunction->setDomainIndex(0, 0);
      }
      return multiDomainFunction;
    }
  } catch (const std::invalid_argument &) {
    return std::make_shared<MultiDomainFunction>();
  } catch (const std::runtime_error &) {
    return std::make_shared<MultiDomainFunction>();
  }
}

QString InelasticFitPropertyBrowser::getSingleFunctionStr() const {
  return QString::fromStdString(getSingleFunction()->asString());
}

std::string InelasticFitPropertyBrowser::minimizer(bool) const {
  return m_fitOptionsBrowser->getProperty("Minimizer").toStdString();
}

int InelasticFitPropertyBrowser::maxIterations() const {
  return m_fitOptionsBrowser->getProperty("MaxIterations").toInt();
}

int InelasticFitPropertyBrowser::getPeakRadius() const {
  return m_fitOptionsBrowser->getProperty("PeakRadius").toInt();
}

std::string InelasticFitPropertyBrowser::costFunction() const {
  return m_fitOptionsBrowser->getProperty("CostFunction").toStdString();
}

bool InelasticFitPropertyBrowser::convolveMembers() const {
  return m_fitOptionsBrowser->getProperty("ConvolveMembers").toStdString() != "0";
}

bool InelasticFitPropertyBrowser::outputCompositeMembers() const {
  return m_fitOptionsBrowser->getProperty("OutputCompositeMembers").toStdString() != "0";
}

std::string InelasticFitPropertyBrowser::fitEvaluationType() const {
  return m_fitOptionsBrowser->getProperty("EvaluationType").toStdString();
}

bool InelasticFitPropertyBrowser::ignoreInvalidData() const {
  return m_fitOptionsBrowser->getProperty("IgnoreInvalidData").toStdString() != "0";
}

std::string InelasticFitPropertyBrowser::fitType() const {
  return m_fitOptionsBrowser->getProperty("FitType").toStdString();
}

std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>
InelasticFitPropertyBrowser::fitProperties(FittingMode const &fittingMode) const {
  auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  Mantid::API::AlgorithmProperties::update("Minimizer", minimizer(true), *properties);
  Mantid::API::AlgorithmProperties::update("MaxIterations", maxIterations(), *properties);
  Mantid::API::AlgorithmProperties::update("PeakRadius", getPeakRadius(), *properties);
  Mantid::API::AlgorithmProperties::update("CostFunction", costFunction(), *properties);
  Mantid::API::AlgorithmProperties::update("IgnoreInvalidData", ignoreInvalidData(), *properties);
  Mantid::API::AlgorithmProperties::update("EvaluationType", fitEvaluationType(), *properties);
  Mantid::API::AlgorithmProperties::update("PeakRadius", getPeakRadius(), *properties);
  Mantid::API::AlgorithmProperties::update("CostFunction", costFunction(), *properties);
  Mantid::API::AlgorithmProperties::update("ConvolveMembers", convolveMembers(), *properties);
  if (convolveMembers()) {
    Mantid::API::AlgorithmProperties::update("OutputCompositeMembers", true, *properties);
  } else {
    Mantid::API::AlgorithmProperties::update("OutputCompositeMembers", outputCompositeMembers(), *properties);
  }
  if (fittingMode == FittingMode::SEQUENTIAL) {
    Mantid::API::AlgorithmProperties::update("FitType", fitType(), *properties);
  }
  Mantid::API::AlgorithmProperties::update("OutputFitStatus", true, *properties);
  return properties;
}

int InelasticFitPropertyBrowser::getNumberOfDatasets() const {
  return isFullFunctionBrowserActive() ? m_functionBrowser->getNumberOfDatasets()
                                       : m_templatePresenter->getNumberOfDatasets();
}

void InelasticFitPropertyBrowser::updateParameters(const IFunction &fun) {
  this->blockSignals(true);
  if (isFullFunctionBrowserActive())
    m_functionBrowser->updateParameters(fun);
  else
    m_templatePresenter->updateParameters(fun);
  this->blockSignals(false);
}

void InelasticFitPropertyBrowser::updateFunctionListInBrowser(
    const std::map<std::string, std::string> &functionStrings) {
  m_templatePresenter->updateAvailableFunctions(functionStrings);
}

void InelasticFitPropertyBrowser::updateMultiDatasetParameters(const IFunction &fun) {
  this->blockSignals(true);
  if (isFullFunctionBrowserActive())
    m_functionBrowser->updateMultiDatasetParameters(fun);
  else
    m_templatePresenter->updateMultiDatasetParameters(fun);
  this->blockSignals(false);
}

void InelasticFitPropertyBrowser::updateMultiDatasetParameters(const ITableWorkspace &paramTable) {
  this->blockSignals(true);
  if (isFullFunctionBrowserActive())
    m_functionBrowser->updateMultiDatasetParameters(paramTable);
  else
    m_templatePresenter->updateMultiDatasetParameters(paramTable);
  this->blockSignals(false);
}

void InelasticFitPropertyBrowser::updateFitStatusData(const std::vector<std::string> &status,
                                                      const std::vector<double> &chiSquared) {
  m_fitStatus = status;
  m_fitChiSquared = chiSquared;
  updateFitStatus(currentDataset());
}

void InelasticFitPropertyBrowser::updateFitStatus(const FitDomainIndex index) {
  if (index.value + 1 > m_fitStatus.size() || index.value + 1 > m_fitChiSquared.size()) {
    return;
  }
  m_fitStatusWidget->update(m_fitStatus[index.value], m_fitChiSquared[index.value]);
  return;
}

/**
 * @return  The currently active fitting mode (Sequential or Simultaneous).
 */
FittingMode InelasticFitPropertyBrowser::getFittingMode() const { return m_fitOptionsBrowser->getCurrentFittingType(); }

/**
 * Sets whether fit members should be convolved with the resolution after a fit.
 *
 * @param convolveEnabled: If true, members are to be convolved.
 */
void InelasticFitPropertyBrowser::setConvolveMembers(bool convolveEnabled) {
  m_fitOptionsBrowser->setProperty("ConvolveMembers", QString(convolveEnabled ? "1" : "0"));
}

/**
 * Sets whether to output fit members
 *
 * @param outputEnabled: If true, fit members are outputted
 */
void InelasticFitPropertyBrowser::setOutputCompositeMembers(bool outputEnabled) {
  m_fitOptionsBrowser->setProperty("OutputCompositeMembers", QString(outputEnabled ? "1" : "0"));
}

/**
 * Clears the functions in this indirect fit property browser.
 */
void InelasticFitPropertyBrowser::clear() {
  m_functionBrowser->clear();
  m_templatePresenter->browser()->clear();
}

/**
 * Updates the plot guess feature in this indirect fit property browser.
 * @param sampleWorkspace :: The workspace loaded as sample
 */
void InelasticFitPropertyBrowser::updatePlotGuess(const MatrixWorkspace_const_sptr &) {}

void InelasticFitPropertyBrowser::setErrorsEnabled(bool enabled) {
  m_functionBrowser->setErrorsEnabled(enabled);
  m_templatePresenter->setErrorsEnabled(enabled);
}

EstimationDataSelector InelasticFitPropertyBrowser::getEstimationDataSelector() const {
  return m_templatePresenter->getEstimationDataSelector();
}

void InelasticFitPropertyBrowser::updateParameterEstimationData(DataForParameterEstimationCollection &&data) {
  m_templatePresenter->updateParameterEstimationData(std::move(data));
}

void InelasticFitPropertyBrowser::estimateFunctionParameters() { m_templatePresenter->estimateFunctionParameters(); }

void InelasticFitPropertyBrowser::setBackgroundA0(double value) {
  if (isFullFunctionBrowserActive()) {
    m_functionBrowser->setBackgroundA0(value);
  } else {
    m_templatePresenter->setBackgroundA0(value);
  }
}

void InelasticFitPropertyBrowser::setCurrentDataset(FitDomainIndex i) {
  if (getNumberOfDatasets() == 0)
    return;
  updateFitStatus(i);
  if (isFullFunctionBrowserActive()) {
    m_functionBrowser->setCurrentDataset(static_cast<int>(i.value));
  } else {
    m_templatePresenter->setCurrentDataset(static_cast<int>(i.value));
  }
}

FitDomainIndex InelasticFitPropertyBrowser::currentDataset() const {
  if (isFullFunctionBrowserActive()) {
    return FitDomainIndex{static_cast<size_t>(m_functionBrowser->getCurrentDataset())};
  } else {
    return FitDomainIndex{static_cast<size_t>(m_templatePresenter->getCurrentDataset())};
  }
}

void InelasticFitPropertyBrowser::updateFunctionBrowserData(
    int nData, const QList<FunctionModelDataset> &datasets, const std::vector<double> &qValues,
    const std::vector<std::pair<std::string, size_t>> &fitResolutions) {
  m_functionBrowser->setNumberOfDatasets(nData);
  m_functionBrowser->setDatasets(datasets);
  m_templatePresenter->setNumberOfDatasets(nData);
  m_templatePresenter->setDatasets(datasets);
  m_templatePresenter->setQValues(qValues);
  m_templatePresenter->setResolution(fitResolutions);
}

void InelasticFitPropertyBrowser::setFitEnabled(bool) {}

/**
 * Schedules a fit.
 */
void InelasticFitPropertyBrowser::fit() { emit fitScheduled(); }

/**
 * Schedules a sequential fit.
 */
void InelasticFitPropertyBrowser::sequentialFit() { emit sequentialFitScheduled(); }

void InelasticFitPropertyBrowser::setModelResolution(
    const std::vector<std::pair<std::string, size_t>> &fitResolutions) {
  if (isFullFunctionBrowserActive()) {
    showFullFunctionBrowser(false);
  }
  m_templatePresenter->setResolution(fitResolutions);
}

/**
 * Called when the browser visibility has changed.
 *
 * @param isVisible True if the browser is visible, false otherwise.
 */
void InelasticFitPropertyBrowser::browserVisibilityChanged(bool isVisible) {
  if (!isVisible)
    emit browserClosed();
}

void InelasticFitPropertyBrowser::updateFitType() {
  auto const nGlobals = m_functionBrowser->getGlobalParameters().size();
  if (nGlobals == 0) {
    m_fitOptionsBrowser->setCurrentFittingType(FittingMode::SEQUENTIAL);
  } else {
    m_fitOptionsBrowser->setCurrentFittingType(FittingMode::SIMULTANEOUS);
  }
}

void InelasticFitPropertyBrowser::showFullFunctionBrowser(bool on) {
  if (on) {
    syncFullBrowserWithTemplate();
  } else {
    try {
      syncTemplateBrowserWithFull();
    } catch (const std::runtime_error &) {
      // Function doesn't match the template.
      // Stay with generic function browser.
      on = true;
      m_browserSwitcher->blockSignals(true);
      m_browserSwitcher->setChecked(true);
      m_browserSwitcher->blockSignals(false);
    }
  }
  auto const index = on ? 1 : 0;
  m_functionWidget->setCurrentIndex(index);
}

} // namespace MantidQt::CustomInterfaces::Inelastic
