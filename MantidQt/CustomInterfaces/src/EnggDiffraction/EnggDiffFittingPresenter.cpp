#include "MantidQtCustomInterfaces/EnggDiffraction/EnggDiffFittingPresenter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidQtCustomInterfaces/EnggDiffraction/EnggDiffFittingPresWorker.h"
#include "MantidQtCustomInterfaces/Muon/ALCHelper.h"

#include <boost/lexical_cast.hpp>
#include <fstream>

#include <Poco/DirectoryIterator.h>
#include <Poco/File.h>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;

namespace MantidQt {
namespace CustomInterfaces {

namespace {
Mantid::Kernel::Logger g_log("EngineeringDiffractionGUI");
}

const bool EnggDiffFittingPresenter::g_useAlignDetectors = true;

const std::string EnggDiffFittingPresenter::g_focusedFittingWSName =
    "engggui_fitting_focused_ws";

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
      m_mainCalib(mainCalib), m_mainParam(mainParam), m_view(view) {}

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
  switch (notif) {

  case IEnggDiffFittingPresenter::Start:
    processStart();
    break;

  case IEnggDiffFittingPresenter::FittingRunNo:
    fittingRunNoChanged();
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
    const std::vector<std::string> &focusedRunNo,
    const std::string &expectedPeaks) {

  delete m_workerThread;
  m_workerThread = new QThread(this);
  EnggDiffFittingWorker *worker =
      new EnggDiffFittingWorker(this, focusedRunNo, expectedPeaks);
  worker->moveToThread(m_workerThread);

  connect(m_workerThread, SIGNAL(started()), worker, SLOT(fitting()));
  connect(worker, SIGNAL(finished()), this, SLOT(fittingFinished()));
  // early delete of thread and worker
  connect(m_workerThread, SIGNAL(finished()), m_workerThread,
          SLOT(deleteLater()), Qt::DirectConnection);
  connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
  m_workerThread->start();
}

void EnggDiffFittingPresenter::fittingFinished() {
  if (!m_view)
    return;

  if (!m_fittingFinishedOK) {
    g_log.warning() << "The single peak fitting did not finish correctly.\n";
    if (m_workerThread) {
      delete m_workerThread;
      m_workerThread = nullptr;
    }

    m_view->showStatus(
        "Single peak fitting process did not complete successfully");
  } else {
    g_log.notice() << "The single peak fitting finished - the output "
                      "workspace is ready.\n";

    m_view->showStatus("Single peak fittin process finished. Ready");
    if (m_workerThread) {
      delete m_workerThread;
      m_workerThread = nullptr;
    }
  }

  try {
    // should now plot the focused workspace when single peak fitting
    // process fails
    plotFitPeaksCurves();

  } catch (std::runtime_error &re) {
    g_log.error() << "Unable to finish the plotting of the graph for "
                     "engggui_fitting_focused_fitpeaks workspace. Error "
                     "description: " +
                         static_cast<std::string>(re.what()) +
                         " Please check also the log message for detail.";
    throw;
  }
  g_log.notice() << "EnggDiffraction GUI: plotting of peaks for single peak "
                    "fits has completed. \n";

  // Reset once whole process is completed
  g_multi_run.clear();
  // enable the GUI
  m_view->enableCalibrateFocusFitUserActions(true);
}

// Fitting Tab Run Number & Bank handling here
void EnggDiffFittingPresenter::fittingRunNoChanged() {

  try {

    // receive the run number from the text-field
    auto strFocusedFile = m_view->getFittingRunNo();

    // file name
    Poco::Path selectedfPath(strFocusedFile);
    Poco::Path bankDir;

    std::vector<std::string> runnoDirVector;

    std::string strFPath = selectedfPath.toString();
    // returns empty if no directory is found
    // split directory if 'ENGINX_' found by '_.'
    std::vector<std::string> splitBaseName =
        m_view->splitFittingDirectory(strFPath);

    // runNo when single focused file selected
    std::vector<std::string> runNoVec;

    // if input file is a directory and successfully splitBaseName
    // or when default bank is set or changed, the text-field is updated with
    // selected bank directory which would trigger this function again
    if (selectedfPath.isFile() && !splitBaseName.empty()) {

#ifdef __unix__
      bankDir = selectedfPath.parent();
#else
      bankDir = (bankDir).expand(selectedfPath.parent().toString());
#endif
      // if vector is not empty and correct focus format file is selected
      if (!splitBaseName.empty() && splitBaseName.size() > 3) {

        std::string bankFileDir = bankDir.toString();

        // browse the file
        browsedFile(strFocusedFile, runnoDirVector, splitBaseName, runNoVec,
                    bankFileDir);
      }
      // if given a multi-run OR single number
    } else if (strFocusedFile.size() > 4) {

      // if multi-run number string finds '-' to define run number
      if (strFocusedFile.find("-") != std::string::npos) {

        // adds run number to list widget on right and changes text-field
        // to single run number and trigger FittingRunNo changed again
        processMultiRun(strFocusedFile, runnoDirVector);

      } else {
        // true if string convertible to digit
        bool isRunNumber = isDigit(strFocusedFile);
        auto focusDir = m_view->focusingDir();

        // if not valid parent dir and not valid single run number
        if (focusDir.empty()) {
          m_view->userWarning(
              "Invalid Input",
              "Please check that a valid directory is "
              "set for Output Folder under Focusing Settings on the "
              "settings tab. "
              "Please try again");

          m_view->enableFitAllButton(false);
        } else if (!isRunNumber) {

          m_view->userWarning("Invalid Run Number",
                              "Invalid format of run number has been entered. "
                              "Please try again");
          m_view->enableFitAllButton(false);
        }

        // else - given or hard-coded to a single run number
        else {
          processSingleRun(focusDir, strFocusedFile, runnoDirVector,
                           splitBaseName);
        }
      }
    }

    // if single or multi run-number
    // set the text-field to directory here to the first in
    // the vector if its not empty
    if (!runnoDirVector.empty() && !selectedfPath.isFile()) {

      auto firstDir = runnoDirVector[0];
      m_view->setFittingRunNo(firstDir);

    } else if (m_view->getFittingRunNo().empty()) {
      m_view->userWarning("Invalid Input",
                          "Invalid directory or run number given. "
                          "Please try again");
    }

  } catch (std::runtime_error &re) {
    m_view->userWarning("Invalid file",
                        "Unable to select the file; " +
                            static_cast<std::string>(re.what()));
    return;
  }
}

void EnggDiffFittingPresenter::browsedFile(
    const std::string strFocusedFile, std::vector<std::string> &runnoDirVector,
    const std::vector<std::string> &splitBaseName,
    std::vector<std::string> &runNoVec, const std::string &bankFileDir) {
  // to track the FittingRunnoChanged loop number
  if (g_fitting_runno_counter == 0) {
    g_multi_run_directories.clear();
  }

  g_fitting_runno_counter++;

  // regenerating the focus file name
  std::string foc_file = splitBaseName[0] + "_" + splitBaseName[1] + "_" +
                         splitBaseName[2] + "_" + splitBaseName[3];

  // if base directory of file is empty
  if (bankFileDir.empty()) {
    m_view->userWarning("Invalid Input",
                        "Please check that a valid directory is "
                        "set for Output Folder under Focusing Settings on the "
                        "settings tab. "
                        "Please try again");
  } else {

    // foc_file - vector holding the file name split
    // runnoDirVector - giving empty vector here holding directory of
    // selected vector
    // dummy vector not being used in this case
    std::vector<std::string> dummy;
    updateFittingDirVec(bankFileDir, foc_file, runnoDirVector, dummy);

    m_view->setFittingRunNumVec(runnoDirVector);

    auto multiRunMode = m_view->getFittingMultiRunMode();
    auto singleRunMode = m_view->getFittingSingleRunMode();
    // if not run mode or bank mode: to avoid recreating widgets
    if (!multiRunMode && !singleRunMode) {

      // add bank to the combo-box
      setBankItems();
      // set the bank widget according to selected bank file
      setDefaultBank(splitBaseName, strFocusedFile);
      runNoVec.clear();
      runNoVec.push_back(splitBaseName[1]);

      // Skips this step if it is multiple run because widget already
      // updated
      setRunNoItems(runNoVec, false);
    }
  }
}

void EnggDiffFittingPresenter::processMultiRun(
    const std::string strFocusedFile,
    std::vector<std::string> &runnoDirVector) {
  std::vector<std::string> firstLastRunNoVec;
  boost::split(firstLastRunNoVec, strFocusedFile, boost::is_any_of("-"));
  std::string firstRun;
  std::string lastRun;
  if (!firstLastRunNoVec.empty()) {
    firstRun = firstLastRunNoVec[0];
    lastRun = firstLastRunNoVec[1];

    m_view->setFittingMultiRunMode(true);
    enableMultiRun(firstRun, lastRun, runnoDirVector);
  }
}

void EnggDiffFittingPresenter::processSingleRun(
    const std::string &focusDir, const std::string &strFocusedFile,
    std::vector<std::string> &runnoDirVector,
    const std::vector<std::string> &splitBaseName) {

  if (g_fitting_runno_counter == 0) {
    g_multi_run_directories.clear();
  }

  // to track the FittingRunnoChanged loop number
  g_fitting_runno_counter++;

  m_view->setFittingSingleRunMode(true);

  // dummy vector will not be used

  std::vector<std::string> foundRunNumber;
  updateFittingDirVec(focusDir, strFocusedFile, runnoDirVector, foundRunNumber);
  m_view->setFittingRunNumVec(runnoDirVector);

  // add bank to the combo-box and list view
  // recreates bank widget for every run (multi-run) depending on
  // number of banks file found for given run number in folder
  setBankItems();
  setDefaultBank(splitBaseName, strFocusedFile);

  auto fittingMultiRunMode = m_view->getFittingMultiRunMode();
  if (!fittingMultiRunMode) {
    setRunNoItems(foundRunNumber, false);
  }
}

void EnggDiffFittingPresenter::updateFittingDirVec(
    const std::string &focusDir, const std::string &runNumberVec,
    std::vector<std::string> &fittingRunNoDirVec,
    std::vector<std::string> &foundRunNumber) {

  try {
    bool found = false;

    const std::string cwd(focusDir);
    Poco::DirectoryIterator it(cwd);
    Poco::DirectoryIterator end;
    while (it != end) {
      if (it->isFile()) {
        std::string itFilePath = it->path();
        Poco::Path itBankfPath(itFilePath);

        std::string itbankFileName = itBankfPath.getBaseName();
        // check if it not any other file.. e.g: texture
        if (itbankFileName.find(runNumberVec) != std::string::npos) {
          fittingRunNoDirVec.push_back(itFilePath);
          found = true;

          // if only first loop in Fitting Runno then add directory
          if (g_fitting_runno_counter == 1) {
            g_multi_run_directories.push_back(itFilePath);
          }
        }
      }
      ++it;
    }

    if (found) {
      foundRunNumber.push_back(runNumberVec);
    }

  } catch (std::runtime_error &re) {
    m_view->userWarning("Invalid file",
                        "File not found in the following directory; " +
                            focusDir + ". " +
                            static_cast<std::string>(re.what()));
  }
}

void EnggDiffFittingPresenter::enableMultiRun(
    std::string firstRun, std::string lastRun,
    std::vector<std::string> &fittingRunNoDirVec) {

  bool firstDig = isDigit(firstRun);
  bool lastDig = isDigit(lastRun);

  std::vector<std::string> RunNumberVec;
  if (firstDig && lastDig) {
    int firstNum = std::stoi(firstRun);
    int lastNum = std::stoi(lastRun);

    if ((lastNum - firstNum) > 200) {
      m_view->userWarning(
          "Please try again",
          "The specified run number range is "
          "far to big, please try a smaller range of consecutive run numbers.");
    }

    else if (firstNum <= lastNum) {

      for (int i = firstNum; i <= lastNum; i++) {
        // push run number to vector so can be
        // used to search for directories with given run no
        RunNumberVec.push_back(std::to_string(i));
      }

      auto focusDir = m_view->focusingDir();
      if (focusDir.empty()) {
        m_view->userWarning(
            "Invalid Input",
            "Please check that a valid directory is "
            "set for Output Folder under Focusing Settings on the "
            "settings tab. "
            "Please try again");
      } else {

        // clear previous directories set before updateFittingDirVec
        if (g_fitting_runno_counter == 0) {
          g_multi_run_directories.clear();
        }
        // to track the FittingRunnoChanged loop number
        g_fitting_runno_counter++;

        // rewrite the vector of run number which is available
        std::vector<std::string> foundRunNumber;

        for (size_t i = 0; i < RunNumberVec.size(); i++) {
          // save dir for every vector
          updateFittingDirVec(focusDir, RunNumberVec[i], fittingRunNoDirVec,
                              foundRunNumber);
        }

        int diff = (lastNum - firstNum) + 1;
        size_t run_vec_size = foundRunNumber.size();

        if (size_t(diff) == run_vec_size) {
          setRunNoItems(foundRunNumber, true);
          m_view->setBankEmit();
        } else {
          m_view->userWarning(
              "Run Number Not Found",
              "The multi-run number specified could not be located "
              "in the focused output directory. Please check that the "
              "correct directory is set for Output Folder under Focusing "
              "Settings "
              "on the settings tab.");
        }
      }
    } else {
      m_view->userWarning("Invalid Run Number",
                          "Invalid multi-run number range has been provided. "
                          "Please try again");
      m_view->enableFitAllButton(false);
    }
  } else {
    m_view->userWarning("Invalid Run Number",
                        "Invalid format of multi-run number has been entered. "
                        "Please try again");
    m_view->enableFitAllButton(false);
  }
}

void EnggDiffFittingPresenter::processStart() {}

void EnggDiffFittingPresenter::processShutDown() {
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
  for (auto dir : g_multi_run_directories) {
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
    startAsyncFittingWorker(g_multi_run_directories, fitPeaksData);

  } else {
    m_view->userWarning("Error in the inputs required for fitting",
                        "Invalid files have been selected for Fit All process");
    m_view->enableFitAllButton(false);
  }
}

void EnggDiffFittingPresenter::processFitPeaks() {
  const std::string focusedRunNo = m_view->getFittingRunNo();
  std::string fittingPeaks = m_view->fittingPeaksData();

  const std::string fitPeaksData = validateFittingexpectedPeaks(fittingPeaks);

  g_log.debug() << "the expected peaks are: " << fitPeaksData << '\n';

  try {
    inputChecksBeforeFitting(focusedRunNo, fitPeaksData);
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
                        outWSName + "'. This "
                                    "may take some seconds... \n";

  m_view->showStatus("Fitting single peaks...");
  // disable GUI to avoid any double threads
  m_view->enableCalibrateFocusFitUserActions(false);
  // startAsyncFittingWorker
  // doFitting()
  std::vector<std::string> focusRunNoVec;
  focusRunNoVec.push_back(focusedRunNo);

  startAsyncFittingWorker(focusRunNoVec, fitPeaksData);
}

void EnggDiffFittingPresenter::inputChecksBeforeFitting(
    const std::string &focusedRunNo, const std::string &expectedPeaks) {
  if (focusedRunNo.size() == 0) {
    throw std::invalid_argument(
        "Focused Run "
        "cannot be empty and must be a valid directory");
  }

  Poco::File file(focusedRunNo);
  if (!file.exists()) {
    throw std::invalid_argument("The focused workspace file for single peak "
                                "fitting could not be found: " +
                                focusedRunNo);
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

void EnggDiffFittingPresenter::setDifcTzero(MatrixWorkspace_sptr wks) const {
  size_t bankID = 1;
  // attempt to guess bankID - this should be done in code that is currently
  // in the view
  auto fittingFilename = m_view->getFittingRunNo();
  Poco::File fittingFile(fittingFilename);
  if (fittingFile.exists()) {
    Poco::Path path(fittingFile.path());
    auto name = path.getBaseName();
    std::vector<std::string> chunks;
    boost::split(chunks, name, boost::is_any_of("_"));
    bool isNum = isDigit(chunks.back());
    if (!chunks.empty() && isNum) {
      try {
        bankID = boost::lexical_cast<size_t>(chunks.back());
      } catch (std::runtime_error &re) {
        g_log.error()
            << "Unable to successfully apply DifcTzero to focused workspace. "
               "Error description: " +
                   static_cast<std::string>(re.what()) << '\n';
        throw;
      }
    }
  }

  const std::string units = "none";
  auto &run = wks->mutableRun();

  std::vector<GSASCalibrationParms> calibParms = currentCalibration();
  if (calibParms.empty()) {
    run.addProperty<int>("bankid", 1, units, true);
    run.addProperty<double>("difc", 18400.0, units, true);
    run.addProperty<double>("difa", 0.0, units, true);
    run.addProperty<double>("tzero", 4.0, units, true);
  } else {
    GSASCalibrationParms parms(0, 0.0, 0.0, 0.0);
    for (const auto &p : calibParms) {
      if (p.bankid == bankID) {
        parms = p;
        break;
      }
    }
    if (0 == parms.difc)
      parms = calibParms.front();

    run.addProperty<int>("bankid", static_cast<int>(parms.bankid), units, true);
    run.addProperty<double>("difc", parms.difc, units, true);
    run.addProperty<double>("difa", parms.difa, units, true);
    run.addProperty<double>("tzero", parms.tzero, units, true);
  }
}

void EnggDiffFittingPresenter::doFitting(const std::string &focusedRunNo,
                                         const std::string &expectedPeaks) {
  g_log.notice() << "EnggDiffraction GUI: starting new fitting with file "
                 << focusedRunNo << ". This may take a few seconds... \n";

  MatrixWorkspace_sptr focusedWS;
  m_fittingFinishedOK = false;

  // if the last directory in vector matches the input directory within this
  // function then clear the vector
  if (!g_multi_run_directories.empty()) {
    auto lastDir = g_multi_run_directories.back() == focusedRunNo;
    if (lastDir) {
      m_view->enableFitAllButton(false);
      g_fitting_runno_counter = 0;
    }
  }

  // load the focused workspace file to perform single peak fits
  try {
    auto load =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged("Load");
    load->initialize();
    load->setPropertyValue("Filename", focusedRunNo);
    load->setPropertyValue("OutputWorkspace", g_focusedFittingWSName);
    load->execute();

    AnalysisDataServiceImpl &ADS = Mantid::API::AnalysisDataService::Instance();
    focusedWS = ADS.retrieveWS<MatrixWorkspace>(g_focusedFittingWSName);
  } catch (std::runtime_error &re) {
    g_log.error()
        << "Error while loading focused data. "
           "Could not run the algorithm Load successfully for the Fit "
           "peaks (file name: " +
               focusedRunNo + "). Error description: " + re.what() +
               " Please check also the previous log messages for details.";
    return;
  }

  // apply calibration to the focused workspace
  setDifcTzero(focusedWS);

  // run the algorithm EnggFitPeaks with workspace loaded above
  // requires unit in Time of Flight
  auto enggFitPeaks =
      Mantid::API::AlgorithmManager::Instance().createUnmanaged("EnggFitPeaks");
  const std::string focusedFitPeaksTableName =
      "engggui_fitting_fitpeaks_params";

  // delete existing table workspace to avoid confusion
  AnalysisDataServiceImpl &ADS = Mantid::API::AnalysisDataService::Instance();
  if (ADS.doesExist(focusedFitPeaksTableName)) {
    ADS.remove(focusedFitPeaksTableName);
  }

  try {
    enggFitPeaks->initialize();
    enggFitPeaks->setProperty("InputWorkspace", focusedWS);
    if (!expectedPeaks.empty()) {
      enggFitPeaks->setProperty("expectedPeaks", expectedPeaks);
    }
    enggFitPeaks->setProperty("FittedPeaks", focusedFitPeaksTableName);
    enggFitPeaks->execute();
  } catch (std::exception &re) {
    g_log.error() << "Could not run the algorithm EnggFitPeaks "
                     "successfully for bank, "
                     // bank name
                     "Error description: " +
                         static_cast<std::string>(re.what()) +
                         " Please check also the log message for detail.\n";
  }

  auto fPath = focusedRunNo;
  runSaveDiffFittingAsciiAlg(focusedFitPeaksTableName, fPath);

  try {
    runFittingAlgs(focusedFitPeaksTableName, g_focusedFittingWSName);

  } catch (std::invalid_argument &ia) {
    g_log.error() << "Error, Fitting could not finish off correctly, " +
                         std::string(ia.what()) << '\n';
    return;
  }
}

void MantidQt::CustomInterfaces::EnggDiffFittingPresenter::
    runSaveDiffFittingAsciiAlg(const std::string &tableWorkspace,
                               std::string &filePath) {

  // split to get run number and bank
  auto fileSplit = m_view->splitFittingDirectory(filePath);
  // returns ['ENGINX', <RUN-NUMBER>, 'focused', `bank`, <BANK>, '.nxs']
  auto runNumber = fileSplit[1];

  // if a normal focused file assign bank number otherwise 'customised'
  std::string bank = "customised";
  if (fileSplit.size() == 5)
    bank = fileSplit[4];

  // generate file name
  std::string fileName;
  if (g_multi_run.size() > 1) {
    fileName = "ENGINX_" + g_multi_run.front() + "-" + g_multi_run.back() +
               "_Single_Peak_Fitting.csv";
  } else {
    fileName = "ENGINX_" + runNumber + "_Single_Peak_Fitting.csv";
  }

  // separate folder for Single Peak Fitting output;
  auto dir = outFilesUserDir("SinglePeakFitting");
  dir.append(fileName);

  // save the results
  // run the algorithm SaveDiffFittingAscii with output of EnggFitPeaks
  auto saveDiffFit = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
      "SaveDiffFittingAscii");

  try {
    saveDiffFit->initialize();
    saveDiffFit->setProperty("InputWorkspace", tableWorkspace);
    saveDiffFit->setProperty("Filename", dir.toString());
    saveDiffFit->setProperty("RunNumber", runNumber);
    saveDiffFit->setProperty("Bank", bank);
    saveDiffFit->setProperty("OutMode", "AppendToExistingFile");
    saveDiffFit->execute();
  } catch (std::exception &re) {
    g_log.error() << "Could not run the algorithm SaveDiffFittingAscii "
                     "successfully for bank, "
                     // bank name
                     "Error description: " +
                         static_cast<std::string>(re.what()) +
                         " Please check also the log message for detail.\n";
  }
}

void EnggDiffFittingPresenter::runFittingAlgs(
    std::string focusedFitPeaksTableName, std::string focusedWSName) {
  // retrieve the table with parameters
  auto &ADS = Mantid::API::AnalysisDataService::Instance();
  if (!ADS.doesExist(focusedFitPeaksTableName)) {
    // convert units so valid dSpacing peaks can still be added to gui
    if (ADS.doesExist(g_focusedFittingWSName)) {
      convertUnits(g_focusedFittingWSName);
    }

    throw std::invalid_argument(
        focusedFitPeaksTableName +
        " workspace could not be found. "
        "Please check the log messages for more details.");
  }

  auto table = ADS.retrieveWS<ITableWorkspace>(focusedFitPeaksTableName);
  size_t rowCount = table->rowCount();
  const std::string single_peak_out_WS = "engggui_fitting_single_peaks";
  std::string currentPeakOutWS;

  std::string Bk2BkExpFunctionStr;
  std::string startX = "";
  std::string endX = "";
  for (size_t i = 0; i < rowCount; i++) {
    // get the functionStrFactory to generate the string for function
    // property, returns the string with i row from table workspace
    // table is just passed so it works?
    Bk2BkExpFunctionStr =
        functionStrFactory(table, focusedFitPeaksTableName, i, startX, endX);

    g_log.debug() << "startX: " + startX + " . endX: " + endX << '\n';

    currentPeakOutWS = "__engggui_fitting_single_peaks" + std::to_string(i);

    // run EvaluateFunction algorithm with focused workspace to produce
    // the correct fit function
    // focusedWSName is not going to change as its always going to be from
    // single workspace
    runEvaluateFunctionAlg(Bk2BkExpFunctionStr, focusedWSName, currentPeakOutWS,
                           startX, endX);

    // crop workspace so only the correct workspace index is plotted
    runCropWorkspaceAlg(currentPeakOutWS);

    // apply the same binning as a focused workspace
    runRebinToWorkspaceAlg(currentPeakOutWS);

    // if the first peak
    if (i == size_t(0)) {

      // create a workspace clone of bank focus file
      // this will import all information of the previous file
      runCloneWorkspaceAlg(focusedWSName, single_peak_out_WS);

      setDataToClonedWS(currentPeakOutWS, single_peak_out_WS);
      ADS.remove(currentPeakOutWS);
    } else {
      const std::string currentPeakClonedWS =
          "__engggui_fitting_cloned_peaks" + std::to_string(i);

      runCloneWorkspaceAlg(focusedWSName, currentPeakClonedWS);

      setDataToClonedWS(currentPeakOutWS, currentPeakClonedWS);

      // append all peaks in to single workspace & remove
      runAppendSpectraAlg(single_peak_out_WS, currentPeakClonedWS);
      ADS.remove(currentPeakOutWS);
      ADS.remove(currentPeakClonedWS);
    }
  }

  convertUnits(g_focusedFittingWSName);

  // convert units for both workspaces to dSpacing from ToF
  if (rowCount > size_t(0)) {
    auto swks = ADS.retrieveWS<MatrixWorkspace>(single_peak_out_WS);
    setDifcTzero(swks);
    convertUnits(single_peak_out_WS);
  } else {
    g_log.error() << "The engggui_fitting_fitpeaks_params table produced is"
                     "empty. Please try again!\n";
  }

  m_fittingFinishedOK = true;
}

std::string EnggDiffFittingPresenter::functionStrFactory(
    Mantid::API::ITableWorkspace_sptr &paramTableWS, std::string tableName,
    size_t row, std::string &startX, std::string &endX) {
  const double windowLeft = 9;
  const double windowRight = 12;

  AnalysisDataServiceImpl &ADS = Mantid::API::AnalysisDataService::Instance();
  paramTableWS = ADS.retrieveWS<ITableWorkspace>(tableName);

  double A0 = paramTableWS->cell<double>(row, size_t(1));
  double A1 = paramTableWS->cell<double>(row, size_t(3));
  double I = paramTableWS->cell<double>(row, size_t(13));
  double A = paramTableWS->cell<double>(row, size_t(7));
  double B = paramTableWS->cell<double>(row, size_t(9));
  double X0 = paramTableWS->cell<double>(row, size_t(5));
  double S = paramTableWS->cell<double>(row, size_t(11));

  startX = boost::lexical_cast<std::string>(X0 - (windowLeft * S));
  endX = boost::lexical_cast<std::string>(X0 + (windowRight * S));

  std::string functionStr =
      "name=LinearBackground,A0=" + boost::lexical_cast<std::string>(A0) +
      ",A1=" + boost::lexical_cast<std::string>(A1) +
      ";name=BackToBackExponential,I=" + boost::lexical_cast<std::string>(I) +
      ",A=" + boost::lexical_cast<std::string>(A) + ",B=" +
      boost::lexical_cast<std::string>(B) + ",X0=" +
      boost::lexical_cast<std::string>(X0) + ",S=" +
      boost::lexical_cast<std::string>(S);

  return functionStr;
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
  } else {
    auto expPeaks = m_view->fittingPeaksData();
    outfile << expPeaks;
  }
}

void EnggDiffFittingPresenter::runEvaluateFunctionAlg(
    const std::string &bk2BkExpFunction, const std::string &InputName,
    const std::string &OutputName, const std::string &startX,
    const std::string &endX) {

  auto evalFunc = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
      "EvaluateFunction");
  g_log.notice() << "EvaluateFunction algorithm has started\n";
  try {
    evalFunc->initialize();
    evalFunc->setProperty("Function", bk2BkExpFunction);
    evalFunc->setProperty("InputWorkspace", InputName);
    evalFunc->setProperty("OutputWorkspace", OutputName);
    evalFunc->setProperty("StartX", startX);
    evalFunc->setProperty("EndX", endX);
    evalFunc->execute();
  } catch (std::runtime_error &re) {
    g_log.error() << "Could not run the algorithm EvaluateFunction, "
                     "Error description: " +
                         static_cast<std::string>(re.what()) << '\n';
  }
}

void EnggDiffFittingPresenter::runCropWorkspaceAlg(std::string workspaceName) {
  auto cropWS = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
      "CropWorkspace");
  try {
    cropWS->initialize();
    cropWS->setProperty("InputWorkspace", workspaceName);
    cropWS->setProperty("OutputWorkspace", workspaceName);
    cropWS->setProperty("StartWorkspaceIndex", 1);
    cropWS->setProperty("EndWorkspaceIndex", 1);
    cropWS->execute();
  } catch (std::runtime_error &re) {
    g_log.error() << "Could not run the algorithm CropWorkspace, "
                     "Error description: " +
                         static_cast<std::string>(re.what()) << '\n';
  }
}

void EnggDiffFittingPresenter::runAppendSpectraAlg(std::string workspace1Name,
                                                   std::string workspace2Name) {
  auto appendSpec = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
      "AppendSpectra");
  try {
    appendSpec->initialize();
    appendSpec->setProperty("InputWorkspace1", workspace1Name);
    appendSpec->setProperty("InputWorkspace2", workspace2Name);
    appendSpec->setProperty("OutputWorkspace", workspace1Name);
    appendSpec->execute();
  } catch (std::runtime_error &re) {
    g_log.error() << "Could not run the algorithm AppendWorkspace, "
                     "Error description: " +
                         static_cast<std::string>(re.what()) << '\n';
  }
}

