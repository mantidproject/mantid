// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitPropertyBrowser.h"
#include "FunctionTemplateBrowser.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/WorkspaceGroup.h"

#include "MantidQtWidgets/Common/SignalBlocker.h"
#include "MantidQtWidgets/Common/FitOptionsBrowser.h"
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

#include <iostream>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

/**
 * Constructor
 * @param parent :: The parent widget - must be an ApplicationWindow
 * @param mantidui :: The UI form for MantidPlot
 */
IndirectFitPropertyBrowser::IndirectFitPropertyBrowser(QWidget *parent)
    : QDockWidget(parent), m_templateBrowser(nullptr), m_functionWidget(nullptr)
{
  setFeatures(QDockWidget::DockWidgetFloatable |
              QDockWidget::DockWidgetMovable);
  setWindowTitle("Fit Function");
}

void IndirectFitPropertyBrowser::initFunctionBrowser() {
  m_functionBrowser = new FunctionBrowser(nullptr, true);
  m_functionBrowser->setObjectName("functionBrowser");
  // Process internally
  connect(m_functionBrowser, SIGNAL(globalsChanged()), this, SLOT(updateFitType()));
  // Re-emit
  connect(m_functionBrowser, SIGNAL(functionStructureChanged()), this,
          SIGNAL(functionChanged()));
  connect(m_functionBrowser, SIGNAL(parameterChanged(const QString &, const QString &)), this, SIGNAL(functionChanged()));
  connect(m_functionBrowser, SIGNAL(tiesChanged()), this, SIGNAL(functionChanged()));
  connect(m_functionBrowser, SIGNAL(constraintsChanged()), this, SIGNAL(functionChanged()));
  connect(m_functionBrowser, SIGNAL(globalsChanged()), this, SIGNAL(functionChanged()));
  connect(m_functionBrowser,
          SIGNAL(localParameterButtonClicked(const QString &)), this,
          SIGNAL(localParameterEditRequested(const QString &)));
}

void IndirectFitPropertyBrowser::initFitOptionsBrowser() {
  m_fitOptionsBrowser = new FitOptionsBrowser(nullptr, FitOptionsBrowser::SimultaneousAndSequential);
  m_fitOptionsBrowser->setObjectName("fitOptionsBrowser");
  m_fitOptionsBrowser->setCurrentFittingType(FitOptionsBrowser::Sequential);
}

bool IndirectFitPropertyBrowser::isFullFunctionBrowserActive() const
{
  return m_functionWidget->currentIndex() == 1;
}

MultiDomainFunction_sptr IndirectFitPropertyBrowser::getGlobalFunction() const
{
  auto fun = isFullFunctionBrowserActive() ? m_functionBrowser->getGlobalFunction() : m_templateBrowser->getGlobalFunction();
  return boost::dynamic_pointer_cast<MultiDomainFunction>(fun);
}

IFunction_sptr IndirectFitPropertyBrowser::getSingleFunction() const
{
  return isFullFunctionBrowserActive() ? m_functionBrowser->getFunction() : m_templateBrowser->getFunction();
}

QStringList IndirectFitPropertyBrowser::getGlobalParameters() const
{
  return isFullFunctionBrowserActive() ? m_functionBrowser->getGlobalParameters() : m_templateBrowser->getGlobalParameters();
}

QStringList IndirectFitPropertyBrowser::getLocalParameters() const
{
  return isFullFunctionBrowserActive() ? m_functionBrowser->getLocalParameters() : m_templateBrowser->getLocalParameters();
}

void IndirectFitPropertyBrowser::syncFullBrowserWithTemplate() {
  auto const fun = m_templateBrowser->getFunction();
  m_functionBrowser->setFunction(fun);
  if (fun) {
    m_functionBrowser->updateMultiDatasetParameters(
        *m_templateBrowser->getGlobalFunction());
    m_functionBrowser->setGlobalParameters(
        m_templateBrowser->getGlobalParameters());
  }
}

void IndirectFitPropertyBrowser::syncTemplateBrowserWithFull() {
  auto const funStr = m_functionBrowser->getFunctionString();
  m_templateBrowser->setFunction(funStr);
  if (auto const fun = m_functionBrowser->getGlobalFunction()) {
    m_templateBrowser->updateMultiDatasetParameters(*fun);
    m_templateBrowser->setGlobalParameters(
        m_functionBrowser->getGlobalParameters());
  }
}

