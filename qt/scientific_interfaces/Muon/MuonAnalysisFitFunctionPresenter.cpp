// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MuonAnalysisFitFunctionPresenter.h"
#include "MantidAPI/IFunction.h"
#include "MantidQtWidgets/Common/EditLocalParameterDialog.h"

using MantidQt::MantidWidgets::EditLocalParameterDialog;
using MantidQt::MantidWidgets::IFunctionBrowser;
using MantidQt::MantidWidgets::IMuonFitFunctionModel;

namespace MantidQt {
namespace CustomInterfaces {

/**
 * Constructor
 * @param parent :: [input] Parent dialog (MuonAnalysis)
 * @param fitBrowser :: [input] Non-owning pointer to muon fit property browser
 * @param funcBrowser :: [input] Non-owning pointer to function browser
 */
MuonAnalysisFitFunctionPresenter::MuonAnalysisFitFunctionPresenter(
    QObject *parent, IMuonFitFunctionModel *fitBrowser,
    IFunctionBrowser *funcBrowser)
    : QObject(parent), m_fitBrowser(fitBrowser), m_funcBrowser(funcBrowser),
      m_multiFitState(Muon::MultiFitState::Disabled) {
  doConnect();
}

/**
 * Connect up signals and slots
 * Abstract base class is not a QObject, so attempt a cast.
 * (Its derived classes are QObjects).
 */
void MuonAnalysisFitFunctionPresenter::doConnect() {
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
    connect(fitBrowser, SIGNAL(workspacesToFitChanged(int)), this,
            SLOT(updateNumberOfDatasets(int)));
    connect(fitBrowser, SIGNAL(userChangedDatasetIndex(int)), this,
            SLOT(handleDatasetIndexChanged(int)));
  }
  setParameterUpdates(true);
}

/**
 * Switch signals on/off for updating the function browser
 * @param on :: [input] On/off for signals and slots
 */
void MuonAnalysisFitFunctionPresenter::setParameterUpdates(bool on) {
  if (const QObject *funcBrowser = dynamic_cast<QObject *>(m_funcBrowser)) {
    if (on) {
      connect(funcBrowser, SIGNAL(functionStructureChanged()), this,
              SLOT(updateFunction()));
      connect(funcBrowser,
              SIGNAL(parameterChanged(const QString &, const QString &)), this,
              SLOT(handleParameterEdited(const QString &, const QString &)));
      connect(funcBrowser, SIGNAL(localParameterButtonClicked(const QString &)),
              this, SLOT(editLocalParameterClicked(const QString &)));
    } else {
      disconnect(funcBrowser, SIGNAL(functionStructureChanged()), this,
                 SLOT(updateFunction()));
      disconnect(funcBrowser,
                 SIGNAL(parameterChanged(const QString &, const QString &)),
                 this,
                 SLOT(handleParameterEdited(const QString &, const QString &)));
      disconnect(funcBrowser,
                 SIGNAL(localParameterButtonClicked(const QString &)), this,
                 SLOT(editLocalParameterClicked(const QString &)));
    }
  }
}

/**
 * Queries function browser and updates function in fit property browser.
 */
void MuonAnalysisFitFunctionPresenter::updateFunction() {
  // Check there is still a function to update
  const auto funcString = m_funcBrowser->getFunctionString();
  const Mantid::API::IFunction_sptr function =
      funcString.isEmpty() ? nullptr // last function has been removed
                           : m_funcBrowser->getGlobalFunction();
  setFunctionInModel(function);
}

/**
 * Called when a fit is requested.
 * Queries function browser and updates function in fit property browser.
 * (No update if multiple fitting mode is disabled, as then there is no function
 * browser).
 * Then calls fit or sequential fit as controlled by argument.
 * @param sequential :: [input] Whether a regular or sequential fit was
 * requested.
 */
void MuonAnalysisFitFunctionPresenter::updateFunctionAndFit(bool sequential) {
  // Update function, if there is a function browser
  if (m_multiFitState == Muon::MultiFitState::Enabled) {
    updateFunction();
  }
  // Run fit
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
 * (No update is performed if multiple fitting is not enabled, as function
 * browser is hidden).
 * @param wsName :: [input] workspace name - empty if fit undone
 */
void MuonAnalysisFitFunctionPresenter::handleFitFinished(
    const QString &wsName) {
  // Don't update if the function browser is hidden
  if (m_multiFitState == Muon::MultiFitState::Enabled) {
    const auto function = m_fitBrowser->getFunction();
    // We are updating function browser from fit browser, so turn off updates to
    // fit browser when function browser is updated...
    setParameterUpdates(false);
    m_funcBrowser->updateMultiDatasetParameters(*function);
    setParameterUpdates(true); // reset signals and slots
  }
  if (wsName.isEmpty()) {
    // No fitted workspace: a fit was undone so clear the errors
    m_funcBrowser->clearErrors();
  }
}

/**
 * Called when user edits a parameter in the function browser.
 * Updates the parameter value in the fit property browser.
 *
 * The whole function is updated, so that the function in the fit property
 * browser matches that in the function browser.
 *
 * @param funcIndex :: [input] index of the function (unused)
 * @param paramName :: [input] parameter name (unused)
 */
void MuonAnalysisFitFunctionPresenter::handleParameterEdited(
    const QString &funcIndex, const QString &paramName) {
  Q_UNUSED(funcIndex);
  Q_UNUSED(paramName);
  updateFunction();
}

/**
 * Called when "Clear model" selected on the fit property browser.
 * Clears the function set in the function browser.
 */
void MuonAnalysisFitFunctionPresenter::handleModelCleared() {
  m_funcBrowser->clear();
}

/**
 * Called when user shows/hides parameter errors.
 * Pass this change on to the function browser.
 * @param enabled :: [input] enabled/disabled state of param errors
 */
void MuonAnalysisFitFunctionPresenter::handleErrorsEnabled(bool enabled) {
  m_funcBrowser->setErrorsEnabled(enabled);
}

/**
 * Called when the number of datasets to fit is changed in the model.
 * Update the view with the new number of datasets.
 *
 * Clear errors in function browser as the data being fitted has changed, so
 * these errors are now stale.
 *
 * @param nDatasets :: [input] Number of datasets to fit
 */
void MuonAnalysisFitFunctionPresenter::updateNumberOfDatasets(int nDatasets) {
  m_funcBrowser->clearErrors();
  m_funcBrowser->setNumberOfDatasets(nDatasets);
  // get names of workspaces
  QStringList wsNames;
  for (const auto &name : m_fitBrowser->getWorkspaceNamesToFit()) {
    wsNames.append(QString::fromStdString(name));
  }
  m_funcBrowser->setDatasetNames(wsNames);
}

/**
 * Called when user changes selected dataset.
 * Update current dataset in function browser.
 * @param index :: [input] Selected dataset index
 */
void MuonAnalysisFitFunctionPresenter::handleDatasetIndexChanged(int index) {
  // Avoid signals being sent to fit browser while this changes
  setParameterUpdates(false);
  m_funcBrowser->setCurrentDataset(index);
  setParameterUpdates(true);
}

/**
 * Turn multiple fitting mode on/off.
 * Turning it off hides the function browser and data selector so that
 * the fitting works as it used to pre-Mantid 3.8.
 * @param state :: [input] On/off for multiple fitting mode.
 */
void MuonAnalysisFitFunctionPresenter::setMultiFitState(
    Muon::MultiFitState state) {
  m_fitBrowser->setMultiFittingMode(state == Muon::MultiFitState::Enabled);
  m_multiFitState = state;
}

/**
 * Set the given function in the model (fit property browser).
 *
 * If and only if multi fit mode is enabled, need to deal with plot guess too.
 * Remove the guess (if one is plotted) before updating the function, then
 * replot if necessary afterwards.
 * This ensures the guess is updated and prevents stale guesses in the plot.
 *
 * If multi fit mode is off, the user sets the function in the model directly so
 * it works fine as is - no need to update the guess as it already happens.
 *
 * @param function :: [input] Function to set in the model
 */
void MuonAnalysisFitFunctionPresenter::setFunctionInModel(
    const Mantid::API::IFunction_sptr &function) {
  const bool updateGuess = m_multiFitState == Muon::MultiFitState::Enabled &&
                           m_fitBrowser->hasGuess();
  if (updateGuess) {
    m_fitBrowser->doRemoveGuess();
  }
  m_fitBrowser->setFunction(function);
  if (updateGuess) {
    m_fitBrowser->doPlotGuess();
  }
}

} // namespace CustomInterfaces
} // namespace MantidQt