void EnggDiffFittingPresenter::runRebinToWorkspaceAlg(
    std::string workspaceName) {
  auto RebinToWs = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
      "RebinToWorkspace");
  try {
    RebinToWs->initialize();
    RebinToWs->setProperty("WorkspaceToRebin", workspaceName);
    RebinToWs->setProperty("WorkspaceToMatch", g_focusedFittingWSName);
    RebinToWs->setProperty("OutputWorkspace", workspaceName);
    RebinToWs->execute();
  } catch (std::runtime_error &re) {
    g_log.error() << "Could not run the algorithm RebinToWorkspace, "
                     "Error description: " +
                         static_cast<std::string>(re.what()) << '\n';
  }
}

/**
 * Converts from time-of-flight to d-spacing
 *
 * @param workspaceName name of the workspace to convert (in place)
 */
void EnggDiffFittingPresenter::convertUnits(std::string workspaceName) {
  // Here using the GSAS (DIFC, TZERO) parameters seems preferred
  if (g_useAlignDetectors) {
    runAlignDetectorsAlg(workspaceName);
  } else {
    runConvertUnitsAlg(workspaceName);
  }
}

void EnggDiffFittingPresenter::getDifcTzero(MatrixWorkspace_const_sptr wks,
                                            double &difc, double &difa,
                                            double &tzero) const {

  try {
    const auto run = wks->run();
    // long, step by step way:
    // auto propC = run.getLogData("difc");
    // auto doubleC =
    //     dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(propC);
    // if (!doubleC)
    //   throw Mantid::Kernel::Exception::NotFoundError(
    //       "Required difc property not found in workspace.", "difc");
    difc = run.getPropertyValueAsType<double>("difc");
    difa = run.getPropertyValueAsType<double>("difa");
    tzero = run.getPropertyValueAsType<double>("tzero");

  } catch (std::runtime_error &rexc) {
    // fallback to something reasonable / approximate values so
    // the fitting tab can work minimally
    difa = tzero = 0.0;
    difc = 18400;
    g_log.warning()
        << "Could not retrieve the DIFC, DIFA, TZERO values from the workspace "
        << wks->name() << ". Using default, which is not adjusted for this "
                          "workspace/run: DIFA: " << difa << ", DIFC: " << difc
        << ", TZERO: " << tzero << ". Error details: " << rexc.what() << '\n';
  }
}

