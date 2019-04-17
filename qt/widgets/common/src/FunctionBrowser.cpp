// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FunctionBrowser.h"
#include "MantidQtWidgets/Common/FunctionTreeView.h"
#include "MantidQtWidgets/Common/FunctionMultiDomainPresenter.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidAPI/ParameterTie.h"

#include "MantidKernel/Logger.h"
#include "MantidKernel/make_unique.h"

#include "MantidQtWidgets/Common/EditLocalParameterDialog.h"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QMessageBox>

#include <algorithm>
#include <boost/lexical_cast.hpp>

namespace {
const char *globalOptionName = "Global";
Mantid::Kernel::Logger g_log("Function Browser");
} // namespace

namespace MantidQt {
namespace MantidWidgets {

using namespace Mantid::API;
using namespace Mantid::Kernel;

/**
 * Constructor
 * @param parent :: The parent widget.
 * @param multi  :: Option to use the browser for multi-dataset fitting.
 */
FunctionBrowser::FunctionBrowser(QWidget *parent, bool multi, const std::vector<std::string>& categories)
    : QWidget(parent)
{
  auto view = new FunctionTreeView(this, multi, categories);
  m_presenter = make_unique<FunctionMultiDomainPresenter>(view);
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setMargin(0);
  layout->addWidget(view);
}

/**
 * Destructor
 */
FunctionBrowser::~FunctionBrowser() {}

/**
 * Clear the contents
 */
void FunctionBrowser::clear() {
  m_presenter->clear();
}

/**
 * Set the function in the browser
 * @param funStr :: FunctionFactory function creation string
 */
void FunctionBrowser::setFunction(const QString &funStr) {
  if (funStr.isEmpty())
    return;
  try {
    auto fun = FunctionFactory::Instance().createInitialized(
        funStr.toStdString());
    if (!fun)
      return;
    this->setFunction(fun);
  } catch (...) {
    // error in the input string
  }
}

/**
 * Set the function in the browser
 * @param fun :: A function
 */
void FunctionBrowser::setFunction(IFunction_sptr fun) {
  m_presenter->setFunction(fun);
}

/**
 * Return function at specified function index (e.g. f0.)
 * @param index :: Index of the function, or empty string for top-level function
 * @return Function at index, or null pointer if not found
 */
IFunction_sptr FunctionBrowser::getFunctionByIndex(const QString &index) {
    return m_presenter->getFunctionByIndex(index);
}

/**
 * Updates the function parameter value
 * @param paramName :: Fully qualified parameter name (includes function index)
 * @param value :: New value
 */
void FunctionBrowser::setParameter(const QString &paramName, double value) {
  m_presenter->setParameter(paramName, value);
}

/**
 * Updates the function parameter error
 * @param paramName :: Fully qualified parameter name (includes function index)
 * @param error :: New error
 */
void FunctionBrowser::setParamError(const QString &paramName, double error) {
  m_presenter->setParamError(paramName, error);
}

/**
 * Get a value of a parameter
 * @param paramName :: Fully qualified parameter name (includes function index)
 */
double FunctionBrowser::getParameter(const QString &paramName) const {
  return m_presenter->getParameter(paramName);
}

/**
 * Update parameter values in the browser to match those of a function.
 * @param fun :: A function to copy the values from. It must have the same
 *   type (composition) as the function in the browser.
 */
void FunctionBrowser::updateParameters(const IFunction &fun) {
  m_presenter->updateParameters(fun);
}

/**
 * Return FunctionFactory function string
 */
QString FunctionBrowser::getFunctionString() {
  return m_presenter->getFunctionString();
}

Mantid::API::IFunction_sptr FunctionBrowser::getFunction() {
  return m_presenter->getFunction();
}

/**
 * Remove the function under currently selected property
 */
void FunctionBrowser::removeFunction() {

}

void FunctionBrowser::addFunction() {

}

/**
 * Fix currently selected parameter
 */
void FunctionBrowser::fixParameter() {
}

/**
 * Unfix currently selected parameter
 */
void FunctionBrowser::removeTie() {
}

/**
 * Add a custom tie to currently selected parameter
 */
void FunctionBrowser::addTie() {
}

/**
 * Copy function from the clipboard
 */
void FunctionBrowser::copyFromClipboard() {
  QString funStr = QApplication::clipboard()->text();
  if (funStr.isEmpty())
    return;
  try {
    auto fun = Mantid::API::FunctionFactory::Instance().createInitialized(
        funStr.toStdString());
    if (!fun)
      return;
    this->setFunction(fun);
  } catch (...) {
    // text in the clipboard isn't a function definition
    QMessageBox::warning(this, "MantidPlot - Warning",
                         "Text in the clipboard isn't a function definition"
                         " or contains errors.");
  }
}

/**
 * Copy function to the clipboard
 */
void FunctionBrowser::copyToClipboard() {
  auto fun = getFunction();
  if (fun) {
    QApplication::clipboard()->setText(QString::fromStdString(fun->asString()));
  }
}

/**
 * Add both constraints to current parameter
 */
void FunctionBrowser::addConstraints() {
}

/**
 * Add both constraints to current parameter
 */
void FunctionBrowser::addConstraints10() {
}

/**
 * Add both constraints to current parameter
 */
void FunctionBrowser::addConstraints50() {
}

/**
 * Remove both constraints from current parameter
 */
void FunctionBrowser::removeConstraints() {
}

/**
 * Remove one constraint from current parameter
 */
void FunctionBrowser::removeConstraint() {
}

void FunctionBrowser::updateCurrentFunctionIndex() {
  //boost::optional<QString> newIndex;

  //if (auto item = m_browser->currentItem()) {
  //  auto prop = item->property();
  //  newIndex = getIndex(prop);
  //}

  //if (m_currentFunctionIndex != newIndex) {
  //  m_currentFunctionIndex = newIndex;
  //  emit currentFunctionChanged();
  //}
}

bool FunctionBrowser::hasFunction() const {
  return false;// m_view->hasFunction();
}

/// Get the number of datasets
int FunctionBrowser::getNumberOfDatasets() const {
  return m_presenter->getNumberOfDatasets();
}

/// Set new number of the datasets
/// @param n :: New value for the number of datasets.
void FunctionBrowser::setNumberOfDatasets(int n) {
  m_presenter->setNumberOfDatasets(n);
}

/**
 * Get value of a local parameter
 * @param parName :: Name of a parameter.
 * @param i :: Data set index.
 */
double FunctionBrowser::getLocalParameterValue(const QString &parName,
                                               int i) const {
  return 0.0;
}

void FunctionBrowser::setLocalParameterValue(const QString &parName, int i,
                                             double value) {
  //checkLocalParameter(parName);
  //m_localParameterValues[parName][i].value = value;
  //if (i == m_currentDataset) {
  //  setParameter(parName, value);
  //}
}

void FunctionBrowser::setLocalParameterValue(const QString &parName, int i,
                                             double value, double error) {
  //checkLocalParameter(parName);
  //m_localParameterValues[parName][i].value = value;
  //m_localParameterValues[parName][i].error = error;
  //if (i == m_currentDataset) {
  //  setParameter(parName, value);
  //  setParamError(parName, error);
  //}
}
/// Get error of a local parameter
double FunctionBrowser::getLocalParameterError(const QString &parName,
                                               int i) const {
  //checkLocalParameter(parName);
  //return m_localParameterValues[parName][i].error;
  return 0.0;
}

void FunctionBrowser::resetLocalParameters() {}

/// Set current dataset.
void FunctionBrowser::setCurrentDataset(int i) {
  //m_currentDataset = i;
  //if (m_currentDataset >= m_numberOfDatasets) {
  //  throw std::runtime_error("Dataset index is outside the range");
  //}
  //auto localParameters = getLocalParameters();
  //foreach (QString par, localParameters) {
  //  setParameter(par, getLocalParameterValue(par, m_currentDataset));
  //  setParamError(par, getLocalParameterError(par, m_currentDataset));
  //  //updateLocalTie(par);
  //  //updateLocalConstraint(par);
  //}
}

/// Remove local parameter values for a number of datasets.
/// @param indices :: A list of indices of datasets to remove.
void FunctionBrowser::removeDatasets(QList<int> indices) {
  //if (indices.size() > m_numberOfDatasets) {
  //  throw std::runtime_error(
  //      "FunctionBrowser asked to removed too many datasets");
  //}
  //qSort(indices);
  //for (auto par = m_localParameterValues.begin();
  //     par != m_localParameterValues.end(); ++par) {
  //  for (int i = indices.size() - 1; i >= 0; --i) {
  //    int index = indices[i];
  //    if (index < 0) {
  //      throw std::runtime_error(
  //          "Index of a dataset in FunctionBrowser cannot be negative.");
  //    }
  //    if (index < m_numberOfDatasets) {
  //      auto &v = par.value();
  //      // value may not have been populated
  //      if (index < v.size()) {
  //        v.remove(index);
  //      }
  //    } else {
  //      throw std::runtime_error(
  //          "Index of a dataset in FunctionBrowser is out of range.");
  //    }
  //  }
  //}
  //setNumberOfDatasets(m_numberOfDatasets - indices.size());
}

/// Add local parameters for additional datasets.
/// @param n :: Number of datasets added.
void FunctionBrowser::addDatasets(int n) {
  //setNumberOfDatasets(m_numberOfDatasets + n);
}

/**
 * Launches the Edit Local Parameter dialog and deals with the input from it.
 * @param parName :: Name of parameter that button was clicked for.
 * @param wsNames :: Names of the workspaces the datasets came from.
 * @param wsIndices :: The workspace indices of the datasets.
 */
void FunctionBrowser::editLocalParameter(const QString &parName,
                                         const QStringList &wsNames,
                                         const std::vector<size_t> &wsIndices) {
  assert(wsNames.size() == wsIndices.size());
  assert(wsNames.size() == getNumberOfDatasets());
  EditLocalParameterDialog dialog(parentWidget(), this, parName, wsNames, wsIndices);
  if (dialog.exec() == QDialog::Accepted) {
    auto values = dialog.getValues();
    auto fixes = dialog.getFixes();
    auto ties = dialog.getTies();
    assert(values.size() == getNumberOfDatasets());
    for (int i = 0; i < values.size(); ++i) {
      setLocalParameterValue(parName, i, values[i]);
      setLocalParameterFixed(parName, i, fixes[i]);
      setLocalParameterTie(parName, i, ties[i]);
    }
  }
}

/// Return the multidomain function for multi-dataset fitting
Mantid::API::IFunction_sptr FunctionBrowser::getGlobalFunction() {
  return Mantid::API::IFunction_sptr();
  //if (!m_multiDataset) {
  //  throw std::runtime_error(
  //      "Function browser wasn't set up for multi-dataset fitting.");
  //}
  //// number of spectra to fit == size of the multi-domain function
  //int nOfDataSets = getNumberOfDatasets();
  //if (nOfDataSets == 0) {
  //  throw std::runtime_error("There are no data sets specified.");
  //}

  //// description of a single function
  //QString funStr = getFunctionString();

  //if (nOfDataSets == 1) {
  //  return Mantid::API::FunctionFactory::Instance().createInitialized(
  //      funStr.toStdString());
  //}

  //bool isComposite =
  //    (std::find(funStr.begin(), funStr.end(), ';') != funStr.end());
  //if (isComposite) {
  //  funStr = ";(" + funStr + ")";
  //} else {
  //  funStr = ";" + funStr;
  //}

  //QString multiFunStr = "composite=MultiDomainFunction,NumDeriv=1";
  //for (int i = 0; i < nOfDataSets; ++i) {
  //  multiFunStr += funStr;
  //}

  //// add the global ties
  //QStringList globals = getGlobalParameters();
  //QString globalTies;
  //if (!globals.isEmpty()) {
  //  globalTies = "ties=(";
  //  bool isFirst = true;
  //  foreach (QString par, globals) {
  //    if (!isFirst)
  //      globalTies += ",";
  //    else
  //      isFirst = false;

  //    for (int i = 1; i < nOfDataSets; ++i) {
  //      globalTies += QString("f%1.").arg(i) + par + "=";
  //    }
  //    globalTies += QString("f0.%1").arg(par);
  //  }
  //  globalTies += ")";
  //  multiFunStr += ";" + globalTies;
  //}

  //// create the multi-domain function
  //auto fun = Mantid::API::FunctionFactory::Instance().createInitialized(
  //    multiFunStr.toStdString());
  //boost::shared_ptr<Mantid::API::MultiDomainFunction> multiFun =
  //    boost::dynamic_pointer_cast<Mantid::API::MultiDomainFunction>(fun);
  //if (!multiFun) {
  //  throw std::runtime_error("Failed to create the MultiDomainFunction");
  //}

  //auto globalParams = getGlobalParameters();

  //// set the domain indices, initial local parameter values and ties
  //for (int i = 0; i < nOfDataSets; ++i) {
  //  multiFun->setDomainIndex(i, i);
  //  auto fun1 = multiFun->getFunction(i);
  //  for (size_t j = 0; j < fun1->nParams(); ++j) {
  //    const auto parameterName = fun1->parameterName(j);
  //    const auto parName = QString::fromStdString(parameterName);
  //    if (globalParams.contains(parName))
  //      continue;
  //    auto tie = fun1->getTie(j);
  //    if (tie) {
  //      // If parameter has a tie at this stage then it gets it from the
  //      // currently displayed function. But the i-th local parameters may not
  //      // have this tie, so remove it
  //      fun1->removeTie(j);
  //    }
  //    if (fun1->isFixed(j)) {
  //      fun1->unfix(j);
  //    }
  //    if (fun1->getConstraint(j)) {
  //      fun1->removeConstraint(parameterName);
  //    }
  //    //checkLocalParameter(parName);
  //    //const auto &localParam = m_localParameterValues[parName][i];
  //    //if (localParam.fixed) {
  //    //  // Fix this particular local parameter
  //    //  fun1->setParameter(j, localParam.value);
  //    //  fun1->fix(j);
  //    //} else {
  //    //  const auto &tie = localParam.tie;
  //    //  if (!tie.isEmpty()) {
  //    //    fun1->tie(parameterName, tie.toStdString());
  //    //  } else {
  //    //    fun1->setParameter(j, localParam.value);
  //    //  }
  //    //}
  //    //if (!localParam.lowerBound.isEmpty() &&
  //    //    !localParam.upperBound.isEmpty()) {
  //    //  auto constraint = getConstraint(parName, localParam.lowerBound,
  //    //                                  localParam.upperBound);
  //    //  fun1->addConstraints(constraint.toStdString());
  //    //}
  //  }
  //}
  //assert(multiFun->nFunctions() == static_cast<size_t>(nOfDataSets));

  //return fun;
}

/// Fix/unfix a local parameter
/// @param parName :: Parameter name
/// @param i :: Index of a dataset.
/// @param fixed :: Make it fixed (true) or free (false)
void FunctionBrowser::setLocalParameterFixed(const QString &parName, int i,
                                             bool fixed) {
  //checkLocalParameter(parName);
  //m_localParameterValues[parName][i].fixed = fixed;
  //if (i == m_currentDataset) {
  //  updateLocalTie(parName);
  //}
}

/// Check if a local parameter is fixed
/// @param parName :: Parameter name
/// @param i :: Index of a dataset.
bool FunctionBrowser::isLocalParameterFixed(const QString &parName,
                                            int i) const {
  //checkLocalParameter(parName);
  //return m_localParameterValues[parName][i].fixed;
  return false;
}

/// Get the tie for a local parameter.
/// @param parName :: Parameter name
/// @param i :: Index of a dataset.
QString FunctionBrowser::getLocalParameterTie(const QString &parName,
                                              int i) const {
  //checkLocalParameter(parName);
  //return m_localParameterValues[parName][i].tie;
  return "";
}

/// Set a tie for a local parameter.
/// @param parName :: Parameter name
/// @param i :: Index of a dataset.
/// @param tie :: A tie string.
void FunctionBrowser::setLocalParameterTie(const QString &parName, int i,
                                           QString tie) {
  //checkLocalParameter(parName);
  //m_localParameterValues[parName][i].tie = tie;
  //if (i == m_currentDataset) {
  //  updateLocalTie(parName);
  //}
}

/// Update the interface to have the same parameter values as in a function.
/// @param fun :: A function to get parameter values from.
void FunctionBrowser::updateMultiDatasetParameters(
    const Mantid::API::IFunction &fun) {
  //auto cfun = dynamic_cast<const Mantid::API::CompositeFunction *>(&fun);
  //if (cfun && cfun->nFunctions() > 0) {

  //  // Multiple datasets
  //  if (const auto *multiFun =
  //          dynamic_cast<const Mantid::API::MultiDomainFunction *>(cfun)) {
  //    // Check the function has the correct number of domains
  //    if (multiFun->getNumberDomains() !=
  //        static_cast<size_t>(m_numberOfDatasets)) {
  //      throw std::invalid_argument("Function has incorrect number of domains");
  //    }
  //    // update function
  //    {
  //      auto sfun = multiFun->getFunction(0);
  //      const auto globalParameters = getGlobalParameters();
  //      for (int j = 0; j < globalParameters.size(); ++j) {
  //        auto const &paramName = globalParameters[j];
  //        auto paramIndex = sfun->parameterIndex(paramName.toStdString());
  //        setParameter(paramName, sfun->getParameter(paramIndex));
  //        setParamError(paramName, sfun->getError(paramIndex));
  //      }
  //    }
  //    size_t currentIndex = static_cast<size_t>(m_currentDataset);
  //    const auto localParameters = getLocalParameters();
  //    for (size_t i = 0; i < multiFun->nFunctions(); ++i) {
  //      auto sfun = multiFun->getFunction(i);
  //      for (int j = 0; j < localParameters.size(); ++j) {
  //        auto const &paramName = localParameters[j];
  //        auto paramIndex = sfun->parameterIndex(paramName.toStdString());
  //        auto value = sfun->getParameter(paramIndex);
  //        auto error = sfun->getError(paramIndex);
  //        setLocalParameterValue(localParameters[j], static_cast<int>(i), value,
  //                               error);
  //        if (i == currentIndex) {
  //          setParameter(paramName, value);
  //          setParamError(paramName, error);
  //        }
  //      }
  //    }
  //  } else { // composite function, 1 domain only
  //    if (m_numberOfDatasets != 1) {
  //      throw std::invalid_argument(
  //          "Multiple datasets, but function is single-domain");
  //    }
  //    updateParameters(*cfun);
  //  }
  //} else {
  //  updateParameters(fun);
  //}
}

/// Get the index of the current dataset.
int FunctionBrowser::getCurrentDataset() const
{
  return m_presenter->getCurrentDataset();
}

/// Resize the browser's columns
/// @param s0 :: New size for the first column (Parameter).
/// @param s1 :: New size for the second column (Value).
/// @param s2 :: New size for the third optional column (Global).
void FunctionBrowser::setColumnSizes(int s0, int s1, int s2) {
  //m_view->setColumnSizes(s0, s1, s2);
}

///**
// * Emit a signal when any of the Global options change.
// */
//void FunctionBrowser::globalChanged(QtProperty *, const QString &, bool) {
//  emit globalsChanged();
//}
//
///**
// * Set display of parameter errors on/off
// * @param enabled :: [input] On/off display of errors
// */
void FunctionBrowser::setErrorsEnabled(bool enabled) {
//  m_parameterManager->setErrorsEnabled(enabled);
}
//
///**
// * Clear all errors, if they are set
// */
void FunctionBrowser::clearErrors() {  }

QStringList FunctionBrowser::getGlobalParameters() const {
  return QStringList();
}

QStringList FunctionBrowser::getLocalParameters() const {
  return QStringList();
}

void FunctionBrowser::setGlobalParameters(QStringList &globals) {}

boost::optional<QString> FunctionBrowser::currentFunctionIndex() { return boost::optional<QString>(); }

QString FunctionBrowser::getUserFunctionFromDialog() { return ""; }

FunctionTreeView *FunctionBrowser::view() const { return dynamic_cast<FunctionTreeView*>(m_presenter->view()); }

} // namespace MantidWidgets
} // namespace MantidQt
