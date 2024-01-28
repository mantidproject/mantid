// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitPropertyBrowser.h"
#include "FitStatusWidget.h"
#include "FunctionBrowser/ITemplatePresenter.h"
#include "FunctionBrowser/SingleFunctionTemplateBrowser.h"
#include "FunctionTemplateBrowser.h"

#include "MantidAPI/AlgorithmManager.h"
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

namespace MantidQt::CustomInterfaces::IDA {

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
IndirectFitPropertyBrowser::IndirectFitPropertyBrowser(QWidget *parent)
    : QDockWidget(parent), m_templatePresenter(nullptr), m_functionWidget(nullptr) {
  setFeatures(QDockWidget::DockWidgetFloatable);
  setWindowTitle("Fit Function");
}

void IndirectFitPropertyBrowser::initFunctionBrowser() {
  // this object is added as a child to the stacked widget m_templateBrowser
  // which is a child of this class so the lifetime of this pointer is handled
  // by Qt
  m_functionBrowser = new FunctionBrowser(nullptr, true);
  m_functionBrowser->setObjectName("functionBrowser");
  // Process internally
  connect(m_functionBrowser, SIGNAL(globalsChanged()), this, SLOT(updateFitType()));
  // Re-emit
  connect(m_functionBrowser, SIGNAL(functionStructureChanged()), this, SIGNAL(functionChanged()));
  connect(m_functionBrowser, SIGNAL(parameterChanged(std::string const &, std::string const &)), this,
          SIGNAL(functionChanged()));
  connect(m_functionBrowser, SIGNAL(globalsChanged()), this, SIGNAL(functionChanged()));
  connect(m_functionBrowser, SIGNAL(localParameterButtonClicked(std::string const &)), this,
          SIGNAL(localParameterEditRequested(std::string const &)));
}

void IndirectFitPropertyBrowser::initFitOptionsBrowser() {
  // this object is added as a child to the stacked widget m_templateBrowser
  // which is a child of this class so the lifetime of this pointer is handled
  // by Qt
  m_fitOptionsBrowser = new FitOptionsBrowser(nullptr, FittingMode::SEQUENTIAL_AND_SIMULTANEOUS);
  m_fitOptionsBrowser->setObjectName("fitOptionsBrowser");
  m_fitOptionsBrowser->setCurrentFittingType(FittingMode::SEQUENTIAL);
}

void IndirectFitPropertyBrowser::setHiddenProperties(const std::vector<std::string> &hiddenProperties) {
  for (const auto &propertyName : hiddenProperties) {
    m_fitOptionsBrowser->addPropertyToBlacklist(QString::fromStdString(propertyName));
  }
}

bool IndirectFitPropertyBrowser::isFullFunctionBrowserActive() const { return m_functionWidget->currentIndex() == 1; }

MultiDomainFunction_sptr IndirectFitPropertyBrowser::getGlobalFunction() const {
  auto fun =
      isFullFunctionBrowserActive() ? m_functionBrowser->getGlobalFunction() : m_templatePresenter->getGlobalFunction();
  if (auto temp = std::dynamic_pointer_cast<MultiDomainFunction>(fun)) {
    return temp;
  } else if (fun) {
    MultiDomainFunction_sptr multiFunction = std::make_shared<MultiDomainFunction>();
    multiFunction->addFunction(fun);
    multiFunction->setDomainIndex(0, 0);
    return multiFunction;
  } else {
    return nullptr;
  }
}

IFunction_sptr IndirectFitPropertyBrowser::getSingleFunction() const {
  return isFullFunctionBrowserActive() ? m_functionBrowser->getFunction() : m_templatePresenter->getFunction();
}

std::vector<std::string> IndirectFitPropertyBrowser::getGlobalParameters() const {
  return isFullFunctionBrowserActive() ? m_functionBrowser->getGlobalParameters()
                                       : m_templatePresenter->getGlobalParameters();
}

std::vector<std::string> IndirectFitPropertyBrowser::getLocalParameters() const {
  return isFullFunctionBrowserActive() ? m_functionBrowser->getLocalParameters()
                                       : m_templatePresenter->getLocalParameters();
}

void IndirectFitPropertyBrowser::syncFullBrowserWithTemplate() {
  m_functionBrowser->blockSignals(true);
  if (auto const fun = m_templatePresenter->getFunction()) {
    m_functionBrowser->setFunction(fun);
    m_functionBrowser->updateMultiDatasetParameters(*m_templatePresenter->getGlobalFunction());
    m_functionBrowser->setGlobalParameters(m_templatePresenter->getGlobalParameters());
    m_functionBrowser->setCurrentDataset(m_templatePresenter->getCurrentDataset());
  }
  m_functionBrowser->blockSignals(false);
}

void IndirectFitPropertyBrowser::syncTemplateBrowserWithFull() {
  m_templatePresenter->browser()->blockSignals(true);
  auto const funStr = m_functionBrowser->getFunctionString();
  if (auto const fun = m_functionBrowser->getGlobalFunction()) {
    m_templatePresenter->setFunction(funStr);
    m_templatePresenter->updateMultiDatasetParameters(*fun);
    m_templatePresenter->setGlobalParameters(m_functionBrowser->getGlobalParameters());
    m_templatePresenter->setCurrentDataset(m_functionBrowser->getCurrentDataset());
  }
  m_templatePresenter->browser()->blockSignals(false);
}

void IndirectFitPropertyBrowser::init() {
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

void IndirectFitPropertyBrowser::setFunctionTemplatePresenter(std::unique_ptr<ITemplatePresenter> templatePresenter) {
  if (m_templatePresenter) {
    throw std::logic_error("Template presenter already set.");
  }
  m_templatePresenter = std::move(templatePresenter);
  m_templatePresenter->init();
  connect(m_templatePresenter->browser(), SIGNAL(functionStructureChanged()), this, SIGNAL(functionChanged()));
}

void IndirectFitPropertyBrowser::setFunction(std::string const &funStr) {
  if (isFullFunctionBrowserActive()) {
    m_functionBrowser->setFunction(funStr);
  } else {
    m_templatePresenter->setFunction(funStr);
  }
}

MultiDomainFunction_sptr IndirectFitPropertyBrowser::getFitFunction() const {
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
  }
}

QString IndirectFitPropertyBrowser::getSingleFunctionStr() const {
  return QString::fromStdString(getSingleFunction()->asString());
}

std::string IndirectFitPropertyBrowser::minimizer(bool) const {
  return m_fitOptionsBrowser->getProperty("Minimizer").toStdString();
}

int IndirectFitPropertyBrowser::maxIterations() const {
  return m_fitOptionsBrowser->getProperty("MaxIterations").toInt();
}

int IndirectFitPropertyBrowser::getPeakRadius() const { return m_fitOptionsBrowser->getProperty("PeakRadius").toInt(); }

std::string IndirectFitPropertyBrowser::costFunction() const {
  return m_fitOptionsBrowser->getProperty("CostFunction").toStdString();
}

bool IndirectFitPropertyBrowser::convolveMembers() const {
  return m_fitOptionsBrowser->getProperty("ConvolveMembers").toStdString() != "0";
}

bool IndirectFitPropertyBrowser::outputCompositeMembers() const {
  return m_fitOptionsBrowser->getProperty("OutputCompositeMembers").toStdString() != "0";
}

std::string IndirectFitPropertyBrowser::fitEvaluationType() const {
  return m_fitOptionsBrowser->getProperty("EvaluationType").toStdString();
}

bool IndirectFitPropertyBrowser::ignoreInvalidData() const {
  return m_fitOptionsBrowser->getProperty("IgnoreInvalidData").toStdString() != "0";
}

std::string IndirectFitPropertyBrowser::fitType() const {
  return m_fitOptionsBrowser->getProperty("FitType").toStdString();
}

int IndirectFitPropertyBrowser::getNumberOfDatasets() const {
  return isFullFunctionBrowserActive() ? m_functionBrowser->getNumberOfDatasets()
                                       : m_templatePresenter->getNumberOfDatasets();
}

void IndirectFitPropertyBrowser::updateParameters(const IFunction &fun) {
  if (isFullFunctionBrowserActive())
    m_functionBrowser->updateParameters(fun);
  else
    m_templatePresenter->updateParameters(fun);
}

void IndirectFitPropertyBrowser::updateFunctionListInBrowser(
    const std::map<std::string, std::string> &functionStrings) {
  m_templatePresenter->updateAvailableFunctions(functionStrings);
}

void IndirectFitPropertyBrowser::updateMultiDatasetParameters(const IFunction &fun) {
  if (isFullFunctionBrowserActive())
    m_functionBrowser->updateMultiDatasetParameters(fun);
  else
    m_templatePresenter->updateMultiDatasetParameters(fun);
}

void IndirectFitPropertyBrowser::updateMultiDatasetParameters(const ITableWorkspace &paramTable) {
  if (isFullFunctionBrowserActive())
    m_functionBrowser->updateMultiDatasetParameters(paramTable);
  else
    m_templatePresenter->updateMultiDatasetParameters(paramTable);
}

void IndirectFitPropertyBrowser::updateFitStatusData(const std::vector<std::string> &status,
                                                     const std::vector<double> &chiSquared) {
  m_fitStatus = status;
  m_fitChiSquared = chiSquared;
  updateFitStatus(currentDataset());
}

void IndirectFitPropertyBrowser::updateFitStatus(const FitDomainIndex index) {
  if (index.value + 1 > m_fitStatus.size() || index.value + 1 > m_fitChiSquared.size()) {
    return;
  }
  m_fitStatusWidget->update(m_fitStatus[index.value], m_fitChiSquared[index.value]);
  return;
}

/**
 * @return  The currently active fitting mode (Sequential or Simultaneous).
 */
FittingMode IndirectFitPropertyBrowser::getFittingMode() const { return m_fitOptionsBrowser->getCurrentFittingType(); }

/**
 * Sets whether fit members should be convolved with the resolution after a fit.
 *
 * @param convolveEnabled: If true, members are to be convolved.
 */
void IndirectFitPropertyBrowser::setConvolveMembers(bool convolveEnabled) {
  m_fitOptionsBrowser->setProperty("ConvolveMembers", QString(convolveEnabled ? "1" : "0"));
}

/**
 * Sets whether to output fit members
 *
 * @param outputEnabled: If true, fit members are outputted
 */
void IndirectFitPropertyBrowser::setOutputCompositeMembers(bool outputEnabled) {
  m_fitOptionsBrowser->setProperty("OutputCompositeMembers", QString(outputEnabled ? "1" : "0"));
}

/**
 * Clears the functions in this indirect fit property browser.
 */
void IndirectFitPropertyBrowser::clear() {
  m_functionBrowser->clear();
  m_templatePresenter->browser()->clear();
}

/**
 * Updates the plot guess feature in this indirect fit property browser.
 * @param sampleWorkspace :: The workspace loaded as sample
 */
void IndirectFitPropertyBrowser::updatePlotGuess(const MatrixWorkspace_const_sptr &) {}

void IndirectFitPropertyBrowser::setErrorsEnabled(bool enabled) {
  m_functionBrowser->setErrorsEnabled(enabled);
  m_templatePresenter->setErrorsEnabled(enabled);
}

EstimationDataSelector IndirectFitPropertyBrowser::getEstimationDataSelector() const {
  return m_templatePresenter->getEstimationDataSelector();
}

void IndirectFitPropertyBrowser::updateParameterEstimationData(DataForParameterEstimationCollection &&data) {
  m_templatePresenter->updateParameterEstimationData(std::move(data));
}

void IndirectFitPropertyBrowser::estimateFunctionParameters() { m_templatePresenter->estimateFunctionParameters(); }

void IndirectFitPropertyBrowser::setBackgroundA0(double value) {
  if (isFullFunctionBrowserActive()) {
    m_functionBrowser->setBackgroundA0(value);
  } else {
    m_templatePresenter->setBackgroundA0(value);
  }
}

void IndirectFitPropertyBrowser::setCurrentDataset(FitDomainIndex i) {
  if (getNumberOfDatasets() == 0)
    return;
  updateFitStatus(i);
  if (isFullFunctionBrowserActive()) {
    m_functionBrowser->setCurrentDataset(static_cast<int>(i.value));
  } else {
    m_templatePresenter->setCurrentDataset(static_cast<int>(i.value));
  }
}

FitDomainIndex IndirectFitPropertyBrowser::currentDataset() const {
  if (isFullFunctionBrowserActive()) {
    return FitDomainIndex{static_cast<size_t>(m_functionBrowser->getCurrentDataset())};
  } else {
    return FitDomainIndex{static_cast<size_t>(m_templatePresenter->getCurrentDataset())};
  }
}

void IndirectFitPropertyBrowser::updateFunctionBrowserData(
    int nData, const QList<FunctionModelDataset> &datasets, const std::vector<double> &qValues,
    const std::vector<std::pair<std::string, size_t>> &fitResolutions) {
  m_functionBrowser->setNumberOfDatasets(nData);
  m_functionBrowser->setDatasets(datasets);
  m_templatePresenter->setNumberOfDatasets(nData);
  m_templatePresenter->setDatasets(datasets);
  m_templatePresenter->setQValues(qValues);
  m_templatePresenter->setResolution(fitResolutions);
}

void IndirectFitPropertyBrowser::setFitEnabled(bool) {}

/**
 * Schedules a fit.
 */
void IndirectFitPropertyBrowser::fit() { emit fitScheduled(); }

/**
 * Schedules a sequential fit.
 */
void IndirectFitPropertyBrowser::sequentialFit() { emit sequentialFitScheduled(); }

void IndirectFitPropertyBrowser::setModelResolution(const std::vector<std::pair<std::string, size_t>> &fitResolutions) {
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
void IndirectFitPropertyBrowser::browserVisibilityChanged(bool isVisible) {
  if (!isVisible)
    emit browserClosed();
}

void IndirectFitPropertyBrowser::updateFitType() {
  auto const nGlobals = m_functionBrowser->getGlobalParameters().size();
  if (nGlobals == 0) {
    m_fitOptionsBrowser->setCurrentFittingType(FittingMode::SEQUENTIAL);
  } else {
    m_fitOptionsBrowser->setCurrentFittingType(FittingMode::SIMULTANEOUS);
  }
}

void IndirectFitPropertyBrowser::showFullFunctionBrowser(bool on) {
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

} // namespace MantidQt::CustomInterfaces::IDA
