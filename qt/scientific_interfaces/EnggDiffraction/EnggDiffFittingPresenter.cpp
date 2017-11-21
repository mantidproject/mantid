#include "EnggDiffFittingPresenter.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidQtWidgets/LegacyQwt/QwtHelper.h"
#include "EnggDiffFittingPresWorker.h"

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

std::pair<int, size_t> runAndBankNumberFromListWidgetLabel(const std::string &listLabel) {
	const size_t underscorePosition = listLabel.find_first_of("_");
	const auto runNumber = listLabel.substr(0, underscorePosition);
	const auto bank = listLabel.substr(underscorePosition + 1);
	
	return std::pair<int, size_t>(std::atoi(runNumber.c_str()), 
		                          std::atoi(bank.c_str()));
}

std::string listWidgetLabelFromRunAndBankNumber(const int runNumber, const size_t bank) {
	return std::to_string(runNumber) + "_" + std::to_string(bank);
}

}

const bool EnggDiffFittingPresenter::g_useAlignDetectors = true;

int EnggDiffFittingPresenter::g_fitting_runno_counter = 0;

/**
 * Constructs a presenter for a fitting tab/widget view, which has a
 * handle on the current calibration (produced and updated elsewhere).
 *
 * @param view the view that is attached to this presenter
 * @param mainCalib provides the current calibration parameters/status
 * @param mainParam provides current params and functions
 */
EnggDiffFittingPresenter::EnggDiffFittingPresenter(
    IEnggDiffFittingView *view,
    boost::shared_ptr<IEnggDiffractionCalibration> mainCalib,
    boost::shared_ptr<IEnggDiffractionParam> mainParam)
    : m_fittingFinishedOK(false), m_workerThread(nullptr),
      m_mainCalib(mainCalib), m_mainParam(mainParam), m_view(view),
      m_viewHasClosed(false) {}

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

  case IEnggDiffFittingPresenter::FittingRunNo:
    fittingRunNoChanged();
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
  }
}

std::vector<GSASCalibrationParms>
EnggDiffFittingPresenter::currentCalibration() const {
  return m_mainCalib->currentCalibration();
}

Poco::Path
EnggDiffFittingPresenter::outFilesUserDir(const std::string &addToDir) {
  return m_mainParam->outFilesUserDir(addToDir);
}

