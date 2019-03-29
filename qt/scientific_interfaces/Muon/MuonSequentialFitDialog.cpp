// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MuonSequentialFitDialog.h"

#include "MantidAPI/AlgorithmProxy.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtWidgets/Common/MuonFitPropertyBrowser.h"
#include "MuonAnalysisFitDataPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {
using namespace Mantid::Kernel;
using namespace Mantid::API;
using MantidQt::CustomInterfaces::MuonAnalysisFitDataPresenter;
using MantidQt::MantidWidgets::MuonFitPropertyBrowser;

namespace {
Logger g_log("MuonSequentialFitDialog");

std::string removeSubPath(const std::string &labelIn) {
  size_t path = labelIn.find_last_of("/");
  if (path == std::string::npos) {
    path = labelIn.find_last_of('\\');
  }
  std::string useThisLabel = labelIn;
  if (path != std::string::npos) {
    path = path + 1;
    size_t end = labelIn.find_last_of(".");
    useThisLabel = labelIn.substr(path);
    useThisLabel = useThisLabel.substr(0, end - path);
    size_t start = useThisLabel.find_first_of("0123456789");
    useThisLabel = useThisLabel.substr(start);
  }
  return useThisLabel;
}

std::string removePath(const std::string &labelIn) {
  std::string useThisLabel;
  std::string tmp = labelIn;
  size_t end = tmp.find_first_of(","); // always seperate by commas

  if (end != std::string::npos) {
    while (end != std::string::npos) {
      useThisLabel += removeSubPath(tmp.substr(0, end)) + ",";
      tmp = tmp.substr(end + 1);
      end = tmp.find_first_of(",");
    }
    // get the last input
    useThisLabel += removeSubPath(tmp);
    return useThisLabel;
  }
  return removeSubPath(labelIn);
}
} // namespace
const std::string MuonSequentialFitDialog::SEQUENTIAL_PREFIX("MuonSeqFit_");

/**
 * Constructor
 * @param fitPropBrowser :: [input] Pointer to fit property browser
 * @param dataPresenter :: [input] Pointer to fit data presenter
 */
MuonSequentialFitDialog::MuonSequentialFitDialog(
    MuonFitPropertyBrowser *fitPropBrowser,
    MuonAnalysisFitDataPresenter *dataPresenter)
    : QDialog(fitPropBrowser), m_fitPropBrowser(fitPropBrowser),
      m_dataPresenter(dataPresenter) {
  m_ui.setupUi(this);

  setState(Stopped);

  // Set initial run to be run number of the workspace selected in fit browser
  // when starting seq. fit dialog
  const auto fitWS = boost::dynamic_pointer_cast<const MatrixWorkspace>(
      m_fitPropBrowser->getWorkspace());
  m_ui.runs->setText(QString::number(fitWS->getRunNumber()) + "-");
  // Set the file finder to the correct instrument (not Mantid's default
  // instrument)
  const auto instName = fitWS->getInstrument()->getName();
  m_ui.runs->setInstrumentOverride(QString(instName.c_str()));

  // TODO: find a better initial one, e.g. previously used
  m_ui.labelInput->setText("Label");

  initDiagnosisTable();

  // After initial values are set, update depending elements accordingly. We
  // don't rely on
  // slot/signal update, as element might be left with default values which
  // means these will
  // never be called on initialization.
  updateLabelError(m_ui.labelInput->text());

  updateControlButtonType(m_state);
  updateInputEnabled(m_state);
  updateControlEnabled(m_state);
  updateCursor(m_state);

  connect(m_ui.labelInput, SIGNAL(textChanged(const QString &)), this,
          SLOT(updateLabelError(const QString &)));

  connect(this, SIGNAL(stateChanged(DialogState)), this,
          SLOT(updateControlButtonType(DialogState)));
  connect(this, SIGNAL(stateChanged(DialogState)), this,
          SLOT(updateInputEnabled(DialogState)));
  connect(this, SIGNAL(stateChanged(DialogState)), this,
          SLOT(updateControlEnabled(DialogState)));
  connect(this, SIGNAL(stateChanged(DialogState)), this,
          SLOT(updateCursor(DialogState)));
}

/**
 * Destructor
 */
MuonSequentialFitDialog::~MuonSequentialFitDialog() {}

/**
 * Checks if specified name is valid as a name for the label.
 * @param label :: The name to check
 * @return Empty string if valid, otherwise an error string
 */
