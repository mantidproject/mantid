// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "EnggDiffFittingPresenter.h"
#include "EnggDiffFittingPresWorker.h"
#include "IEnggDiffFittingModel.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidQtWidgets/LegacyQwt/QwtHelper.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <cctype>
#include <fstream>

#include <Poco/DirectoryIterator.h>
#include <Poco/File.h>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;

namespace MantidQt {
namespace QwtHelper = API::QwtHelper;
namespace CustomInterfaces {

namespace {
Mantid::Kernel::Logger g_log("EngineeringDiffractionGUI");

RunLabel runLabelFromListWidgetLabel(const std::string &listLabel) {
  const size_t underscorePosition = listLabel.find_first_of("_");
  const auto runNumber = listLabel.substr(0, underscorePosition);
  const auto bank = listLabel.substr(underscorePosition + 1);

  return RunLabel(std::atoi(runNumber.c_str()), std::atoi(bank.c_str()));
}

std::string listWidgetLabelFromRunLabel(const RunLabel &runLabel) {
  return std::to_string(runLabel.runNumber) + "_" +
         std::to_string(runLabel.bank);
}

// Remove commas at the start and end of the string,
// as well as any adjacent to another (eg ,, gets corrected to ,)
std::string stripExtraCommas(std::string &expectedPeaks) {
  if (!expectedPeaks.empty()) {

    g_log.debug() << "Validating the expected peak list.\n";

    const auto comma = ',';

    for (size_t i = 0; i < expectedPeaks.size() - 1; i++) {
      size_t j = i + 1;

      if (expectedPeaks[i] == comma && expectedPeaks[i] == expectedPeaks[j]) {
        expectedPeaks.erase(j, 1);
        i--;

      } else {
        ++j;
      }
    }

    size_t strLength = expectedPeaks.length() - 1;
    if (expectedPeaks.at(0) == ',') {
      expectedPeaks.erase(0, 1);
      strLength -= 1;
    }

    if (expectedPeaks.at(strLength) == ',') {
      expectedPeaks.erase(strLength, 1);
    }
  }
  return expectedPeaks;
}

std::string generateXAxisLabel(Mantid::Kernel::Unit_const_sptr unit) {
  std::string label = unit->unitID();
  if (label == "TOF") {
    label += " (us)";
  } else if (label == "dSpacing") {
    label += " (A)";
  }
  return label;
}
} // namespace

/**
 * Constructs a presenter for a fitting tab/widget view, which has a
 * handle on the current calibration (produced and updated elsewhere).
 *
 * @param view the view that is attached to this presenter
 * @param model the model that is attached to this presenter
 * @param mainCalib provides the current calibration parameters/status
 * @param mainParam provides current params and functions
 */
EnggDiffFittingPresenter::EnggDiffFittingPresenter(
    IEnggDiffFittingView *view, std::unique_ptr<IEnggDiffFittingModel> model,
    boost::shared_ptr<IEnggDiffractionCalibration> mainCalib,
    boost::shared_ptr<IEnggDiffractionParam> mainParam)
    : m_fittingFinishedOK(false), m_workerThread(nullptr),
      m_mainCalib(mainCalib), m_mainParam(mainParam), m_view(view),
      m_model(std::move(model)), m_viewHasClosed(false) {}

EnggDiffFittingPresenter::~EnggDiffFittingPresenter() { cleanup(); }

/**
 * Close open sessions, kill threads etc., for a graceful window
 * close/destruction
 */
void EnggDiffFittingPresenter::cleanup() {
  // m_model->cleanup();

  // this may still be running
  if (m_workerThread) {
    if (m_workerThread->isRunning()) {
      g_log.notice() << "A fitting process is currently running, shutting "
                        "it down immediately...\n";
      m_workerThread->wait(10);
    }
    delete m_workerThread;
    m_workerThread = nullptr;
  }
}

void EnggDiffFittingPresenter::notify(
    IEnggDiffFittingPresenter::Notification notif) {

  // Check the view is valid - QT can send multiple notification
  // signals in any order at any time. This means that it is possible
  // to receive a shutdown signal and subsequently an input example
  // for example. As we can't guarantee the state of the viewer
  // after calling shutdown instead we shouldn't do anything after
  if (m_viewHasClosed) {
    return;
  }

  switch (notif) {

  case IEnggDiffFittingPresenter::Start:
    processStart();
    break;

  case IEnggDiffFittingPresenter::Load:
    processLoad();
    break;

  case IEnggDiffFittingPresenter::FitPeaks:
    processFitPeaks();
    break;

  case IEnggDiffFittingPresenter::FitAllPeaks:
    processFitAllPeaks();
    break;

  case IEnggDiffFittingPresenter::addPeaks:
    addPeakToList();
    break;

  case IEnggDiffFittingPresenter::browsePeaks:
    browsePeaksToFit();
    break;

  case IEnggDiffFittingPresenter::savePeaks:
    savePeakList();
    break;

  case IEnggDiffFittingPresenter::ShutDown:
    processShutDown();
    break;

  case IEnggDiffFittingPresenter::LogMsg:
    processLogMsg();
    break;

  case IEnggDiffFittingPresenter::selectRun:
    processSelectRun();
    break;

  case IEnggDiffFittingPresenter::updatePlotFittedPeaks:
    processUpdatePlotFitPeaks();
    break;

  case IEnggDiffFittingPresenter::removeRun:
    processRemoveRun();
    break;
  }
}

std::vector<GSASCalibrationParms>
EnggDiffFittingPresenter::currentCalibration() const {
  return m_mainCalib->currentCalibration();
}

Poco::Path
EnggDiffFittingPresenter::outFilesUserDir(const std::string &addToDir) const {
  return m_mainParam->outFilesUserDir(addToDir);
}

std::string
EnggDiffFittingPresenter::userHDFRunFilename(const int runNumber) const {
  return m_mainParam->userHDFRunFilename(runNumber);
}

std::string EnggDiffFittingPresenter::userHDFMultiRunFilename(
    const std::vector<RunLabel> &runLabels) const {
  return m_mainParam->userHDFMultiRunFilename(runLabels);
}

void EnggDiffFittingPresenter::startAsyncFittingWorker(
    const std::vector<RunLabel> &runLabels, const std::string &expectedPeaks) {

  delete m_workerThread;
  m_workerThread = new QThread(this);
  EnggDiffFittingWorker *worker =
      new EnggDiffFittingWorker(this, runLabels, expectedPeaks);
  worker->moveToThread(m_workerThread);

  connect(m_workerThread, SIGNAL(started()), worker, SLOT(fitting()));
  connect(worker, SIGNAL(finished()), this, SLOT(fittingFinished()));
  // early delete of thread and worker
  connect(m_workerThread, SIGNAL(finished()), m_workerThread,
          SLOT(deleteLater()), Qt::DirectConnection);
  connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
  m_workerThread->start();
}

/**
 * Takes a full file path as a string and attempts to get the base name
 * of the file at that location and return it
 *
 * @param filePath The full path to get the basename of
 *
 * @return The base name (without ext) of the file
 */
std::string EnggDiffFittingPresenter::getBaseNameFromStr(
    const std::string &filePath) const {
  Poco::Path pocoPath = filePath;
  return pocoPath.getBaseName();
}

void EnggDiffFittingPresenter::fittingFinished() {
  if (!m_view)
    return;

  if (m_fittingFinishedOK) {

    g_log.notice() << "The single peak fitting finished - the output "
                      "workspace is ready.\n";

    m_view->showStatus("Single peak fitting process finished. Ready");

    if (!m_view->listWidgetHasSelectedRow()) {
      m_view->setFittingListWidgetCurrentRow(0);
    }

    m_model->addAllFitResultsToADS();
    m_model->addAllFittedPeaksToADS();

    try {
      // should now plot the focused workspace when single peak fitting
      // process fails
      plotAlignedWorkspace(m_view->plotFittedPeaksEnabled());

    } catch (std::runtime_error &re) {
      g_log.error() << "Unable to finish the plotting of the graph for "
                       "engggui_fitting_focused_fitpeaks workspace. Error "
                       "description: " +
                           static_cast<std::string>(re.what()) +
                           " Please check also the log message for detail.";
    }
    g_log.notice() << "EnggDiffraction GUI: plotting of peaks for single peak "
                      "fits has completed. \n";

    if (m_workerThread) {
      delete m_workerThread;
      m_workerThread = nullptr;
    }

  } else {
    // Fitting failed log and tidy up
    g_log.warning() << "The single peak fitting did not finish correctly. "
                       "Please check a focused file was selected.";
    if (m_workerThread) {
      delete m_workerThread;
      m_workerThread = nullptr;
    }

    m_view->showStatus(
        "Single peak fitting process did not complete successfully");
  }
  // enable the GUI
  m_view->enableFitAllButton(m_model->getNumFocusedWorkspaces() > 1);
  m_view->enableCalibrateFocusFitUserActions(true);
}

void EnggDiffFittingPresenter::processSelectRun() { updatePlot(); }

void EnggDiffFittingPresenter::processStart() {}

void EnggDiffFittingPresenter::processLoad() {
  const std::string filenames = m_view->getFocusedFileNames();
  if (filenames.empty()) {
    m_view->userWarning("No file selected", "Please enter filename(s) to load");
    return;
  }

  try {
    m_model->loadWorkspaces(filenames);
  } catch (Poco::PathSyntaxException &ex) {
    warnFileNotFound(ex);
    return;
  } catch (std::invalid_argument &ex) {
    warnFileNotFound(ex);
    return;
  } catch (Mantid::Kernel::Exception::NotFoundError &ex) {
    warnFileNotFound(ex);
    return;
  }

  const auto runLabels = m_model->getRunLabels();
  std::vector<std::string> listWidgetLabels;
  std::transform(runLabels.begin(), runLabels.end(),
                 std::back_inserter(listWidgetLabels),
                 [](const RunLabel &runLabel) {
                   return listWidgetLabelFromRunLabel(runLabel);
                 });
  m_view->enableFittingListWidget(true);
  m_view->updateFittingListWidget(listWidgetLabels);

  m_view->enableFitAllButton(m_model->getNumFocusedWorkspaces() > 1);
}

void EnggDiffFittingPresenter::processShutDown() {
  m_viewHasClosed = true;
  m_view->saveSettings();
  cleanup();
}

void EnggDiffFittingPresenter::processLogMsg() {
  std::vector<std::string> msgs = m_view->logMsgs();
  for (const auto &msg : msgs) {
    g_log.information() << msg << '\n';
  }
}

void EnggDiffFittingPresenter::processUpdatePlotFitPeaks() { updatePlot(); }

void EnggDiffFittingPresenter::processRemoveRun() {
  const auto workspaceLabel = m_view->getFittingListWidgetCurrentValue();

  if (workspaceLabel) {
    const auto runLabel = runLabelFromListWidgetLabel(*workspaceLabel);
    m_model->removeRun(runLabel);

    const auto runLabels = m_model->getRunLabels();
    std::vector<std::string> listWidgetLabels;
    std::transform(runLabels.begin(), runLabels.end(),
                   std::back_inserter(listWidgetLabels),
                   [](const RunLabel &runLabel) {
                     return listWidgetLabelFromRunLabel(runLabel);
                   });
    m_view->updateFittingListWidget(listWidgetLabels);
  } else {
    m_view->userWarning("No run selected",
                        "Tried to remove run but no run was selected.\n"
                        "Please select a run and try again");
  }
}

void EnggDiffFittingPresenter::processFitAllPeaks() {
  std::string fittingPeaks = m_view->getExpectedPeaksInput();

  const std::string normalisedPeakCentres = stripExtraCommas(fittingPeaks);
  m_view->setPeakList(normalisedPeakCentres);

  const auto runLabels = m_model->getRunLabels();

  g_log.debug() << "Focused files found are: " << normalisedPeakCentres << '\n';
  for (const auto &runLabel : runLabels) {
    g_log.debug() << listWidgetLabelFromRunLabel(runLabel) << '\n';
  }

  if (!runLabels.empty()) {

    for (const auto &runLabel : runLabels) {
      try {
        validateFittingInputs(m_model->getWorkspaceFilename(runLabel),
                              normalisedPeakCentres);
      } catch (std::invalid_argument &ia) {
        m_view->userWarning("Error in the inputs required for fitting",
                            ia.what());
        return;
      }
    }

    g_log.notice() << "EnggDiffraction GUI: starting new multi-run "
                   << "single peak fits. This may take some seconds...\n";
    m_view->showStatus("Fitting multi-run single peaks...");

    // disable GUI to avoid any double threads
    m_view->enableCalibrateFocusFitUserActions(false);
    m_view->enableFitAllButton(false);

    startAsyncFittingWorker(runLabels, normalisedPeakCentres);
  } else {
    m_view->userWarning("Error in the inputs required for fitting",
                        "No runs were loaded for fitting");
  }
}

void EnggDiffFittingPresenter::processFitPeaks() {
  const auto listLabel = m_view->getFittingListWidgetCurrentValue();

  if (!listLabel) {
    m_view->userWarning("No run selected",
                        "Please select a run to fit from the list");
    return;
  }

  const auto runLabel = runLabelFromListWidgetLabel(*listLabel);
  std::string fittingPeaks = m_view->getExpectedPeaksInput();

  const std::string normalisedPeakCentres = stripExtraCommas(fittingPeaks);
  m_view->setPeakList(normalisedPeakCentres);

  g_log.debug() << "the expected peaks are: " << normalisedPeakCentres << '\n';

  const auto filename = m_model->getWorkspaceFilename(runLabel);
  try {
    validateFittingInputs(filename, normalisedPeakCentres);
  } catch (std::invalid_argument &ia) {
    m_view->userWarning("Error in the inputs required for fitting", ia.what());
    return;
  }

  // disable so that user is forced to select file again
  // otherwise empty vector will be passed
  m_view->enableFitAllButton(false);

  const std::string outWSName = "engggui_fitting_fit_peak_ws";
  g_log.notice() << "EnggDiffraction GUI: starting new "
                 << "single peak fits into workspace '" << outWSName
                 << "'. This may take some seconds... \n";

  m_view->showStatus("Fitting single peaks...");
  // disable GUI to avoid any double threads
  m_view->enableCalibrateFocusFitUserActions(false);

  startAsyncFittingWorker({runLabel}, normalisedPeakCentres);
}

void EnggDiffFittingPresenter::validateFittingInputs(
    const std::string &focusedRunFilename, const std::string &expectedPeaks) {
  if (focusedRunFilename.empty()) {
    throw std::invalid_argument(
        "Focused run filename cannot be empty and must be a valid file");
  }

  Poco::File file(focusedRunFilename);
  if (!file.exists()) {
    throw std::invalid_argument("The focused workspace file for single peak "
                                "fitting could not be found: " +
                                focusedRunFilename);
  }

  if (expectedPeaks.empty()) {
    g_log.warning() << "Expected peaks were not passed, via fitting interface, "
                       "the default list of "
                       "expected peaks will be utilised instead.\n";
  }
  bool contains_non_digits =
      expectedPeaks.find_first_not_of("0123456789,. ") != std::string::npos;
  if (contains_non_digits) {
    throw std::invalid_argument("The expected peaks provided " + expectedPeaks +
                                " is invalid, "
                                "fitting process failed. Please try again!");
  }
}

void EnggDiffFittingPresenter::doFitting(const std::vector<RunLabel> &runLabels,
                                         const std::string &expectedPeaks) {
  m_fittingFinishedOK = false;

  for (const auto &runLabel : runLabels) {
    g_log.notice() << "EnggDiffraction GUI: starting new fitting with run "
                   << runLabel.runNumber << " and bank " << runLabel.bank
                   << ". This may take a few seconds... \n";

    // apply calibration to the focused workspace
    m_model->setDifcTzero(runLabel, currentCalibration());

    // run the algorithm EnggFitPeaks with workspace loaded above
    // requires unit in Time of Flight
    try {
      m_model->enggFitPeaks(runLabel, expectedPeaks);
    } catch (const std::runtime_error &exc) {
      g_log.error() << "Could not run the algorithm EnggFitPeaks successfully."
                    << exc.what();
      // A userError should be used for this message once the threading has been
      // looked into
      return;
    }

    const auto outFilename = userHDFRunFilename(runLabel.runNumber);
    m_model->saveFitResultsToHDF5({runLabel}, outFilename);

    m_model->createFittedPeaksWS(runLabel);
  }

  if (runLabels.size() > 1) {
    m_model->saveFitResultsToHDF5(runLabels,
                                  userHDFMultiRunFilename(runLabels));
  }
  m_fittingFinishedOK = true;
}

void EnggDiffFittingPresenter::browsePeaksToFit() {
  try {
    const auto &userDir = outFilesUserDir("");
    std::string path = m_view->getOpenFile(userDir.toString());
    if (path.empty()) {
      return;
    }

    m_view->setPreviousDir(path);
    std::string peaksData = readPeaksFile(path);
    m_view->setPeakList(peaksData);

  } catch (std::runtime_error &re) {
    m_view->userWarning(
        "Unable to import the peaks from a file: ",
        "File corrupted or could not be opened. Please try again" +
            static_cast<std::string>(re.what()) + '\n');
    return;
  }
}

void EnggDiffFittingPresenter::addPeakToList() {

  if (m_view->peakPickerEnabled()) {
    auto peakCentre = m_view->getPeakCentre();

    std::stringstream stream;
    stream << std::fixed << std::setprecision(4) << peakCentre;
    auto strPeakCentre = stream.str();

    auto curExpPeaksList = m_view->getExpectedPeaksInput();

    std::string comma = ",";

    if (!curExpPeaksList.empty()) {
      // when further peak added to list

      std::string lastTwoChr =
          curExpPeaksList.substr(curExpPeaksList.size() - 2);
      auto lastChr = curExpPeaksList.back();
      if (lastChr == ',' || lastTwoChr == ", ") {
        curExpPeaksList.append(strPeakCentre);
      } else {
        curExpPeaksList.append(comma + strPeakCentre);
      }
      m_view->setPeakList(curExpPeaksList);
    } else {
      // when new peak given when list is empty
      curExpPeaksList.append(strPeakCentre);
      curExpPeaksList.append(comma);
      m_view->setPeakList(curExpPeaksList);
    }
  }
}

void EnggDiffFittingPresenter::savePeakList() {
  try {
    const auto &userDir = outFilesUserDir("");
    const auto &path = m_view->getSaveFile(userDir.toString());

    if (path.empty()) {
      return;
    }

    fittingWriteFile(path);
  } catch (std::runtime_error &re) {
    m_view->userWarning(
        "Unable to save the peaks file: ",
        "Invalid file path or could not be saved. Error description : " +
            static_cast<std::string>(re.what()) + '\n');
    return;
  }
}

std::string EnggDiffFittingPresenter::readPeaksFile(std::string fileDir) {
  std::string fileData = "";
  std::string line;
  std::string comma = ", ";

  std::ifstream peakFile(fileDir);

  if (peakFile.is_open()) {
    while (std::getline(peakFile, line)) {
      fileData += line;
      if (!peakFile.eof())
        fileData += comma;
    }
    peakFile.close();
  }

  else
    fileData = "";

  return fileData;
}

void EnggDiffFittingPresenter::fittingWriteFile(const std::string &fileDir) {
  std::ofstream outfile(fileDir.c_str());
  if (!outfile) {
    m_view->userWarning("File not found",
                        "File " + fileDir +
                            " , could not be found. Please try again!");
  } else {
    auto expPeaks = m_view->getExpectedPeaksInput();
    outfile << expPeaks;
  }
}

void EnggDiffFittingPresenter::updatePlot() {
  const auto listLabel = m_view->getFittingListWidgetCurrentValue();
  if (listLabel) {
    const auto runLabel = runLabelFromListWidgetLabel(*listLabel);

    const bool fitResultsExist = m_model->hasFittedPeaksForRun(runLabel);
    const bool plotFittedPeaksEnabled = m_view->plotFittedPeaksEnabled();

    if (fitResultsExist) {
      plotAlignedWorkspace(plotFittedPeaksEnabled);
    } else {
      if (plotFittedPeaksEnabled) {
        m_view->userWarning("Cannot plot fitted peaks",
                            "Cannot plot fitted peaks, as none have been "
                            "generated by a fit. Plotting focused workspace "
                            "instead.");
      }
      const auto ws = m_model->getFocusedWorkspace(runLabel);
      plotFocusedFile(false, ws);
    }
  }
}

bool EnggDiffFittingPresenter::isDigit(const std::string &text) const {
  return std::all_of(text.cbegin(), text.cend(), ::isdigit);
}

void EnggDiffFittingPresenter::warnFileNotFound(const std::exception &ex) {
  m_view->showStatus("Error while loading focused run");
  m_view->userWarning("Invalid file selected",
                      "Mantid could not load the selected file. "
                      "Are you sure it exists? "
                      "See the logger for more information");
  g_log.error("Failed to load file. Error message: ");
  g_log.error(ex.what());
}

void EnggDiffFittingPresenter::plotFocusedFile(
    bool plotSinglePeaks, MatrixWorkspace_sptr focusedPeaksWS) {

  try {
    auto focusedData = QwtHelper::curveDataFromWs(focusedPeaksWS);

    // Check that the number of curves to plot isn't excessive
    // lets cap it at 20 to begin with - this number could need
    // raising but each curve creates about ~5 calls on the stack
    // so keep the limit low. This will stop users using unfocused
    // files which have 200+ curves to plot and will "freeze" Mantid
    constexpr int maxCurves = 20;

    if (focusedData.size() > maxCurves) {
      throw std::invalid_argument("Too many curves to plot."
                                  " Is this a focused file?");
    }

    m_view->setDataVector(
        focusedData, true, plotSinglePeaks,
        generateXAxisLabel(focusedPeaksWS->getAxis(0)->unit()));

  } catch (std::runtime_error &re) {
    g_log.error()
        << "Unable to plot focused workspace on the canvas. "
        << "Error description: " << re.what()
        << " Please check also the previous log messages for details.";

    m_view->showStatus("Error while plotting the peaks fitted");
    throw;
  }
}

void EnggDiffFittingPresenter::plotAlignedWorkspace(
    const bool plotFittedPeaks) {
  try {

    // detaches previous plots from canvas
    m_view->resetCanvas();

    const auto listLabel = m_view->getFittingListWidgetCurrentValue();
    if (!listLabel) {
      m_view->userWarning("Invalid run number or bank",
                          "Tried to plot a focused file which does not exist");
      return;
    }

    const auto runLabel = runLabelFromListWidgetLabel(*listLabel);
    const auto ws = m_model->getAlignedWorkspace(runLabel);

    // plots focused workspace
    plotFocusedFile(m_fittingFinishedOK, ws);

    if (plotFittedPeaks) {
      g_log.debug() << "single peaks fitting being plotted now.\n";
      auto singlePeaksWS = m_model->getFittedPeaksWS(runLabel);
      auto singlePeaksData = QwtHelper::curveDataFromWs(singlePeaksWS);
      m_view->setDataVector(singlePeaksData, false, true,
                            generateXAxisLabel(ws->getAxis(0)->unit()));
      m_view->showStatus("Peaks fitted successfully");
    }
  } catch (std::runtime_error) {
    g_log.error()
        << "Unable to finish of the plotting of the graph for "
           "engggui_fitting_focused_fitpeaks  workspace. Error "
           "description. Please check also the log message for detail.";

    m_view->showStatus("Error while plotting the peaks fitted");
    throw;
  }
}

} // namespace CustomInterfaces
} // namespace MantidQt
