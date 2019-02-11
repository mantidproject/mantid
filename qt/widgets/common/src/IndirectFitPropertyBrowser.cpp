// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/IndirectFitPropertyBrowser.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/SignalBlocker.h"

#include "MantidQtWidgets/Common/FitOptionsBrowser.h"
#include "MantidQtWidgets/Common/FunctionBrowser.h"

#include <QAction>
#include <QFormLayout>
#include <QMessageBox>
#include <QSettings>

#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>

#include <QMenu>
#include <QSignalMapper>

#include <QCheckBox>

#include <iostream>

using namespace Mantid::API;

namespace MantidQt {
namespace MantidWidgets {

/**
 * Constructor
 * @param parent :: The parent widget - must be an ApplicationWindow
 * @param mantidui :: The UI form for MantidPlot
 */
IndirectFitPropertyBrowser::IndirectFitPropertyBrowser(QWidget *parent,
                                                       QObject *mantidui)
    : QDockWidget(parent)
{
  setFeatures(QDockWidget::DockWidgetFloatable |
              QDockWidget::DockWidgetMovable);
  setWindowTitle("Fit Function");
}

void IndirectFitPropertyBrowser::initFunctionBrowser() {
  m_functionBrowser = new FunctionBrowser(nullptr, true);
  m_functionBrowser->setObjectName("functionBrowser");
  connect(m_functionBrowser, SIGNAL(functionStructureChanged()), this,
          SIGNAL(functionChanged()));
  connect(m_functionBrowser,
          SIGNAL(localParameterButtonClicked(const QString &)), this,
          SIGNAL(localParameterEditRequested(const QString &)));
  connect(m_functionBrowser, SIGNAL(globalsChanged()), this, SLOT(updateFitType()));
}

void IndirectFitPropertyBrowser::iniFitOptionsBrowser() {
  m_fitOptionsBrowser = new FitOptionsBrowser(nullptr, FitOptionsBrowser::SimultaneousAndSequential);
  m_fitOptionsBrowser->setObjectName("fitOptionsBrowser");
  m_fitOptionsBrowser->setCurrentFittingType(FitOptionsBrowser::Sequential);
}

void IndirectFitPropertyBrowser::init() {
  initFunctionBrowser();
  iniFitOptionsBrowser();

  auto w = new QWidget(this);
  auto layout = new QVBoxLayout(w);
  layout->setContentsMargins(0, 0, 0, 0);
  auto splitter = new QSplitter(Qt::Vertical);
  layout->addWidget(splitter);

  splitter->addWidget(m_functionBrowser);
  splitter->addWidget(m_fitOptionsBrowser);
  w->setLayout(layout);
  setWidget(w);
}

void IndirectFitPropertyBrowser::setFunction(const QString &funStr) {
  m_functionBrowser->setFunction(funStr);
}

MultiDomainFunction_sptr
IndirectFitPropertyBrowser::getFittingFunction() const {
  try {
    if (m_functionBrowser->getNumberOfDatasets() == 0) {
        auto multiDomainFunction = boost::make_shared<MultiDomainFunction>();
        multiDomainFunction->addFunction(m_functionBrowser->getFunction());
        multiDomainFunction->setDomainIndex(0, 0);
        return multiDomainFunction;
    }
    return boost::dynamic_pointer_cast<MultiDomainFunction>(m_functionBrowser->getGlobalFunction());
  } catch (std::invalid_argument) {
    return boost::make_shared<MultiDomainFunction>();
  }
}

//IFunction_sptr IndirectFitPropertyBrowser::compositeFunction() const {
//  return getFittingFunction();
//}

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

} // namespace MantidWidgets
} // namespace MantidQt