void IndirectFitPropertyBrowser::init() {
  initFunctionBrowser();
  initFitOptionsBrowser();

  auto w = new QWidget(this);
  m_mainLayout = new QVBoxLayout(w);
  m_mainLayout->setContentsMargins(0, 0, 0, 0);
  m_functionWidget = new QStackedWidget(this);
  if (m_templateBrowser) {
    m_functionWidget->insertWidget(0, m_templateBrowser);
    m_browserSwitcher = new QCheckBox("See full function");
    m_browserSwitcher->setObjectName("browserSwitcher");
    connect(m_browserSwitcher, SIGNAL(clicked(bool)), this, SLOT(showFullFunctionBrowser(bool)));
    m_mainLayout->insertWidget(0, m_browserSwitcher);
  }
  m_functionWidget->addWidget(m_functionBrowser);
  auto splitter = new QSplitter(Qt::Vertical);
  m_mainLayout->addWidget(splitter);
  splitter->addWidget(m_functionWidget);
  splitter->addWidget(m_fitOptionsBrowser);
  w->setLayout(m_mainLayout);
  setWidget(w);
}

void IndirectFitPropertyBrowser::setFunctionTemplateBrowser(FunctionTemplateBrowser * templateBrowser)
{
  if (m_templateBrowser) {
    throw std::logic_error("Template browser already set.");
  }
  m_templateBrowser = templateBrowser;
  m_templateBrowser->init();
  m_templateBrowser->setObjectName("templateBrowser");
  connect(m_templateBrowser, SIGNAL(functionStructureChanged()), this,
    SIGNAL(functionChanged()));
}

void IndirectFitPropertyBrowser::setFunction(const QString &funStr) {
  if (isFullFunctionBrowserActive()) {
    m_functionBrowser->setFunction(funStr);
  } else {
    m_templateBrowser->setFunction(funStr);
  }
}

MultiDomainFunction_sptr
IndirectFitPropertyBrowser::getFittingFunction() const {
  try {
    if (getNumberOfDatasets() > 0) {
      return getGlobalFunction();
    } else {
        auto multiDomainFunction = boost::make_shared<MultiDomainFunction>();
        auto singleFunction = getSingleFunction();
        if (singleFunction) {
          multiDomainFunction->addFunction(singleFunction);
          multiDomainFunction->setDomainIndex(0, 0);
        }
        return multiDomainFunction;
    }
  } catch (std::invalid_argument) {
    return boost::make_shared<MultiDomainFunction>();
  }
}

QString IndirectFitPropertyBrowser::getSingleFunctionStr() const
{
  return QString::fromStdString(getSingleFunction()->asString());
}

std::string IndirectFitPropertyBrowser::minimizer(bool withProperties) const {
  return m_fitOptionsBrowser->getProperty("Minimizer").toStdString();
}

int IndirectFitPropertyBrowser::maxIterations() const {
  return m_fitOptionsBrowser->getProperty("MaxIterations").toInt();
}

int IndirectFitPropertyBrowser::getPeakRadius() const {
  return m_fitOptionsBrowser->getProperty("PeakRadius").toInt();
}

std::string IndirectFitPropertyBrowser::costFunction() const {
  return m_fitOptionsBrowser->getProperty("CostFunction").toStdString();
}

bool IndirectFitPropertyBrowser::convolveMembers() const { return false; }

bool IndirectFitPropertyBrowser::isHistogramFit() const { return false; }

bool IndirectFitPropertyBrowser::ignoreInvalidData() const { return false; }

int IndirectFitPropertyBrowser::getNumberOfDatasets() const
{
  return isFullFunctionBrowserActive() ? m_functionBrowser->getNumberOfDatasets() : m_templateBrowser->getNumberOfDatasets();
}

void IndirectFitPropertyBrowser::updateParameters(const IFunction &fun) {
  if (isFullFunctionBrowserActive())
    m_functionBrowser->updateParameters(fun);
  else
    m_templateBrowser->updateParameters(fun);
}

void IndirectFitPropertyBrowser::updateMultiDatasetParameters(const IFunction & fun) {
  if (isFullFunctionBrowserActive())
    m_functionBrowser->updateMultiDatasetParameters(fun);
  else
    m_templateBrowser->updateMultiDatasetParameters(fun);
}