/**
 * Converts units from time-of-flight to d-spacing, using
 * AlignDetectors.  This is the GSAS-style alternative to using the
 * algorithm ConvertUnits.  Needs to make sure that the workspace is
 * not of distribution type (and use the algorithm
 * ConvertFromDistribution if it is). This is a requirement of
 * AlignDetectors.
 *
 * @param workspaceName name of the workspace to convert
 */
void EnggDiffFittingPresenter::runAlignDetectorsAlg(std::string workspaceName) {
  const std::string targetUnit = "dSpacing";
  const std::string algName = "AlignDetectors";

  const auto &ADS = Mantid::API::AnalysisDataService::Instance();
  auto inputWS = ADS.retrieveWS<MatrixWorkspace>(workspaceName);
  if (!inputWS)
    return;

  double difc, difa, tzero;
  getDifcTzero(inputWS, difc, difa, tzero);

  // create a table with the GSAS calibration parameters
  ITableWorkspace_sptr difcTable;
  try {
    difcTable = Mantid::API::WorkspaceFactory::Instance().createTable();
    if (!difcTable) {
      return;
    }
    difcTable->addColumn("int", "detid");
    difcTable->addColumn("double", "difc");
    difcTable->addColumn("double", "difa");
    difcTable->addColumn("double", "tzero");
    TableRow row = difcTable->appendRow();
    auto &spec = inputWS->getSpectrum(0);
    Mantid::detid_t detID = *(spec.getDetectorIDs().cbegin());

    row << detID << difc << difa << tzero;
  } catch (std::runtime_error &rexc) {
    g_log.error() << "Failed to prepare calibration table input to convert "
                     "units with the algorithm " << algName
                  << ". Error details: " << rexc.what() << '\n';
    return;
  }

  // AlignDetectors doesn't take distribution workspaces (it enforces
  // RawCountValidator)
  if (inputWS->isDistribution()) {
    try {
      auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
          "ConvertFromDistribution");
      alg->initialize();
      alg->setProperty("Workspace", workspaceName);
      alg->execute();
    } catch (std::runtime_error &rexc) {
      g_log.error() << "Could not run ConvertFromDistribution. Error: "
                    << rexc.what() << '\n';
      return;
    }
  }

  try {
    auto alg =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged(algName);
    alg->initialize();
    alg->setProperty("InputWorkspace", workspaceName);
    alg->setProperty("OutputWorkspace", workspaceName);
    alg->setProperty("CalibrationWorkspace", difcTable);
    alg->execute();
  } catch (std::runtime_error &rexc) {
    g_log.error() << "Could not run the algorithm " << algName
                  << " to convert workspace to " << targetUnit
                  << ", Error details: " + static_cast<std::string>(rexc.what())
                  << '\n';
  }
}