void EnggDiffFittingPresenter::startAsyncFittingWorker(
    const int runNumber, const size_t bank, const std::string &expectedPeaks) {

  delete m_workerThread;
  m_workerThread = new QThread(this);
  EnggDiffFittingWorker *worker =
      new EnggDiffFittingWorker(this, runNumber, bank, expectedPeaks);
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

    try {
      // should now plot the focused workspace when single peak fitting
      // process fails
      const auto listLabel = m_view->getFittingListWidgetCurrentValue();
	  int runNumber;
	  size_t bank;
	  std::tie(runNumber, bank) = runAndBankNumberFromListWidgetLabel(listLabel);

      plotFitPeaksCurves();

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
  // Reset once whole process is completed
  g_multi_run.clear();
  // enable the GUI
  m_view->enableCalibrateFocusFitUserActions(true);
}

// Fitting Tab Run Number & Bank handling here
void EnggDiffFittingPresenter::fittingRunNoChanged() {

  // receive the run number from the text-field
  const std::string userPathInput = m_view->getFittingRunNo();

  if (m_previousInput == userPathInput || userPathInput.empty()) {
    // Short circuit the checks and skip any warnings
    // or errors as the user has not changed anything
    // just clicked the box. Additionally this resolves an
    // issue where QT will return the cursor and produce a new
    // warning when the current warning is closed
    return;
  } else {
    m_previousInput = userPathInput;
  }

  // file name
  const Poco::Path pocoUserPathInput(userPathInput);

  std::vector<std::string> foundFullFilePaths;

  // returns empty if no directory is found
  const std::string parsedUserInput = pocoUserPathInput.toString();

  // split directory if 'ENGINX_' found by '_.'
  std::vector<std::string> splitBaseName;
  if (parsedUserInput.find(m_view->getCurrentInstrument() + "_") !=
      std::string::npos) {
    boost::split(splitBaseName, parsedUserInput, boost::is_any_of("_."));
  }

  try {
    // if input file is a directory and successfully splitBaseName
    // or when default bank is set or changed, the text-field is updated with
    // selected bank directory which would trigger this function again
    if (pocoUserPathInput.isFile() && !splitBaseName.empty()) {
      foundFullFilePaths =
          processFullPathInput(pocoUserPathInput, splitBaseName);
      // if given a multi-run
    } else if (userPathInput.find("-") != std::string::npos) {
      foundFullFilePaths = processMultiRun(userPathInput);
      // try to process using single run
    } else {
      foundFullFilePaths = processSingleRun(userPathInput, splitBaseName);
    }
  } catch (std::invalid_argument &ia) {
    // If something went wrong stop and print error only
    g_log.error("Failed to process user input. Error was: ");
    g_log.error(ia.what());
    return;
  }

  // if single or multi run-number
  // set the text-field to directory here to the first in
  // the vector if its not empty
  if (foundFullFilePaths.empty()) {
    m_view->userWarning(
        "Error finding file(s)",
        "Unable to find one or more files with a name matching the run"
        " number or range input. Please check the files are present in"
        " the focused output directory and the input is correct.");
  } else if (!pocoUserPathInput.isFile()) {
    // foundFiles is not empty and this is a directory
    const std::string firstDir = foundFullFilePaths[0];
    m_view->setFittingRunNo(firstDir);
  }
}

/**
  * Verifies and uses the user input path (i.e. a browsed file)
  * to update drop down for available banks and various widgets on the GUI
  *
  * @param filePath The user entered file path as a Poco Path
  * @param splitBaseName The base file name split by the `_` delimiter
  *
  * @return The full file path as a vector of strings to make this
  * consistent with the other file processing methods
  */
std::vector<std::string> EnggDiffFittingPresenter::processFullPathInput(
    const Poco::Path &filePath, const std::vector<std::string> &splitBaseName) {

  std::vector<std::string> foundRunNumbers;
  std::vector<std::string> foundFullFilePaths;

  // Handle files the user browsed to separately
  try {
    foundRunNumbers =
        getAllBrowsedFilePaths(filePath.toString(), foundFullFilePaths);
  } catch (std::runtime_error &e) {
    const std::string eMsg(e.what());
    g_log.error("Error loading browsed file: " + eMsg);

    m_view->userWarning(
        "Run Number Not Found",
        "The run number specified could not be located "
        "in the focused output directory. Please check that the "
        "correct directory is set for Output Folder under Focusing Settings "
        "on the settings tab and that the input is correct");
    // Bring it back to a known state of 0 found
    foundFullFilePaths.clear();
    return foundFullFilePaths;
  }

  // Update UI to reflect found files
  // Update the list of files found in the view
  m_view->setFittingRunNumVec(foundFullFilePaths);

  const bool multiRunMode = m_view->getFittingMultiRunMode();
  const bool singleRunMode = m_view->getFittingSingleRunMode();
  // if not run mode or bank mode: to avoid recreating widgets
  if (!multiRunMode && !singleRunMode) {

    // add bank to the combo-box
    setBankItems(foundFullFilePaths);
    // set the bank widget according to selected bank file
    setDefaultBank(splitBaseName, filePath.toString());

    // Skips this step if it is multiple run because widget already
    // updated
    setRunNoItems(foundRunNumbers, false);
  }

  return foundFullFilePaths;
}

void EnggDiffFittingPresenter::processSelectRun() {
  const auto listLabel = m_view->getFittingListWidgetCurrentValue();
  int runNumber;
  size_t bank;
  std::tie(runNumber, bank) = runAndBankNumberFromListWidgetLabel(listLabel);

  const auto ws = m_model.getWorkspace(runNumber, bank);
  plotFocusedFile(false, ws);
}

/**
  * Takes the full path of a file which has been selected through
  * browse, the run number the user has input and stores the
  * full file paths of all files (specifically all banks) associated
  * with that run number. Then updates the view to display all run
  * numbers for multi-runs
  *
  * @param inputFullPath The user inputted path in the view
  * @param foundFullFilePaths The full paths of all associated files found
  *
  * @return Vector of all run numbers for which a full file path was found
  */
std::vector<std::string> EnggDiffFittingPresenter::getAllBrowsedFilePaths(
    const std::string &inputFullPath,
    std::vector<std::string> &foundFullFilePaths) {
  // to track the FittingRunnoChanged loop number
  if (g_fitting_runno_counter == 0) {
    g_multi_run_directories.clear();
  }

  g_fitting_runno_counter++;

  // Files take the form 'ENGINX_123456_focused_bank_1' so create
  // a string similar to 'ENGINX_123456_focused_bank' to allow us
  // to search for additional banks]
  const std::string baseName = getBaseNameFromStr(inputFullPath);
  const std::vector<std::string> splitBaseName =
      splitFittingDirectory(baseName);

  // TODO look at removal of hard coded filename positions
  // Produced <INST>_<RunNumber>_focused for subsequent lookup
  const std::string baseFilenamePrefix =
      splitBaseName[0] + "_" + splitBaseName[1] + "_" + splitBaseName[2];

  Poco::Path pocoFullFilePath;
  if (!pocoFullFilePath.tryParse(inputFullPath)) {
    // File path isn't valid
    m_view->userWarning("Bad file path entered",
                        "The entered file path could not "
                        " be opened. Please check the file and/or directory "
                        "exists.");

    throw std::runtime_error(
        "The file path entered could not be parsed"
        " this usually indicates a syntax error or bad path input");
  }

  const std::string workingDirectory = pocoFullFilePath.parent().toString();

  // Find all files which match this baseFilenamePrefix -
  // like a poor mans regular expression for files
  if (!findFilePathFromBaseName(workingDirectory, baseFilenamePrefix,
                                foundFullFilePaths)) {
    // I can't see this ever being thrown if the user is browsing to files but
    // better to be safe and give an informative message
    throw std::runtime_error("Could not find any files matching the generated"
                             " pattern: " +
                             baseFilenamePrefix);
  }

  // Store the run number as found
  std::vector<std::string> runNoVec;
  runNoVec.push_back(splitBaseName[1]);

  return runNoVec;
}

/**
  * Processes a multi run input to the interface
  * such as '12345-12350' by splitting it into '12345' to '12350'
  * then calling enableMultiRun
  * @param userInput The user input from the view
  *
  * @return List of found full file paths for the files specified
  */
std::vector<std::string>
EnggDiffFittingPresenter::processMultiRun(const std::string &userInput) {

  // Split user input into the first and last run number
  std::vector<std::string> firstLastRunNoVec;
  boost::split(firstLastRunNoVec, userInput, boost::is_any_of("-"));

  // Then store them in their own strings
  std::string firstRun;
  std::string lastRun;
  if (!firstLastRunNoVec.empty()) {
    firstRun = firstLastRunNoVec[0];
    lastRun = firstLastRunNoVec[1];

    m_view->setFittingMultiRunMode(true);
  }
  return enableMultiRun(firstRun, lastRun);
}

std::vector<std::string> EnggDiffFittingPresenter::processSingleRun(
    const std::string &userInputBasename,
    const std::vector<std::string> &splitBaseName) {

  const auto focusDir = m_view->focusingDir();

  // Check there is a folder to search for this file
  // this will be changed to respect user directories as well later
  if (focusDir.empty()) {
    m_view->userWarning("Focus directory not set.",
                        "Please check that a valid directory is "
                        "set for Output Folder under Focusing Settings on the "
                        "settings tab. "
                        "Please try again");

    m_view->enableFitAllButton(false);
  }

  // Next check input is a run number only as this is currently all
  // that we can handle
  if (!isDigit(userInputBasename)) {
    m_view->userWarning(
        "Invalid Run Number",
        "Invalid format of run number has been entered. There was"
        " non-numeric digits present in the input. Please try again");
    m_view->enableFitAllButton(false);
    throw std::invalid_argument("User input contained non-numeric characters");
  }

  if (userInputBasename.empty()) {

    m_view->userWarning("Invalid Run Number",
                        "Invalid format of run number has been entered. "
                        " The input was blank. Please try again");
    m_view->enableFitAllButton(false);
    throw std::invalid_argument("User input was blank");
  }

  if (g_fitting_runno_counter == 0) {
    g_multi_run_directories.clear();
  }

  // to track the FittingRunnoChanged loop number
  g_fitting_runno_counter++;

  // Inform the view we are using single run mode
  m_view->setFittingSingleRunMode(true);

  std::vector<std::string> foundFilePaths;
  const bool wasFound =
      findFilePathFromBaseName(focusDir, userInputBasename, foundFilePaths);

  if (!wasFound) {
    // Skip all UI update code
    return foundFilePaths;
  }

  const bool fittingMultiRunMode = m_view->getFittingMultiRunMode();
  if (!fittingMultiRunMode) {
    // Wrap the current run number in a vector and pass through
    // We cant use an initializer list as MSVC doesn't support this yet
    std::vector<std::string> strFocusedFileVector;
    strFocusedFileVector.push_back(userInputBasename);
    setRunNoItems(strFocusedFileVector, false);
  }

  // Update the list of found runs shown in the view
  m_view->setFittingRunNumVec(foundFilePaths);

  // add bank to the combo-box and list view
  // recreates bank widget for every run (multi-run) depending on
  // number of banks file found for given run number in folder

  setBankItems(foundFilePaths);
  setDefaultBank(splitBaseName, userInputBasename);

  return foundFilePaths;
}

/**
  * Finds the full file path (including extensions) for files that
  * match the given base filename (without ext) in the given folder.
  *
  * @param directoryToSearch The directory to search for these files
  * @param baseFileNamesToFind The base filename to find in this folder
  * @param foundFullFilePath Holds the path of the files if one was found
  * which matched the given base filename
  *
  * @return True if any files were found or false if none were
  */
bool EnggDiffFittingPresenter::findFilePathFromBaseName(
    const std::string &directoryToSearch,
    const std::string &baseFileNamesToFind,
    std::vector<std::string> &foundFullFilePath) {

  bool found = false;

  // Ask for an iterator of all files/folders in 'directoryToSearch'
  Poco::DirectoryIterator directoryIter;
  Poco::DirectoryIterator directoryIterEnd;

  try {
    directoryIter = directoryToSearch;
  } catch (Poco::FileNotFoundException) {
    // UNIX will throw if the directory is blank however Windows
    // will continue then fail to find the file in a non existent
    // directory - this ultimately results in the same thing.
    return false;
  }

  try {
    // Walk through every file within that folder looking for required files
    while (directoryIter != directoryIterEnd) {

      // Get files and not folders (don't recurse down)
      if (directoryIter->isFile()) {

        // Store the full path so if we get a matching file we know its path
        const std::string fullPathToCheck = directoryIter->path();

        // Get base name e.g. (ENGINX0012345)
        // Poco forces us to create a file from path to ask for base name
        // There must be a better way of doing this
        const Poco::Path PocoFileName = directoryIter->path();
        const std::string baseFileName = PocoFileName.getBaseName();

        // Look for the user input by comparing the base name of the
        // current file with the user input
        if (baseFileName.find(baseFileNamesToFind) != std::string::npos) {
          foundFullFilePath.emplace_back(fullPathToCheck);
          found = true;

          // if only first loop in Fitting Runno then add directory
          if (g_fitting_runno_counter == 1) {
            g_multi_run_directories.push_back(fullPathToCheck);
          }
        }
      }
      ++directoryIter;
    }

  } catch (std::runtime_error &re) {
    m_view->userWarning("Invalid file",
                        "File not found in the following directory; " +
                            directoryToSearch + ". " + re.what());
  }

  return found;
}

/**
  * Tests the user input for a multi run is valid and generates
  * all values between that range (e.g. 1-10 produces 1,2,3...)
  * to then look up those runs in the focus directory and find
  * their full file paths. Additionally it updates the GUI to reflect
  * if all runs were found or not
  *
  * @param firstRun The first run number of the range as a string
  * @param lastRun The last run number of the range as a string
  *
  * @return A vector containing all file paths which were found
  */
std::vector<std::string>
EnggDiffFittingPresenter::enableMultiRun(const std::string &firstRun,
                                         const std::string &lastRun) {

  std::vector<std::string> fittingRunNoDirVec;

  // Perform input checks first
  // Are both values either side of '-' the user input digits
  if (!isDigit(firstRun) || !isDigit(lastRun)) {
    m_view->userWarning("Invalid Run Number",
                        "Invalid format of multi-run number has been entered. "
                        "Please try again");
    m_view->enableFitAllButton(false);
    throw std::invalid_argument("Both values are not numerical entries");
  }

  // Convert strings to integers for remainder of function
  const int firstNum = std::stoi(firstRun);
  const int lastNum = std::stoi(lastRun);
  const size_t range = abs(lastNum - firstNum);

  // Cap the maximum number of runs we can process at 200
  constexpr size_t maximumNumberOfRuns = 200;

  if (range > maximumNumberOfRuns) {
    m_view->userWarning("Range too large",
                        "The specified run number range is too large."
                        " A maximum of 200 entries can be processed a time.");
    m_view->enableFitAllButton(false);
    throw std::invalid_argument("Number of runs is greater than 200");
  }

  // By performing this check we can make optimizations and assumptions
  // about the ordering of values
  if (firstNum > lastNum) {
    m_view->userWarning("Range not ascending",
                        "The range specified was not ascending. "
                        "The last run number needs to be"
                        " larger than the start run number");
    throw std::invalid_argument("Run range is not ascending");
  }

  std::string workingDirectory = m_view->focusingDir();

  if (workingDirectory.empty()) {
    m_view->userWarning("Invalid Input",
                        "Please check that a valid directory is "
                        "set for Output Folder under Focusing Settings on the "
                        "settings tab. "
                        "Please try again");
    throw std::invalid_argument("Focus directory not set correctly");
  }

  // --- All checks complete lets process the multi run input ---

  // Reserve the number of elements needed so we don't allocate multiple times
  std::vector<std::string> RunNumberVec;
  RunNumberVec.reserve(range);

  for (int i = firstNum; i <= lastNum; i++) {
    // Populate vector with list of runs
    RunNumberVec.push_back(std::to_string(i));
  }

  // clear previous directories set before findFilePathsFromBaseName
  if (g_fitting_runno_counter == 0) {
    g_multi_run_directories.clear();
  }
  // to track the FittingRunnoChanged loop number
  g_fitting_runno_counter++;

  // rewrite the vector of run number which is available
  std::vector<std::string> foundRunNumber;

  bool foundAllRuns = true;

  for (const auto &runNumber : RunNumberVec) {
    // Get full path for every run selected
    std::vector<std::string> foundFileNames;
    if (findFilePathFromBaseName(workingDirectory, runNumber, foundFileNames)) {
      // Append those that were found with fittingRunNoDirVec
      fittingRunNoDirVec.insert(fittingRunNoDirVec.end(),
                                foundFileNames.cbegin(), foundFileNames.cend());
    } else {
      // We couldn't find this one - set the flag and break out of loop
      foundAllRuns = false;
      break;
    }
  }

  if (foundAllRuns) {
    setRunNoItems(RunNumberVec, true);
  } else {
    // Set the size to 0 as some were not found
    fittingRunNoDirVec.clear();
  }

  return fittingRunNoDirVec;
}

void EnggDiffFittingPresenter::processStart() {}

void EnggDiffFittingPresenter::processLoad() {
  const std::string filenames = m_view->getFittingRunNo();

  try {
    m_model.loadWorkspaces(filenames);
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

  const auto runNoBankPairs = m_model.getRunNumbersAndBanksIDs();
  std::vector<std::string> listWidgetLabels;
  std::transform(
      runNoBankPairs.begin(), runNoBankPairs.end(),
      std::back_inserter(listWidgetLabels), [](const std::pair<int, size_t> &pair) {
        return listWidgetLabelFromRunAndBankNumber(pair.first, pair.second);
      });
  m_view->enableFittingListWidget(true);
  m_view->clearFittingListWidget();
  std::for_each(listWidgetLabels.begin(), listWidgetLabels.end(),
                [&](const std::string &listLabel) {
                  m_view->addRunNoItem(listLabel);
                });
}

void EnggDiffFittingPresenter::processShutDown() {
  m_viewHasClosed = true;
  m_view->saveSettings();
  cleanup();
}

void EnggDiffFittingPresenter::processLogMsg() {
  std::vector<std::string> msgs = m_view->logMsgs();
  for (size_t i = 0; i < msgs.size(); i++) {
    g_log.information() << msgs[i] << '\n';
  }
}

void EnggDiffFittingPresenter::processFitAllPeaks() {

  std::string fittingPeaks = m_view->fittingPeaksData();

  // validate fitting data as it will remain the same through out
  const std::string fitPeaksData = validateFittingexpectedPeaks(fittingPeaks);

  g_log.debug() << "Focused files found are: " << fitPeaksData << '\n';
  for (const auto &dir : g_multi_run_directories) {
    g_log.debug() << dir << '\n';
  }

  if (!g_multi_run_directories.empty()) {

    for (size_t i = 0; i < g_multi_run_directories.size(); i++) {
      try {

        inputChecksBeforeFitting(g_multi_run_directories[i], fitPeaksData);
      } catch (std::invalid_argument &ia) {
        m_view->userWarning("Error in the inputs required for fitting",
                            ia.what());
        return;
      }
    }

    const std::string outWSName = "engggui_fitting_fit_peak_ws";
    g_log.notice() << "EnggDiffraction GUI: starting new multi-run "
                      "single peak fits into workspace '" +
                          outWSName + "'. This "
                                      "may take some seconds... \n";

    m_view->showStatus("Fitting multi-run single peaks...");

    // disable GUI to avoid any double threads
    m_view->enableCalibrateFocusFitUserActions(false);
    m_view->enableFitAllButton(false);
    // startAsyncFittingWorker
    // doFitting()
    // WORK OUT WHAT TO DO HERE
	//startAsyncFittingWorker(g_multi_run_directories, fitPeaksData);

  } else {
    m_view->userWarning("Error in the inputs required for fitting",
                        "Invalid files have been selected for Fit All process");
    m_view->enableFitAllButton(false);
  }
}

void EnggDiffFittingPresenter::processFitPeaks() {
  if (!m_view->listWidgetHasSelectedRow()) {
	  m_view->userWarning("No run selected", "Please select a run to fit from the list");
	  return;
  }
  const auto listLabel = m_view->getFittingListWidgetCurrentValue();
  int runNumber;
  size_t bank;
  std::tie(runNumber, bank) = runAndBankNumberFromListWidgetLabel(listLabel);
  const auto filename = m_model.getWorkspaceFilename(runNumber, bank);
  std::string fittingPeaks = m_view->fittingPeaksData();

  const std::string fitPeaksData = validateFittingexpectedPeaks(fittingPeaks);

  g_log.debug() << "the expected peaks are: " << fitPeaksData << '\n';

  try {
    inputChecksBeforeFitting(filename, fitPeaksData);
  } catch (std::invalid_argument &ia) {
    m_view->userWarning("Error in the inputs required for fitting", ia.what());
    return;
  }

  // disable so that user is forced to select file again
  // otherwise empty vector will be passed
  m_view->enableFitAllButton(false);

  const std::string outWSName = "engggui_fitting_fit_peak_ws";
  g_log.notice() << "EnggDiffraction GUI: starting new "
                    "single peak fits into workspace '" +
                        outWSName + "'. This may take some seconds... \n";

  m_view->showStatus("Fitting single peaks...");
  // disable GUI to avoid any double threads
  m_view->enableCalibrateFocusFitUserActions(false);

  startAsyncFittingWorker(runNumber, bank, fitPeaksData);
}

void EnggDiffFittingPresenter::inputChecksBeforeFitting(
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

/**
* Splits the file name in to sections of '_' and 'ENGINX' text
* within the filename
*
* @param selectedfPath is the selected file's path
*
* @return std::vector<std::string> of splitted file name with run
* number & bank
*/
std::vector<std::string> EnggDiffFittingPresenter::splitFittingDirectory(
    const std::string &selectedfPath) {

  Poco::Path PocofPath(selectedfPath);
  std::string selectedbankfName = PocofPath.getBaseName();
  std::vector<std::string> splitBaseName;
  boost::split(splitBaseName, selectedbankfName, boost::is_any_of("_."));
  return splitBaseName;
}

std::string EnggDiffFittingPresenter::validateFittingexpectedPeaks(
    std::string &expectedPeaks) const {

  if (!expectedPeaks.empty()) {

    g_log.debug() << "Validating the expected peak list.\n";

    auto *comma = ",";

    for (size_t i = 0; i < expectedPeaks.size() - 1; i++) {
      size_t j = i + 1;

      if (expectedPeaks[i] == *comma && expectedPeaks[i] == expectedPeaks[j]) {
        expectedPeaks.erase(j, 1);
        i--;

      } else {
        ++j;
      }
    }

    size_t strLength = expectedPeaks.length() - 1;
    if (expectedPeaks.at(size_t(0)) == ',') {
      expectedPeaks.erase(size_t(0), 1);
      strLength -= size_t(1);
    }

    if (expectedPeaks.at(strLength) == ',') {
      expectedPeaks.erase(strLength, 1);
    }

    m_view->setPeakList(expectedPeaks);
  }

  return expectedPeaks;
}

void EnggDiffFittingPresenter::doFitting(const int runNumber, const size_t bank,
                                         const std::string &expectedPeaks) {
  g_log.notice() << "EnggDiffraction GUI: starting new fitting with run "
                 << runNumber << " and bank " << bank 
	             << ". This may take a few seconds... \n";

  m_fittingFinishedOK = false;

  // load the focused workspace file to perform single peak fits

  // apply calibration to the focused workspace
  m_model.setDifcTzero(runNumber, bank, currentCalibration());

  // run the algorithm EnggFitPeaks with workspace loaded above
  // requires unit in Time of Flight
  m_model.enggFitPeaks(runNumber, bank, expectedPeaks);

  const auto outFilename = m_view->getCurrentInstrument() + std::to_string(runNumber) +
	  "_Single_Peak_Fitting.csv";
  auto saveDirectory = outFilesUserDir("SinglePeakFitting");
  saveDirectory.append(outFilename);
  m_model.saveDiffFittingAscii(runNumber, bank, saveDirectory.toString());

  m_model.createFittedPeaksWS(runNumber, bank);
  m_fittingFinishedOK = true;
}

void EnggDiffFittingPresenter::browsePeaksToFit() {
  try {
    auto prevPath = m_view->focusingDir();
    if (prevPath.empty()) {
      prevPath = m_view->getPreviousDir();
    }
    std::string path = m_view->getOpenFile(prevPath);
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

    auto curExpPeaksList = m_view->fittingPeaksData();

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
    QString prevPath = QString::fromStdString(m_view->focusingDir());
    if (prevPath.isEmpty()) {
      prevPath = QString::fromStdString(m_view->getPreviousDir());
    }

    std::string path = m_view->getSaveFile(prevPath.toStdString());

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
	}
	else {
		auto expPeaks = m_view->fittingPeaksData();
		outfile << expPeaks;
	}
}

void EnggDiffFittingPresenter::setBankItems(
    const std::vector<std::string> &bankFiles) {

  if (bankFiles.empty()) {
    // upon invalid file
    // disable the widgets when only one related file found
    m_view->enableFittingComboBox(false);

    m_view->clearFittingComboBox();
    return;
  }

  // delete previous bank added to the list
  m_view->clearFittingComboBox();

  try {
    // Keep track of current loop iteration for banks
    int index = 0;
    for (const auto &filePath : bankFiles) {

      const Poco::Path bankFile(filePath);
      const std::string strVecFile = bankFile.toString();
      // split the directory from m_fitting_runno_dir_vec
      std::vector<std::string> vecFileSplit = splitFittingDirectory(strVecFile);

      // get the last split in vector which will be bank
      std::string bankID = (vecFileSplit.back());

      bool digit = isDigit(bankID);

      if (digit || bankID == "cropped") {
        m_view->addBankItem(bankID);
      } else {
        QString qBank = QString("Bank %1").arg(index + 1);
        m_view->addBankItem(qBank.toStdString());
        ++index;
      }
    }

    m_view->enableFittingComboBox(true);

  } catch (std::runtime_error &re) {
    m_view->userWarning("Unable to insert items: ",
                        "Could not add banks to "
                        "combo-box or list widget; " +
                            static_cast<std::string>(re.what()) +
                            ". Please try again");
  }
}

void EnggDiffFittingPresenter::setRunNoItems(
    const std::vector<std::string> &runNumVector, bool multiRun) {
  try {
    if (!runNumVector.empty()) {

      // delete previous run number added to the list
      m_view->clearFittingListWidget();

      // enable fit button only when run number provided
      m_view->enableFitAllButton(true);

      for (size_t i = 0; i < runNumVector.size(); i++) {

        std::string currentRun = (runNumVector[i]);

        // adding to widget
        m_view->addRunNoItem(currentRun);
        g_multi_run.push_back(currentRun);
      }

      // change to selected run number item
      if (multiRun) {
        m_view->enableFittingListWidget(true);

        auto currentIndex = m_view->getFittingListWidgetCurrentRow();
        if (currentIndex == -1)
          m_view->setFittingListWidgetCurrentRow(0);
      } else {
        m_view->enableFittingListWidget(false);
      }
    }

    else {
      // upon invalid file
      // disable the widgets when only one related file found
      m_view->enableFittingListWidget(false);
      m_view->enableFitAllButton(false);
      m_view->clearFittingListWidget();

      m_view->userWarning(
          "Run Number Not Found",
          "The run number specified could not be located "
          "in the focused output directory. Please check that the "
          "correct directory is set for Output Folder under Focusing Settings "
          "on the settings tab.");
    }

  } catch (std::runtime_error &re) {
    m_view->userWarning("Unable to insert items: ",
                        "Could not add list widget; " +
                            static_cast<std::string>(re.what()) +
                            ". Please try again");
  }
}

void EnggDiffFittingPresenter::setDefaultBank(
    const std::vector<std::string> &splittedBaseName,
    const std::string &selectedFile) {

  if (!splittedBaseName.empty()) {

    std::string bankID = (splittedBaseName.back());
    auto combo_data = m_view->getFittingComboIdx(bankID);

    if (combo_data > -1) {
      m_view->setBankIdComboBox(combo_data);
    } else {
      m_view->setFittingRunNo(selectedFile);
    }
  }
  // check if the vector is not empty so that the first directory
  // can be assigned to text-field when number is given
  else if (!m_view->getFittingRunNumVec().empty()) {
    auto firstDir = m_view->getFittingRunNumVec().at(0);
    const auto &intialDir = firstDir;
    m_view->setFittingRunNo(intialDir);
  }
  // if nothing found related to text-field input
  else if (!m_view->getFittingRunNo().empty())
    m_view->setFittingRunNo(selectedFile);
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

    m_view->setDataVector(focusedData, true, plotSinglePeaks);

  } catch (std::runtime_error &re) {
    g_log.error()
        << "Unable to plot focused workspace on the canvas. "
        << "Error description: " << re.what()
        << " Please check also the previous log messages for details.";

    m_view->showStatus("Error while plotting the peaks fitted");
    throw;
  }
}

void EnggDiffFittingPresenter::plotFitPeaksCurves() {
  try {

    // detaches previous plots from canvas
	  m_view->resetCanvas();

	const auto listLabel = m_view->getFittingListWidgetCurrentValue();
	int runNumber;
	size_t bank;
	std::tie(runNumber, bank) = runAndBankNumberFromListWidgetLabel(listLabel);
	const auto ws = m_model.getAlignedWorkspace(runNumber, bank);

    // plots focused workspace
    plotFocusedFile(m_fittingFinishedOK, ws);

    if (m_fittingFinishedOK) {
      g_log.debug() << "single peaks fitting being plotted now.\n";
	  auto singlePeaksWS = m_model.getFittedPeaksWS(runNumber, bank);
      auto singlePeaksData = QwtHelper::curveDataFromWs(singlePeaksWS);
      m_view->setDataVector(singlePeaksData, false, true);
      m_view->showStatus("Peaks fitted successfully");

    } else {
      g_log.notice()
          << "Focused workspace has been plotted to the "
             "graph; further peaks can be adding using Peak Tools.\n";
      g_log.warning() << "Peaks could not be plotted as the fitting process "
                         "did not finish correctly.\n";
      m_view->showStatus("No peaks could be fitted");
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
