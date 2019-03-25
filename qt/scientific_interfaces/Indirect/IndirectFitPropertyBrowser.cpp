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

void IndirectFitPropertyBrowser::init() {
  initFunctionBrowser();
  initFitOptionsBrowser();

  auto w = new QWidget(this);
  m_mainLayout = new QVBoxLayout(w);
  m_mainLayout->setContentsMargins(0, 0, 0, 0);
  m_functionWidget = new QStackedWidget(this);
  if (m_templateBrowser) {
    m_functionWidget->insertWidget(0, m_templateBrowser);
    auto browserSwitcher = new QCheckBox("See full function");
    browserSwitcher->setObjectName("browserSwitcher");
    connect(browserSwitcher, SIGNAL(clicked(bool)), this, SLOT(showFullFunctionBrowser(bool)));
    m_mainLayout->insertWidget(0, browserSwitcher);
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
}

void IndirectFitPropertyBrowser::setFunction(const QString &funStr) {
  m_functionBrowser->setFunction(funStr);
}

MultiDomainFunction_sptr
IndirectFitPropertyBrowser::getFittingFunction() const {
  try {
    if (m_functionBrowser->getNumberOfDatasets() > 0) {
      return boost::dynamic_pointer_cast<MultiDomainFunction>(m_functionBrowser->getGlobalFunction());
    } else {
        auto multiDomainFunction = boost::make_shared<MultiDomainFunction>();
        auto singleFunction = m_functionBrowser->getFunction();
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
  return m_functionBrowser->getFunctionString();
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
  return m_functionBrowser->getNumberOfDatasets();
}

void IndirectFitPropertyBrowser::updateParameters(
    const Mantid::API::IFunction &fun) {
  m_functionBrowser->updateParameters(fun);
}

void IndirectFitPropertyBrowser::updateMultiDatasetParameters(const Mantid::API::IFunction & fun) {
  m_functionBrowser->updateMultiDatasetParameters(fun);
}

void IndirectFitPropertyBrowser::updateMultiDatasetParameters(const Mantid::API::IFunction & fun, const Mantid::API::ITableWorkspace & paramTable)
{
  auto const nRows = paramTable.rowCount();
  if (nRows == 0) return;

  auto const globalParameterNames = m_functionBrowser->getGlobalParameters();
  for (auto &&name : globalParameterNames) {
    auto valueColumn = paramTable.getColumn(name.toStdString());
    auto errorColumn = paramTable.getColumn((name + "_Err").toStdString());
    m_functionBrowser->setParameter(name, valueColumn->toDouble(0));
    m_functionBrowser->setParamError(name, errorColumn->toDouble(0));
  }

  auto const localParameterNames = m_functionBrowser->getLocalParameters();
  for (auto &&name : localParameterNames) {
    auto valueColumn = paramTable.getColumn(name.toStdString());
    auto errorColumn = paramTable.getColumn((name + "_Err").toStdString());
    if (nRows > 1) {
      for (size_t i = 0; i < nRows; ++i) {
        m_functionBrowser->setLocalParameterValue(name, static_cast<int>(i), valueColumn->toDouble(i), errorColumn->toDouble(i));
      }
    } else {
      auto const i = m_functionBrowser->getCurrentDataset();
      m_functionBrowser->setLocalParameterValue(name, static_cast<int>(i), valueColumn->toDouble(0), errorColumn->toDouble(0));
    }
  }
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


void IndirectFitPropertyBrowser::setWorkspaceIndex(int i) {
  if (m_functionBrowser->getNumberOfDatasets() > 0)
    m_functionBrowser->setCurrentDataset(i);
}

int IndirectFitPropertyBrowser::workspaceIndex() const { return m_functionBrowser->getCurrentDataset(); }

void IndirectFitPropertyBrowser::updateFunctionBrowserData(size_t nData) {
  m_functionBrowser->setNumberOfDatasets(static_cast<int>(nData));
}

void IndirectFitPropertyBrowser::editLocalParameter(
    const QString &parName, const QStringList &wsNames,
    const std::vector<size_t> &wsIndices) {
  m_functionBrowser->editLocalParameter(parName, wsNames, wsIndices);
  emit functionChanged();
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
  auto const index = on ? 1 : 0;
  m_functionWidget->setCurrentIndex(index);
}

} // IDA
} // namespace CustomInterfaces
} // namespace MantidQt