void EnggDiffFittingPresenter::runConvertUnitsAlg(std::string workspaceName) {
  const std::string targetUnit = "dSpacing";
  auto ConvertUnits =
      Mantid::API::AlgorithmManager::Instance().createUnmanaged("ConvertUnits");
  try {
    ConvertUnits->initialize();
    ConvertUnits->setProperty("InputWorkspace", workspaceName);
    ConvertUnits->setProperty("OutputWorkspace", workspaceName);
    ConvertUnits->setProperty("Target", targetUnit);
    ConvertUnits->setPropertyValue("EMode", "Elastic");
    ConvertUnits->execute();
  } catch (std::runtime_error &re) {
    g_log.error() << "Could not run the algorithm ConvertUnits to convert "
                     "workspace to " << targetUnit
                  << ", Error description: " +
                         static_cast<std::string>(re.what()) << '\n';
  }
}

void EnggDiffFittingPresenter::runCloneWorkspaceAlg(
    std::string inputWorkspace, const std::string &outputWorkspace) {

  auto cloneWorkspace =
      Mantid::API::AlgorithmManager::Instance().createUnmanaged(
          "CloneWorkspace");
  try {
    cloneWorkspace->initialize();
    cloneWorkspace->setProperty("InputWorkspace", inputWorkspace);
    cloneWorkspace->setProperty("OutputWorkspace", outputWorkspace);
    cloneWorkspace->execute();
  } catch (std::runtime_error &re) {
    g_log.error() << "Could not run the algorithm CreateWorkspace, "
                     "Error description: " +
                         static_cast<std::string>(re.what()) << '\n';
  }
}