std::string MuonSequentialFitDialog::isValidLabel(const std::string &label) {
  if (label.empty())
    return "Cannot be empty";
  else
    return AnalysisDataService::Instance().isValid(label);
}

/**
 * Returns displayable title for the given workspace;
 * @param ws :: Workspace to get title from
 * @return The title, or empty string if unable to get one
 */
std::string MuonSequentialFitDialog::getRunTitle(Workspace_const_sptr ws) {
  auto matrixWS = boost::dynamic_pointer_cast<const MatrixWorkspace>(ws);

  if (!matrixWS)
    return "";

  const std::string &instrName = matrixWS->getInstrument()->getName();
  const int runNumber = matrixWS->getRunNumber();

  if (instrName.empty() || runNumber == 0)
    return "";

  std::ostringstream runTitle;
  runTitle << instrName << runNumber;
  return runTitle.str();
}

/**
 * Initialize diagnosis table.
 */
void MuonSequentialFitDialog::initDiagnosisTable() {
  QStringList headerLabels;

  // Add two static columns
  headerLabels << "Run"
               << "Fit quality";

  // Add remaining columns - one for every fit function parameter
  IFunction_sptr fitFunc = m_fitPropBrowser->getFittingFunction();

  for (size_t i = 0; i < fitFunc->nParams(); i++) {
    QString paramName = QString::fromStdString(fitFunc->parameterName(i));
    headerLabels << paramName;
    headerLabels << paramName + "_Err";
  }

  m_ui.diagnosisTable->setColumnCount(headerLabels.size());
  m_ui.diagnosisTable->setHorizontalHeaderLabels(headerLabels);

  // Make the table fill all the available space and columns be resized to fit
  // contents
  m_ui.diagnosisTable->horizontalHeader()->setResizeMode(
      QHeaderView::ResizeToContents);

  // Make rows alternate bg colors for better user experience
  m_ui.diagnosisTable->setAlternatingRowColors(true);
}

/**
 * Add a new entry to the diagnosis table.
 * @param runTitle       :: Title of the run fitted
 * @param fitQuality     :: Number representing goodness of the fit
 * @param fittedFunction :: Function containing fitted parameters
 */
void MuonSequentialFitDialog::addDiagnosisEntry(const std::string &runTitle,
                                                double fitQuality,
                                                IFunction_sptr fittedFunction) {
  int newRow = m_ui.diagnosisTable->rowCount();

  m_ui.diagnosisTable->insertRow(newRow);

  QString runTitleDisplay = QString::fromStdString(runTitle);
  m_ui.diagnosisTable->setItem(newRow, 0,
                               createTableWidgetItem(runTitleDisplay));

  QString fitQualityDisplay = QString::number(fitQuality);
  m_ui.diagnosisTable->setItem(newRow, 1,
                               createTableWidgetItem(fitQualityDisplay));

  for (int i = 2; i < m_ui.diagnosisTable->columnCount(); i += 2) {
    std::string paramName =
        m_ui.diagnosisTable->horizontalHeaderItem(i)->text().toStdString();
    size_t paramIndex = fittedFunction->parameterIndex(paramName);

    QString value = QString::number(fittedFunction->getParameter(paramIndex));
    QString error = QString::number(fittedFunction->getError(paramIndex));

    m_ui.diagnosisTable->setItem(newRow, i, createTableWidgetItem(value));
    m_ui.diagnosisTable->setItem(newRow, i + 1, createTableWidgetItem(error));
  }

  m_ui.diagnosisTable->scrollToBottom();
}

/**
 * Helper function to create new item for Diagnosis table.
 * @return Created and initialized item with text
 */
QTableWidgetItem *
MuonSequentialFitDialog::createTableWidgetItem(const QString &text) {
  auto newItem = new QTableWidgetItem(text);
  newItem->setFlags(newItem->flags() ^ Qt::ItemIsEditable);
  return newItem;
}

/**
 * Updates visibility/tooltip of label error asterisk.
 * @param label :: New label as specified by user
 */
void MuonSequentialFitDialog::updateLabelError(const QString &label) {
  std::string error = isValidLabel(label.toStdString());

  m_ui.labelError->setVisible(!error.empty());
  m_ui.labelError->setToolTip(QString::fromStdString(error));
}

/**
 * Check if all the input fields are valid.
 * @return True if everything valid, false otherwise
 */
bool MuonSequentialFitDialog::isInputValid() {
  if (!m_ui.runs->isValid())
    return false;

  std::string label = m_ui.labelInput->text().toStdString();

  return isValidLabel(label).empty();
}