void IndirectFitPropertyBrowser::updateMultiDatasetParameters(const ITableWorkspace & paramTable)
{
  if (isFullFunctionBrowserActive())
    m_functionBrowser->updateMultiDatasetParameters(paramTable);
  else
    m_templateBrowser->updateMultiDatasetParameters(paramTable);
}

/**
 * @return  The selected fit type in the fit type combo box.
 */
QString IndirectFitPropertyBrowser::selectedFitType() const {
  return m_fitOptionsBrowser->getCurrentFittingType() == FitOptionsBrowser::Simultaneous ? "Simultaneous" : "Sequential";
}

/**
 * Sets whether fit members should be convolved with the resolution after a fit.
 *
 * @param convolveMembers If true, members are to be convolved.
 */
void IndirectFitPropertyBrowser::setConvolveMembers(bool convolveMembers) {
}

/**
 * Clears the functions in this indirect fit property browser.
 */
void IndirectFitPropertyBrowser::clear() {
  //FitPropertyBrowser::clear();
}

/**
 * Updates the plot guess feature in this indirect fit property browser.
 * @param sampleWorkspace :: The workspace loaded as sample
 */
void IndirectFitPropertyBrowser::updatePlotGuess(
    MatrixWorkspace_const_sptr sampleWorkspace) {
  // if (sampleWorkspace && compositeFunction()->nFunctions() > 0)
  //  setPeakToolOn(true);
  // else
  //  setPeakToolOn(false);
}

void IndirectFitPropertyBrowser::setErrorsEnabled(bool enabled)
{
  m_functionBrowser->setErrorsEnabled(enabled);
  m_templateBrowser->setErrorsEnabled(enabled);
}

void IndirectFitPropertyBrowser::updateParameterEstimationData(
    DataForParameterEstimationCollection &&data) {
  m_templateBrowser->updateParameterEstimationData(std::move(data));
}

void IndirectFitPropertyBrowser::setBackgroundA0(double value) {
  if (isFullFunctionBrowserActive()) {
    m_functionBrowser->setBackgroundA0(value);
  } else {
    m_templateBrowser->setBackgroundA0(value);
  }
}

void IndirectFitPropertyBrowser::setCurrentDataset(SpectrumRowIndex i) {
  if (m_functionBrowser->getNumberOfDatasets() == 0) return;
  if (isFullFunctionBrowserActive()) {
    m_functionBrowser->setCurrentDataset(i.value);
  } else {
    m_templateBrowser->setCurrentDataset(i.value);
  }
}

SpectrumRowIndex IndirectFitPropertyBrowser::currentDataset() const { return SpectrumRowIndex{m_functionBrowser->getCurrentDataset()}; }

void IndirectFitPropertyBrowser::updateFunctionBrowserData(SpectrumRowIndex nData, const QStringList &datasetNames) {
  m_functionBrowser->setNumberOfDatasets(nData.value);
  m_functionBrowser->setDatasetNames(datasetNames);
  m_templateBrowser->setNumberOfDatasets(nData.value);
  m_templateBrowser->setDatasetNames(datasetNames);
}

void IndirectFitPropertyBrowser::setFitEnabled(bool enable) {
  // FitPropertyBrowser::setFitEnabled(enable);
}

/**
 * Schedules a fit.
 */
void IndirectFitPropertyBrowser::fit() { emit fitScheduled(); }

/**
 * Schedules a sequential fit.
 */
void IndirectFitPropertyBrowser::sequentialFit() {
  emit sequentialFitScheduled();
}

void IndirectFitPropertyBrowser::setModelResolution(std::string const &name,
                                                    DatasetIndex const &index) {
  if (isFullFunctionBrowserActive()) {
    showFullFunctionBrowser(false);
  }
  m_templateBrowser->setResolution(name, index);
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
    m_fitOptionsBrowser->setCurrentFittingType(FitOptionsBrowser::Sequential);
  } else {
    m_fitOptionsBrowser->setCurrentFittingType(FitOptionsBrowser::Simultaneous);
  }
}

void IndirectFitPropertyBrowser::showFullFunctionBrowser(bool on){
  if (on) {
    syncFullBrowserWithTemplate();
  } else {
    try {
      syncTemplateBrowserWithFull();
    } catch (const std::runtime_error&) {
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

} // IDA
} // namespace CustomInterfaces
} // namespace MantidQt