void EnggDiffFittingPresenter::setDataToClonedWS(std::string &current_WS,
                                                 const std::string &cloned_WS) {
  AnalysisDataServiceImpl &ADS = Mantid::API::AnalysisDataService::Instance();
  auto currentPeakWS = ADS.retrieveWS<MatrixWorkspace>(current_WS);
  auto currentClonedWS = ADS.retrieveWS<MatrixWorkspace>(cloned_WS);
  currentClonedWS->dataY(0) = currentPeakWS->readY(0);
  currentClonedWS->dataE(0) = currentPeakWS->readE(0);
}

void EnggDiffFittingPresenter::setBankItems() {
  try {
    auto fitting_runno_vector = m_view->getFittingRunNumVec();

    if (!fitting_runno_vector.empty()) {

      // delete previous bank added to the list
      m_view->clearFittingComboBox();

      for (size_t i = 0; i < fitting_runno_vector.size(); i++) {
        Poco::Path vecFile(fitting_runno_vector[i]);
        std::string strVecFile = vecFile.toString();
        // split the directory from m_fitting_runno_dir_vec
        std::vector<std::string> vecFileSplit =
            m_view->splitFittingDirectory(strVecFile);

        // get the last split in vector which will be bank
        std::string bankID = (vecFileSplit.back());

        bool digit = isDigit(bankID);

        if (digit || bankID == "cropped") {
          m_view->addBankItem(bankID);
        } else {
          QString qBank = QString("Bank %1").arg(i + 1);
          m_view->addBankItem(qBank.toStdString());
        }
      }

      m_view->enableFittingComboBox(true);
    } else {
      // upon invalid file
      // disable the widgets when only one related file found
      m_view->enableFittingComboBox(false);

      m_view->clearFittingComboBox();
    }

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
    auto intialDir = firstDir;
    m_view->setFittingRunNo(intialDir);
  }
  // if nothing found related to text-field input
  else if (!m_view->getFittingRunNo().empty())
    m_view->setFittingRunNo(selectedFile);
}