/**
 * Sets control button to be start/stop depending on new dialog state.
 * @param newState :: New state of the dialog
 */
void MuonSequentialFitDialog::updateControlButtonType(DialogState newState) {
  // Disconnect everything connected to pressed() signal of the button
  disconnect(m_ui.controlButton, SIGNAL(pressed()), nullptr, nullptr);

  // Connect to appropriate slot
  auto buttonSlot = (newState == Running) ? SLOT(stopFit()) : SLOT(startFit());
  connect(m_ui.controlButton, SIGNAL(pressed()), this, buttonSlot);

  // Set appropriate text
  QString buttonText = (newState == Running) ? "Stop" : "Start";
  m_ui.controlButton->setText(buttonText);
}

/**
 * Updates current state of the dialog.
 */
void MuonSequentialFitDialog::setState(DialogState newState) {
  m_state = newState;
  emit stateChanged(newState);
}

/**
 * Update enabled state of all the input widgets depending on new dialog state.
 * @param newState :: New state of the dialog
 */
void MuonSequentialFitDialog::updateInputEnabled(DialogState newState) {
  bool enabled = (newState == Stopped);

  m_ui.runs->setEnabled(enabled);
  m_ui.labelInput->setEnabled(enabled);

  foreach (QAbstractButton *button, m_ui.paramTypeGroup->buttons())
    button->setEnabled(enabled);
}

/**
 * Update control button enabled status depending on the new state.
 * Button is disabled in one case only - when preparing for running.
 * @param newState :: New state of the dialog
 */
void MuonSequentialFitDialog::updateControlEnabled(DialogState newState) {
  m_ui.controlButton->setEnabled(newState != Preparing);
}

/**
 * Update cursor depending on the new state of the dialog.
 * Waiting cursor is displayed while preparing so that user does know that
 * something is happening.
 * @param newState :: New state of the dialog
 */
void MuonSequentialFitDialog::updateCursor(DialogState newState) {
  switch (newState) {
  case Preparing:
    setCursor(Qt::WaitCursor);
    break;
  case Running:
    setCursor(Qt::BusyCursor);
    break;
  default:
    unsetCursor();
    break;
  }
}

/**
 * Start fitting process by running file search.
 * Once search is complete, fit continues in continueFit().
 */
void MuonSequentialFitDialog::startFit() {
  if (m_state != Stopped)
    throw std::runtime_error("Couldn't start: already running");

  setState(Preparing);

  // Explicitly run the file search. This might be needed when Start is clicked
  // straight after editing the run box. In that case, lost focus event might
  // not be processed yet and search might not have been started yet.
  // Otherwise, search is not done as the widget sees that it has not been
  // changed.
  connect(m_ui.runs, SIGNAL(fileInspectionFinished()), this,
          SLOT(continueFit()));
  if (!m_ui.runs->isSearching()) {
    m_ui.runs->findFiles();
  }
}

/**
 * Carries out the fitting process once the file search has completed.
 * Called when the run control reports that its search has finished.
 */
