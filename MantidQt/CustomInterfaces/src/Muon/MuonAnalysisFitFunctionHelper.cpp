#include "MantidQtCustomInterfaces/Muon/MuonAnalysisFitFunctionHelper.h"
#include "MantidAPI/IFunction.h"

using MantidQt::MantidWidgets::IFunctionBrowser;
using MantidQt::MantidWidgets::IMuonFitFunctionControl;

namespace MantidQt {
namespace CustomInterfaces {

/**
 * Constructor
 * @param parent :: [input] Parent dialog (MuonAnalysis)
 * @param fitBrowser :: [input] Non-owning pointer to muon fit property browser
 * @param funcBrowser :: [input] Non-owning pointer to function browser
 */
MuonAnalysisFitFunctionHelper::MuonAnalysisFitFunctionHelper(
    QObject *parent, IMuonFitFunctionControl *fitBrowser,
    IFunctionBrowser *funcBrowser)
    : QObject(parent), m_fitBrowser(fitBrowser), m_funcBrowser(funcBrowser) {
  doConnect();
}

/**
 * Connect up signals and slots
 * Abstract base class is not a QObject, so attempt a cast.
 * (Its derived classes are QObjects).
 */
void MuonAnalysisFitFunctionHelper::doConnect() {
  if (const QObject *fitBrowser = dynamic_cast<QObject *>(m_fitBrowser)) {
    connect(fitBrowser, SIGNAL(functionUpdateRequested()), this,
            SLOT(updateFunction()));
    connect(fitBrowser, SIGNAL(functionUpdateAndFitRequested(bool)), this,
            SLOT(updateFunctionAndFit(bool)));
    connect(fitBrowser, SIGNAL(fittingDone(const QString &)), this,
            SLOT(handleFitFinished(const QString &)));
    connect(fitBrowser, SIGNAL(functionCleared()), this,
            SLOT(handleModelCleared()));
    connect(fitBrowser, SIGNAL(errorsEnabled(bool)), this,
            SLOT(handleErrorsEnabled(bool)));
    connect(fitBrowser, SIGNAL(fitUndone()), this, SLOT(handleFitFinished()));
    connect(fitBrowser, SIGNAL(functionLoaded(const QString &)), this,
            SLOT(handleFunctionLoaded(const QString &)));
  }
  if (const QObject *funcBrowser = dynamic_cast<QObject *>(m_funcBrowser)) {
    connect(funcBrowser, SIGNAL(functionStructureChanged()), this,
            SLOT(updateFunction()));
    connect(funcBrowser,
            SIGNAL(parameterChanged(const QString &, const QString &)), this,
            SLOT(handleParameterEdited(const QString &, const QString &)));
  }
}

/**
 * Queries function browser and updates function in fit property browser.
 */
void MuonAnalysisFitFunctionHelper::updateFunction() {
  const QString funcString = m_funcBrowser->getFunctionString();
  m_fitBrowser->setFunction(funcString);
}

/**
 * Called when a fit is requested.
 * Queries function browser and updates function in fit property browser.
 * Then calls fit or sequential fit as controlled by argument.
 * @param sequential :: [input] Whether a regular or sequential fit was
 * requested.
 */
void MuonAnalysisFitFunctionHelper::updateFunctionAndFit(bool sequential) {
  updateFunction();
  if (sequential) {
    m_fitBrowser->runSequentialFit();
  } else {
    m_fitBrowser->runFit();
  }
}

/**
 * Called when fit finished OR undone.
 * Updates parameters displayed in function browser from the fit results.
 * In the case of "fit undone", this has the effect of resetting them, and also
 * removing the errors.
 * @param wsName :: [input] workspace name - empty if fit undone
 */
void MuonAnalysisFitFunctionHelper::handleFitFinished(const QString &wsName) {
  const auto function = m_fitBrowser->getFunction();
  m_funcBrowser->updateParameters(*function);
  if (wsName.isEmpty()) {
    // No fitted workspace: a fit was undone so clear the errors
    m_funcBrowser->clearErrors();
  }
}

/**
 * Called when user edits a parameter in the function browser.
 * Updates the parameter value in the fit property browser.
 * @param funcIndex :: [input] index of the function
 * @param paramName :: [input] parameter name
 */
void MuonAnalysisFitFunctionHelper::handleParameterEdited(
    const QString &funcIndex, const QString &paramName) {
  const double value = m_funcBrowser->getParameter(funcIndex, paramName);
  m_fitBrowser->setParameterValue(funcIndex, paramName, value);
}

/**
 * Called when "Clear model" selected on the fit property browser.
 * Clears the function set in the function browser.
 */
void MuonAnalysisFitFunctionHelper::handleModelCleared() {
  m_funcBrowser->clear();
}

/**
 * Called when user shows/hides parameter errors.
 * Pass this change on to the function browser.
 * @param enabled :: [input] enabled/disabled state of param errors
 */
void MuonAnalysisFitFunctionHelper::handleErrorsEnabled(bool enabled) {
  m_funcBrowser->setErrorsEnabled(enabled);
}

/**
 * Called when a saved setup is loaded into the fit property browser.
 * Update the function browser with this loaded function.
 * @param funcString :: [input] Loaded function as a string
 */
void MuonAnalysisFitFunctionHelper::handleFunctionLoaded(
    const QString &funcString) {
  m_funcBrowser->clear();
  m_funcBrowser->setFunction(funcString);
}

} // namespace CustomInterfaces
} // namespace MantidQt