bool EnggDiffFittingPresenter::isDigit(const std::string text) const {
	return std::all_of(text.cbegin(), text.cend(), ::isdigit);
}

void EnggDiffFittingPresenter::plotFitPeaksCurves() {
  AnalysisDataServiceImpl &ADS = Mantid::API::AnalysisDataService::Instance();
  std::string singlePeaksWs = "engggui_fitting_single_peaks";

  if (!ADS.doesExist(singlePeaksWs) && !ADS.doesExist(g_focusedFittingWSName)) {
    g_log.error() << "Fitting results could not be plotted as there is no " +
                         singlePeaksWs + " or " + g_focusedFittingWSName +
                         " workspace found.\n";
    m_view->showStatus("Error while fitting peaks");
    return;
  }

  try {
    auto focusedPeaksWS =
        ADS.retrieveWS<MatrixWorkspace>(g_focusedFittingWSName);
    auto focusedData = ALCHelper::curveDataFromWs(focusedPeaksWS);
    m_view->setDataVector(focusedData, true, m_fittingFinishedOK);

    if (m_fittingFinishedOK) {
      g_log.debug() << "single peaks fitting being plotted now.\n";
      auto singlePeaksWS = ADS.retrieveWS<MatrixWorkspace>(singlePeaksWs);
      auto singlePeaksData = ALCHelper::curveDataFromWs(singlePeaksWS);
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