void MuonSequentialFitDialog::continueFit() {
  disconnect(m_ui.runs, SIGNAL(fileInspectionFinished()), this,
             SLOT(continueFit()));

  // Validate input fields
  if (!isInputValid()) {
    QMessageBox::critical(this, "Input is not valid",
                          "One or more input fields are invalid.\n\nInvalid "
                          "fields are marked with a '*'.");
    setState(Stopped);
    return;
  }

  // Get names of workspaces to fit
  const auto wsNames = m_dataPresenter->generateWorkspaceNames(
      m_ui.runs->getInstrumentOverride().toStdString(),
      removePath(m_ui.runs->getText().toStdString()), false);
  if (wsNames.size() == 0) {
    QMessageBox::critical(
        this, "No data to fit",
        "No data was found to fit (the list of workspaces to fit was empty).");
    setState(Stopped);
    return;
  }

  // Create the workspaces to fit
  m_dataPresenter->createWorkspacesToFit(wsNames);

  QStringList runFilenames = m_ui.runs->getFilenames();
  const auto numRuns = static_cast<size_t>(runFilenames.size());

  // This must divide with no remainder:
  // datasets per run = groups * periods
  assert(wsNames.size() % numRuns == 0);
  const auto datasetsPerRun = wsNames.size() / numRuns;

  const std::string label = m_ui.labelInput->text().toStdString();
  const std::string labelGroupName = SEQUENTIAL_PREFIX + label;

  AnalysisDataServiceImpl &ads = AnalysisDataService::Instance();

  if (ads.doesExist(labelGroupName)) {
    QMessageBox::StandardButton answer =
        QMessageBox::question(this, "Label already exists",
                              "Label you specified was used for one of the "
                              "previous fits. Do you want to overwrite it?",
                              QMessageBox::Yes | QMessageBox::Cancel);

    if (answer != QMessageBox::Yes) {
      setState(Stopped);
      return;
    }

    ads.deepRemoveGroup(labelGroupName);
  }

  // Create a group for label
  ads.add(labelGroupName, boost::make_shared<WorkspaceGroup>());

  // Tell progress bar how many iterations we will need to make and reset it
  m_ui.progress->setRange(0, runFilenames.size());
  m_ui.progress->setFormat("%p%");
  m_ui.progress->setValue(0);

  // Clear diagnosis table for new fit
  m_ui.diagnosisTable->setRowCount(0);

  // Get fit function as specified by user in the fit browser
  IFunction_sptr fitFunction = FunctionFactory::Instance().createInitialized(
      m_fitPropBrowser->getFittingFunction()->asString());

  // Whether we should use initial function for every fit
  bool useInitFitFunction =
      (m_ui.paramTypeGroup->checkedButton() == m_ui.paramTypeInitial);

  setState(Running);
  m_stopRequested = false;

  // For each run, fit "datasetsPerRun" groups and periods simultaneously
  for (size_t i = 0; i < numRuns; i++) {
    // Process events (so that Stop button press is processed)
    QApplication::processEvents();

    // Stop if requested by user
    if (m_stopRequested)
      break;

    // Workspaces to be fitted simultaneously for this run
    std::vector<std::string> workspacesToFit;
    const auto startIter = wsNames.begin() + i * datasetsPerRun;
    std::copy(startIter, startIter + datasetsPerRun,
              std::back_inserter(workspacesToFit));

    // Get run title. Workspaces should be in ADS
    MatrixWorkspace_sptr matrixWS;
    try {
      matrixWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          workspacesToFit.front());
    } catch (const Mantid::Kernel::Exception::NotFoundError &err) {
      QMessageBox::critical(
          this, "Data not found",
          QString::fromStdString("Workspace to fit not found in ADS: " +
                                 workspacesToFit.front() + err.what()));
      setState(Stopped);
      return;
    }
    const std::string runTitle = getRunTitle(matrixWS);
    const std::string wsBaseName = labelGroupName + "_" + runTitle;

    IFunction_sptr functionToFit;

    if (useInitFitFunction)
      // Create a copy so that the original function is not changed
      functionToFit = FunctionFactory::Instance().createInitialized(
          fitFunction->asString());
    else
      // Use the same function over and over, so that previous fitted params are
      // used for the next fit
      functionToFit = fitFunction;

    IAlgorithm_sptr fit = AlgorithmManager::Instance().create("Fit");
    fit->setRethrows(true);

    try {

      // Set function. Gets updated when fit is done.
      fit->setProperty("Function", functionToFit);

      fit->setProperty("InputWorkspace", workspacesToFit.front());
      fit->setProperty("Output", wsBaseName);

      // We should have one spectrum only in the workspace, so use the first
      // one.
      fit->setProperty("WorkspaceIndex", 0);

      // Various properties from the fit prop. browser
      fit->setProperty("StartX", m_fitPropBrowser->startX());
      fit->setProperty("EndX", m_fitPropBrowser->endX());
      fit->setProperty("Minimizer", m_fitPropBrowser->minimizer());
      fit->setProperty("CostFunction", m_fitPropBrowser->costFunction());

      // If multiple groups/periods, set up simultaneous fit
      if (datasetsPerRun > 1) {
        for (size_t j = 1; j < workspacesToFit.size(); j++) {
          std::string suffix = boost::lexical_cast<std::string>(j);
          fit->setPropertyValue("InputWorkspace_" + suffix, workspacesToFit[j]);
          fit->setProperty("WorkspaceIndex_" + suffix, 0);
          fit->setProperty("StartX_" + suffix, m_fitPropBrowser->startX());
          fit->setProperty("EndX_" + suffix, m_fitPropBrowser->endX());
        }
      }

      fit->execute();
    } catch (const std::exception &err) {
      g_log.error(err.what());
      QMessageBox::critical(
          this, "Fitting failed",
          "Unable to fit one of the files.\n\nCheck log for details");
      break;
    }

    // Copy log values and group created fit workspaces
    finishAfterRun(labelGroupName, fit, datasetsPerRun > 1, matrixWS);

    // If fit was simultaneous, transform results
    if (datasetsPerRun > 1) {
      // sub groups in `_label` mean wsBaseName is the name of the parent
      // wsGroup
      m_dataPresenter->handleFittedWorkspaces(wsBaseName);
      m_dataPresenter->extractFittedWorkspaces(wsBaseName);
    }

    // Add information about the fit to the diagnosis table
    addDiagnosisEntry(runTitle, fit->getProperty("OutputChi2OverDof"),
                      functionToFit);

    // Update progress
    m_ui.progress->setFormat("%p% - " + QString::fromStdString(runTitle));
    m_ui.progress->setValue(m_ui.progress->value() + 1);
  }

  setState(Stopped);
}

/**
 * Handle reorganising workspaces after fit of a single run has finished
 * Group output together and copy log values
 * @param labelGroupName :: [input] Label for group
 * @param fitAlg :: [input] Pointer to fit algorithm
 * @param simultaneous :: [input] Whether several groups/periods were fitted
 * simultaneously or not
 * @param firstWS :: [input] Pointer to first input workspace (to copy logs
 * from)
 */
void MuonSequentialFitDialog::finishAfterRun(
    const std::string &labelGroupName, const IAlgorithm_sptr &fitAlg,
    bool simultaneous, const MatrixWorkspace_sptr &firstWS) const {
  auto &ads = AnalysisDataService::Instance();
  const std::string wsBaseName = fitAlg->getPropertyValue("Output");
  if (simultaneous) {
    // copy logs
    auto fitWSGroup =
        ads.retrieveWS<WorkspaceGroup>(wsBaseName + "_Workspaces");
    for (size_t i = 0; i < fitWSGroup->size(); i++) {
      auto fitWs =
          boost::dynamic_pointer_cast<MatrixWorkspace>(fitWSGroup->getItem(i));
      if (fitWs) {
        fitWs->copyExperimentInfoFrom(firstWS.get());
      }
    }
    // insert workspace names into table
    try {
      const std::string paramTableName =
          fitAlg->getProperty("OutputParameters");
      const auto paramTable = ads.retrieveWS<ITableWorkspace>(paramTableName);
      if (paramTable) {
        Mantid::API::TableRow f0Row = paramTable->appendRow();
        f0Row << "f0=" + fitAlg->getPropertyValue("InputWorkspace") << 0.0
              << 0.0;
        for (size_t i = 1; i < fitWSGroup->size(); i++) {
          const std::string suffix = boost::lexical_cast<std::string>(i);
          const auto wsName =
              fitAlg->getPropertyValue("InputWorkspace_" + suffix);
          Mantid::API::TableRow row = paramTable->appendRow();
          row << "f" + suffix + "=" + wsName << 0.0 << 0.0;
        }
      }
    } catch (const Mantid::Kernel::Exception::NotFoundError &) {
      // Not a fatal error, but shouldn't happen
      g_log.warning(
          "Could not find output parameters table for simultaneous fit");
    }
    // Group output together
    ads.add(wsBaseName, boost::make_shared<WorkspaceGroup>());
    ads.addToGroup(wsBaseName, wsBaseName + "_NormalisedCovarianceMatrix");
    ads.addToGroup(wsBaseName, wsBaseName + "_Parameters");
    ads.addToGroup(wsBaseName, wsBaseName + "_Workspaces");
    ads.addToGroup(labelGroupName, wsBaseName);
  } else {
    ads.add(wsBaseName, boost::make_shared<WorkspaceGroup>());
    ads.addToGroup(wsBaseName, wsBaseName + "_NormalisedCovarianceMatrix");
    ads.addToGroup(wsBaseName, wsBaseName + "_Parameters");
    ads.addToGroup(wsBaseName, wsBaseName + "_Workspace");
    auto fitWs = ads.retrieveWS<MatrixWorkspace>(wsBaseName + "_Workspace");
    fitWs->copyExperimentInfoFrom(firstWS.get());
    ads.addToGroup(labelGroupName, wsBaseName);
  }
}

/**
 * Stop fitting process.
 */
void MuonSequentialFitDialog::stopFit() {
  if (m_state == Stopped)
    throw std::runtime_error("Couldn't stop: is not running");

  m_stopRequested = true;
}

} // namespace CustomInterfaces
} // namespace MantidQt
