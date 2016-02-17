#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtAPI/PythonRunner.h"
// #include "MantidQtCustomInterfaces/EnggDiffraction/EnggDiffractionModel.h"
#include "MantidQtCustomInterfaces/EnggDiffraction/EnggDiffractionPresenter.h"
#include "MantidQtCustomInterfaces/EnggDiffraction/EnggDiffractionPresWorker.h"
#include "MantidQtCustomInterfaces/EnggDiffraction/IEnggDiffractionView.h"

#include <fstream>

#include <boost/lexical_cast.hpp>

#include <Poco/File.h>

#include <QThread>
#include <MantidAPI/AlgorithmManager.h>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;

namespace MantidQt {
namespace CustomInterfaces {

namespace {
Mantid::Kernel::Logger g_log("EngineeringDiffractionGUI");
}

const std::string EnggDiffractionPresenter::g_enginxStr = "ENGINX";

const std::string EnggDiffractionPresenter::g_runNumberErrorStr =
    " cannot be empty, must be an integer number, valid ENGINX run number/s "
    "or "
    "valid directory/directories.";

// discouraged at the moment
const bool EnggDiffractionPresenter::g_askUserCalibFilename = false;
const std::string EnggDiffractionPresenter::g_vanIntegrationWSName =
    "engggui_vanadium_integration_ws";
int EnggDiffractionPresenter::g_croppedCounter = 0;
int EnggDiffractionPresenter::g_plottingCounter = 0;
bool EnggDiffractionPresenter::g_abortThread = false;
std::string EnggDiffractionPresenter::g_lastValidRun = "";
std::string EnggDiffractionPresenter::g_calibCropIdentifier = "SpectrumNumbers";
std::string EnggDiffractionPresenter::g_sumOfFilesFocus = "";

EnggDiffractionPresenter::EnggDiffractionPresenter(IEnggDiffractionView *view)
    : m_workerThread(NULL), m_calibFinishedOK(false), m_focusFinishedOK(false),
      m_rebinningFinishedOK(false),
      m_view(view) /*, m_model(new EnggDiffractionModel()), */ {
  if (!m_view) {
    throw std::runtime_error(
        "Severe inconsistency found. Presenter created "
        "with an empty/null view (engineeering diffraction interface). "
        "Cannot continue.");
  }
}

EnggDiffractionPresenter::~EnggDiffractionPresenter() { cleanup(); }

/**
* Close open sessions, kill threads etc., save settings, etc. for a
* graceful window close/destruction
*/
void EnggDiffractionPresenter::cleanup() {
  // m_model->cleanup();

  // this may still be running
  if (m_workerThread) {
    if (m_workerThread->isRunning()) {
      g_log.notice() << "A calibration process is currently running, shutting "
                        "it down immediately..."
                     << std::endl;
      m_workerThread->wait(10);
    }
    delete m_workerThread;
    m_workerThread = NULL;
  }
}

void EnggDiffractionPresenter::notify(
    IEnggDiffractionPresenter::Notification notif) {

  switch (notif) {

  case IEnggDiffractionPresenter::Start:
    processStart();
    break;

  case IEnggDiffractionPresenter::LoadExistingCalib:
    processLoadExistingCalib();
    break;

  case IEnggDiffractionPresenter::CalcCalib:
    processCalcCalib();
    break;

  case IEnggDiffractionPresenter::CropCalib:
    ProcessCropCalib();
    break;

  case IEnggDiffractionPresenter::FocusRun:
    processFocusBasic();
    break;

  case IEnggDiffractionPresenter::FocusCropped:
    processFocusCropped();
    break;

  case IEnggDiffractionPresenter::FocusTexture:
    processFocusTexture();
    break;

  case IEnggDiffractionPresenter::ResetFocus:
    processResetFocus();
    break;

  case IEnggDiffractionPresenter::RebinTime:
    processRebinTime();
    break;

  case IEnggDiffractionPresenter::RebinMultiperiod:
    processRebinMultiperiod();
    break;

  case IEnggDiffractionPresenter::LogMsg:
    processLogMsg();
    break;

  case IEnggDiffractionPresenter::InstrumentChange:
    processInstChange();
    break;

  case IEnggDiffractionPresenter::RBNumberChange:
    processRBNumberChange();
    break;

  case IEnggDiffractionPresenter::ShutDown:
    processShutDown();
    break;

  case IEnggDiffractionPresenter::StopFocus:
    processStopFocus();
    break;
  }
}

void EnggDiffractionPresenter::processStart() {
  EnggDiffCalibSettings cs = m_view->currentCalibSettings();
}

void EnggDiffractionPresenter::processLoadExistingCalib() {
  EnggDiffCalibSettings cs = m_view->currentCalibSettings();

  std::string fname = m_view->askExistingCalibFilename();
  if (fname.empty()) {
    return;
  }

  std::string instName, vanNo, ceriaNo;
  try {
    parseCalibrateFilename(fname, instName, vanNo, ceriaNo);
  } catch (std::invalid_argument &ia) {
    m_view->userWarning("Invalid calibration filename : " + fname, ia.what());
    return;
  }

  m_view->newCalibLoaded(vanNo, ceriaNo, fname);
}

void EnggDiffractionPresenter::processCalcCalib() {
  const std::string vanNo = isValidRunNumber(m_view->newVanadiumNo());
  const std::string ceriaNo = isValidRunNumber(m_view->newCeriaNo());
  try {
    inputChecksBeforeCalibrate(vanNo, ceriaNo);
  } catch (std::invalid_argument &ia) {
    m_view->userWarning("Error in the inputs required for calibrate",
                        ia.what());
    return;
  }
  g_log.notice() << "EnggDiffraction GUI: starting new calibration. This may "
                    "take a few seconds... "
                 << std::endl;

  const std::string outFilename = outputCalibFilename(vanNo, ceriaNo);

  m_view->enableCalibrateAndFocusActions(false);
  // alternatively, this would be GUI-blocking:
  // doNewCalibration(outFilename, vanNo, ceriaNo, specNos);
  // calibrationFinished()
  startAsyncCalibWorker(outFilename, vanNo, ceriaNo, "");
}

void EnggDiffractionPresenter::ProcessCropCalib() {
  const std::string vanNo = isValidRunNumber(m_view->newVanadiumNo());
  const std::string ceriaNo = isValidRunNumber(m_view->newCeriaNo());
  int specIdNum = m_view->currentCropCalibBankName();
  enum BankMode { SPECIDS = 0, NORTH = 1, SOUTH = 2 };

  try {
    inputChecksBeforeCalibrate(vanNo, ceriaNo);
    if (m_view->currentCalibSpecNos().empty() &&
        specIdNum == BankMode::SPECIDS) {
      throw std::invalid_argument(
          "The Spectrum IDs cannot be empty, must be a"
          "valid range or a Bank Name can be selected instead");
    }
  } catch (std::invalid_argument &ia) {
    m_view->userWarning("Error in the inputs required for calibrate",
                        ia.what());
    return;
  }

  g_log.notice()
      << "EnggDiffraction GUI: starting cropped calibration. This may "
         "take a few seconds... "
      << std::endl;

  const std::string outFilename = outputCalibFilename(vanNo, ceriaNo);

  std::string specId = "";
  if (specIdNum == BankMode::NORTH) {
    specId = "North";
    g_calibCropIdentifier = "Bank";

  } else if (specIdNum == BankMode::SOUTH) {
    specId = "South";
    g_calibCropIdentifier = "Bank";

  } else if (specIdNum == BankMode::SPECIDS) {
    specId = m_view->currentCalibSpecNos();
  }

  m_view->enableCalibrateAndFocusActions(false);
  // alternatively, this would be GUI-blocking:
  // doNewCalibration(outFilename, vanNo, ceriaNo, specID/bankName);
  // calibrationFinished()
  startAsyncCalibWorker(outFilename, vanNo, ceriaNo, specId);
}

void EnggDiffractionPresenter::processFocusBasic() {
  std::vector<std::string> multi_RunNo =
      isValidMultiRunNumber(m_view->focusingRunNo());
  const std::vector<bool> banks = m_view->focusingBanks();

  // reset global values
  g_abortThread = false;
  g_sumOfFilesFocus = "";
  g_plottingCounter = 0;

  // check if valid run number provided before focusin
  try {
    inputChecksBeforeFocusBasic(multi_RunNo, banks);
  } catch (std::invalid_argument &ia) {
    m_view->userWarning("Error in the inputs required to focus a run",
                        ia.what());
    return;
  }

  int focusMode = m_view->currentMultiRunMode();
  if (focusMode == 0) {
    g_log.debug() << " focus mode selected Individual Run Files Separately "
                  << std::endl;

    // start focusing
    startFocusing(multi_RunNo, banks, "", "");

  } else if (focusMode == 1) {
    g_log.debug() << " focus mode selected Focus Sum Of Files " << std::endl;
    g_sumOfFilesFocus = "basic";
    std::vector<std::string> firstRun;
    firstRun.push_back(multi_RunNo[0]);

    // to avoid multiple loops, use firstRun instead as the
    // multi-run number is not required for sumOfFiles
    startFocusing(firstRun, banks, "", "");
  }
}

void EnggDiffractionPresenter::processFocusCropped() {
  const std::vector<std::string> multi_RunNo =
      isValidMultiRunNumber(m_view->focusingCroppedRunNo());
  const std::vector<bool> banks = m_view->focusingBanks();
  const std::string specNos = m_view->focusingCroppedSpectrumIDs();

  // reset global values
  g_abortThread = false;
  g_sumOfFilesFocus = "";
  g_plottingCounter = 0;

  // check if valid run number provided before focusin
  try {
    inputChecksBeforeFocusCropped(multi_RunNo, banks, specNos);
  } catch (std::invalid_argument &ia) {
    m_view->userWarning(
        "Error in the inputs required to focus a run (in cropped mode)",
        ia.what());
    return;
  }

  int focusMode = m_view->currentMultiRunMode();
  if (focusMode == 0) {
    g_log.debug() << " focus mode selected Individual Run Files Separately "
                  << std::endl;

    startFocusing(multi_RunNo, banks, specNos, "");

  } else if (focusMode == 1) {
    g_log.debug() << " focus mode selected Focus Sum Of Files " << std::endl;
    g_sumOfFilesFocus = "cropped";
    std::vector<std::string> firstRun{multi_RunNo[0]};

    // to avoid multiple loops, use firstRun instead as the
    // multi-run number is not required for sumOfFiles
    startFocusing(firstRun, banks, specNos, "");
  }
}

void EnggDiffractionPresenter::processFocusTexture() {
  const std::vector<std::string> multi_RunNo =
      isValidMultiRunNumber(m_view->focusingTextureRunNo());
  const std::string dgFile = m_view->focusingTextureGroupingFile();

  // reset global values
  g_abortThread = false;
  g_sumOfFilesFocus = "";
  g_plottingCounter = 0;

  // check if valid run number provided before focusing
  try {
    inputChecksBeforeFocusTexture(multi_RunNo, dgFile);
  } catch (std::invalid_argument &ia) {
    m_view->userWarning(
        "Error in the inputs required to focus a run (in texture mode)",
        ia.what());
    return;
  }

  int focusMode = m_view->currentMultiRunMode();
  if (focusMode == 0) {
    g_log.debug() << " focus mode selected Individual Run Files Separately "
                  << std::endl;
    startFocusing(multi_RunNo, std::vector<bool>(), "", dgFile);

  } else if (focusMode == 1) {
    g_log.debug() << " focus mode selected Focus Sum Of Files " << std::endl;
    g_sumOfFilesFocus = "texture";
    std::vector<std::string> firstRun{multi_RunNo[0]};

    // to avoid multiple loops, use firstRun instead as the
    // multi-run number is not required for sumOfFiles
    startFocusing(firstRun, std::vector<bool>(), "", dgFile);
  }
}

/**
* Starts a focusing worker, for different modes depending on the
* inputs provided. Assumes that the inputs have been checked by the
* respective specific processFocus methods (for normal, cropped,
* texture, etc. focusing).
*
* @param multi_RunNo vector of run/file number to focus
* @param banks banks to include in the focusing, processed one at a time
*
* @param specNos list of spectra to use when focusing. If not empty
* this implies focusing in cropped mode.
*
* @param dgFile detector grouping file to define banks (texture). If
* not empty, this implies focusing in texture mode.
*/
void EnggDiffractionPresenter::startFocusing(
    const std::vector<std::string> &multi_RunNo, const std::vector<bool> &banks,
    const std::string &specNos, const std::string &dgFile) {

  std::string optMsg = "";
  if (!specNos.empty()) {
    optMsg = " (cropped)";
  } else if (!dgFile.empty()) {
    optMsg = " (texture)";
  }
  g_log.notice() << "EnggDiffraction GUI: starting new focusing" << optMsg
                 << ". This may take some seconds... " << std::endl;

  const std::string focusDir = m_view->focusingDir();

  m_view->enableCalibrateAndFocusActions(false);
  // GUI-blocking alternative:
  // doFocusRun(focusDir, outFilenames, runNo, banks, specNos, dgFile)
  // focusingFinished()

  startAsyncFocusWorker(focusDir, multi_RunNo, banks, specNos, dgFile);
}

void EnggDiffractionPresenter::processResetFocus() { m_view->resetFocus(); }

void EnggDiffractionPresenter::processRebinTime() {

  const std::string runNo = isValidRunNumber(m_view->currentPreprocRunNo());
  double bin = m_view->rebinningTimeBin();

  try {
    inputChecksBeforeRebinTime(runNo, bin);
  } catch (std::invalid_argument &ia) {
    m_view->userWarning(
        "Error in the inputs required to pre-process (rebin) a run", ia.what());
    return;
  }

  const std::string outWSName = "engggui_preproc_time_ws";
  g_log.notice() << "EnggDiffraction GUI: starting new pre-processing "
                    "(re-binning) with a TOF bin into workspace '" +
                        outWSName + "'. This "
                                    "may take some seconds... "
                 << std::endl;

  m_view->enableCalibrateAndFocusActions(false);
  // GUI-blocking alternative:
  // doRebinningTime(runNo, bin, outWSName)
  // rebinningFinished()
  startAsyncRebinningTimeWorker(runNo, bin, outWSName);
}

void EnggDiffractionPresenter::processRebinMultiperiod() {
  const std::string runNo = isValidRunNumber(m_view->currentPreprocRunNo());
  size_t nperiods = m_view->rebinningPulsesNumberPeriods();
  double timeStep = m_view->rebinningPulsesTime();

  try {
    inputChecksBeforeRebinPulses(runNo, nperiods, timeStep);
  } catch (std::invalid_argument &ia) {
    m_view->userWarning("Error in the inputs required to pre-process (rebin) a "
                        "run by pulse times",
                        ia.what());
    return;
  }
  const std::string outWSName = "engggui_preproc_by_pulse_time_ws";
  g_log.notice() << "EnggDiffraction GUI: starting new pre-processing "
                    "(re-binning) by pulse times into workspace '" +
                        outWSName + "'. This "
                                    "may take some seconds... "
                 << std::endl;

  m_view->enableCalibrateAndFocusActions(false);
  // GUI-blocking alternative:
  // doRebinningPulses(runNo, nperiods, timeStep, outWSName)
  // rebinningFinished()
  startAsyncRebinningPulsesWorker(runNo, nperiods, timeStep, outWSName);
}

void EnggDiffractionPresenter::processLogMsg() {
  std::vector<std::string> msgs = m_view->logMsgs();
  for (size_t i = 0; i < msgs.size(); i++) {
    g_log.information() << msgs[i] << std::endl;
  }
}

void EnggDiffractionPresenter::processInstChange() {
  const std::string err = "Changing instrument is not supported!";
  g_log.error() << err << std::endl;
  m_view->userError("Fatal error", err);
}

void EnggDiffractionPresenter::processRBNumberChange() {
  const std::string rbn = m_view->getRBNumber();
  m_view->enableTabs(validateRBNumber(rbn));
}

void EnggDiffractionPresenter::processShutDown() {
  m_view->saveSettings();
  cleanup();
}

void EnggDiffractionPresenter::processStopFocus() {
  if (m_workerThread) {
    if (m_workerThread->isRunning()) {
      g_log.notice() << "A focus process is currently running, shutting "
                        "it down as soon as possible..."
                     << std::endl;

      g_abortThread = true;
      g_log.warning() << "Focus Stop has been clicked, please wait until "
                         "current focus run process has been completed. "
                      << std::endl;
    }
  }
}

/**
* Check if an RB number is valid to work with it (retrieve data,
* calibrate, focus, etc.).
*
* @param rbn RB number as given by the user
*/
bool EnggDiffractionPresenter::validateRBNumber(const std::string &rbn) const {
  return !rbn.empty();
}

/**
* Checks if the provided run number is valid and if a direcotory is provided
* it will convert it to a run number
*
* @param dir takes the input/directory of the user
*
* @return run_number 6 character string of a run number
*/
std::string
EnggDiffractionPresenter::isValidRunNumber(std::vector<std::string> dir) {

  std::vector<std::string> run_vec = dir;
  std::string run_number;

  // if empty string
  size_t i = 0;
  if (!dir.empty() && dir.at(i) != "") {

    auto p = run_vec.begin();
    int i = 0;
    while (p != run_vec.end()) {
      run_number = *p;
      p++;
      i++;

      try {
        if (Poco::File(run_number).exists()) {
          Poco::Path inputDir = run_number;
          run_number = "";
          // get file name name via poco::path

          std::string filename = inputDir.getFileName();

          // convert to int or assign it to size_t
          for (size_t i = 0; i < filename.size(); i++) {
            char *str = &filename[i];
            if (std::isdigit(*str)) {
              run_number += filename[i];
            }
          }
          run_number.erase(0, run_number.find_first_not_of('0'));
        }

      } catch (std::runtime_error &re) {
        throw std::invalid_argument("Error browsing selected file: " +
                                    static_cast<std::string>(re.what()));
      } catch (...) {
        throw std::invalid_argument("Error browsing selected file: ");
      }
    }
  }

  g_log.debug() << "run number is: " << run_number << std::endl;

  return run_number;
}

/**
* Checks if the provided run number is valid and if a direcotory is provided
*
* @param dir takes the input/directory of the user
*
* @return vector of string multi_run_number, 6 character string of a run number
*/
std::vector<std::string>
EnggDiffractionPresenter::isValidMultiRunNumber(std::vector<std::string> dir) {

  std::vector<std::string> run_vec = dir;
  std::string run_number;
  std::vector<std::string> multi_run_number;

  // if empty string
  size_t i = 0;
  if (!dir.empty() && dir.at(i) != "") {

    auto p = run_vec.begin();
    int i = 0;
    while (p != run_vec.end()) {
      run_number = *p;
      p++;
      i++;

      try {
        if (Poco::File(run_number).exists()) {
          Poco::Path inputDir = run_number;
          run_number = "";
          // get file name name via poco::path

          std::string filename = inputDir.getFileName();

          // convert to int or assign it to size_t
          for (size_t i = 0; i < filename.size(); i++) {
            char *str = &filename[i];
            if (std::isdigit(*str)) {
              run_number += filename[i];
            }
          }
          run_number.erase(0, run_number.find_first_not_of('0'));
        }
      } catch (std::runtime_error &re) {
        throw std::invalid_argument("Error browsing selected file: " +
                                    static_cast<std::string>(re.what()));
      } catch (...) {
        throw std::invalid_argument("Error browsing selected file: ");
      }

      multi_run_number.push_back(run_number);
    }
  }

  g_log.debug() << "run number selected for multi-run: " << run_number
                << std::endl;

  return multi_run_number;
}

/**
* Does several checks on the current inputs and settings. This should
* be done before starting any calibration work. The message return
* should in principle be shown to the user as a visible message
* (pop-up, error log, etc.)
*
* @param newVanNo number of the Vanadium run for the new calibration
* @param newCeriaNo number of the Ceria run for the new calibration
*
* @throws std::invalid_argument with an informative message.
*/
void EnggDiffractionPresenter::inputChecksBeforeCalibrate(
    const std::string &newVanNo, const std::string &newCeriaNo) {
  if (newVanNo.empty()) {
    throw std::invalid_argument("The Vanadium number" + g_runNumberErrorStr);
  }
  if (newCeriaNo.empty()) {
    throw std::invalid_argument("The Ceria number" + g_runNumberErrorStr);
  }

  EnggDiffCalibSettings cs = m_view->currentCalibSettings();
  const std::string pixelCalib = cs.m_pixelCalibFilename;
  if (pixelCalib.empty()) {
    throw std::invalid_argument(
        "You need to set a pixel (full) calibration in settings.");
  }
  const std::string templGSAS = cs.m_templateGSAS_PRM;
  if (templGSAS.empty()) {
    throw std::invalid_argument(
        "You need to set a template calibration file for GSAS in settings.");
  }
}

/**
* What should be the name of the output GSAS calibration file, given
* the Vanadium and Ceria runs
*
* @param vanNo number of the Vanadium run, which is normally part of the name
* @param ceriaNo number of the Ceria run, which is normally part of the name
*
* @return filename (without the full path)
*/
std::string
EnggDiffractionPresenter::outputCalibFilename(const std::string &vanNo,
                                              const std::string &ceriaNo) {
  std::string outFilename = "";
  const std::string sugg = buildCalibrateSuggestedFilename(vanNo, ceriaNo);
  if (!g_askUserCalibFilename) {
    outFilename = sugg;
  } else {
    outFilename = m_view->askNewCalibrationFilename(sugg);
    if (!outFilename.empty()) {
      // make sure it follows the rules
      try {
        std::string inst, van, ceria;
        parseCalibrateFilename(outFilename, inst, van, ceria);
      } catch (std::invalid_argument &ia) {
        m_view->userWarning(
            "Invalid output calibration filename: " + outFilename, ia.what());
        outFilename = "";
      }
    }
  }

  return outFilename;
}

/**
* Parses the name of a calibration file and guesses the instrument,
* vanadium and ceria run numbers, assuming that the name has been
* build with buildCalibrateSuggestedFilename().
*
* @todo this is currently based on the filename. This method should
* do a basic sanity check that the file is actually a calibration
* file (IPARM file for GSAS), and get the numbers from the file not
* from the name of the file. Leaving this as a TODO for now, as the
* file template and contents are still subject to change.
*
* @param path full path to a calibration file
* @param instName instrument used in the file
* @param vanNo number of the Vanadium run used for this calibration file
* @param ceriaNo number of the Ceria run
*
* @throws invalid_argument with an informative message if tha name does
* not look correct (does not follow the conventions).
*/
void EnggDiffractionPresenter::parseCalibrateFilename(const std::string &path,
                                                      std::string &instName,
                                                      std::string &vanNo,
                                                      std::string &ceriaNo) {
  instName = "";
  vanNo = "";
  ceriaNo = "";

  Poco::Path fullPath(path);
  const std::string filename = fullPath.getFileName();
  if (filename.empty()) {
    return;
  }

  const std::string explMsg =
      "Expected a file name like 'INSTR_vanNo_ceriaNo_....par', "
      "where INSTR is the instrument name and vanNo and ceriaNo are the "
      "numbers of the Vanadium and calibration sample (Ceria, CeO2) runs.";
  std::vector<std::string> parts;
  boost::split(parts, filename, boost::is_any_of("_"));
  if (parts.size() < 4) {
    throw std::invalid_argument(
        "Failed to find at least the 4 required parts of the file name.\n\n" +
        explMsg);
  }

  // check the rules on the file name
  if (g_enginxStr != parts[0]) {
    throw std::invalid_argument("The first component of the file name is not "
                                "the expected instrument name: " +
                                g_enginxStr + ".\n\n" + explMsg);
  }
  const std::string castMsg =
      "It is not possible to interpret as an integer number ";
  try {
    boost::lexical_cast<int>(parts[1]);
  } catch (std::runtime_error &) {
    throw std::invalid_argument(
        castMsg + "the Vanadium number part of the file name.\n\n" + explMsg);
  }
  try {
    boost::lexical_cast<int>(parts[2]);
  } catch (std::runtime_error &) {
    throw std::invalid_argument(
        castMsg + "the Ceria number part of the file name.\n\n" + explMsg);
  }

  instName = parts[0];
  vanNo = parts[1];
  ceriaNo = parts[2];
}

/**
* Start the calibration work without blocking the GUI. This uses
* connect for Qt signals/slots so that it runs well with the Qt event
* loop. Because of that this class needs to be a Q_OBJECT.
*
* @param outFilename name for the output GSAS calibration file
* @param vanNo vanadium run number
* @param ceriaNo ceria run number
* @param specNos specIDs or bank name to be passed
*/
void EnggDiffractionPresenter::startAsyncCalibWorker(
    const std::string &outFilename, const std::string &vanNo,
    const std::string &ceriaNo, const std::string &specNos) {
  delete m_workerThread;
  m_workerThread = new QThread(this);
  EnggDiffWorker *worker =
      new EnggDiffWorker(this, outFilename, vanNo, ceriaNo, specNos);
  worker->moveToThread(m_workerThread);

  connect(m_workerThread, SIGNAL(started()), worker, SLOT(calibrate()));
  connect(worker, SIGNAL(finished()), this, SLOT(calibrationFinished()));
  // early delete of thread and worker
  connect(m_workerThread, SIGNAL(finished()), m_workerThread,
          SLOT(deleteLater()), Qt::DirectConnection);
  connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
  m_workerThread->start();
}

/**
* Calculate a new calibration. This is what threads/workers should
* use to run the calculations in response to the user clicking
* 'calibrate' or similar.
*
* @param outFilename name for the output GSAS calibration file
* @param vanNo vanadium run number
* @param ceriaNo ceria run number
* @param specNos specIDs or bank name to be passed
*/
void EnggDiffractionPresenter::doNewCalibration(const std::string &outFilename,
                                                const std::string &vanNo,
                                                const std::string &ceriaNo,
                                                const std::string &specNos) {
  g_log.notice() << "Generating new calibration file: " << outFilename
                 << std::endl;

  EnggDiffCalibSettings cs = m_view->currentCalibSettings();
  Mantid::Kernel::ConfigServiceImpl &conf =
      Mantid::Kernel::ConfigService::Instance();
  const std::vector<std::string> tmpDirs = conf.getDataSearchDirs();
  // in principle, the run files will be found from 'DirRaw', and the
  // pre-calculated Vanadium corrections from 'DirCalib'
  if (!cs.m_inputDirCalib.empty() && Poco::File(cs.m_inputDirCalib).exists()) {
    conf.appendDataSearchDir(cs.m_inputDirCalib);
  }
  if (!cs.m_inputDirRaw.empty() && Poco::File(cs.m_inputDirRaw).exists())
    conf.appendDataSearchDir(cs.m_inputDirRaw);

  try {
    m_calibFinishedOK = false;
    doCalib(cs, vanNo, ceriaNo, outFilename, specNos);
    m_calibFinishedOK = true;
  } catch (std::runtime_error &) {
    g_log.error() << "The calibration calculations failed. One of the "
                     "algorithms did not execute correctly. See log messages "
                     "for details. "
                  << std::endl;
  } catch (std::invalid_argument &) {
    g_log.error()
        << "The calibration calculations failed. Some input properties "
           "were not valid. See log messages for details. "
        << std::endl;
  }
  // restore normal data search paths
  conf.setDataSearchDirs(tmpDirs);
}

/**
* Method to call when the calibration work has finished, either from
* a separate thread or not (as in this presenter's' test).
*/
void EnggDiffractionPresenter::calibrationFinished() {
  if (!m_view)
    return;

  m_view->enableCalibrateAndFocusActions(true);
  if (!m_calibFinishedOK) {
    g_log.warning() << "The cablibration did not finish correctly."
                    << std::endl;
  } else {
    const std::string vanNo = isValidRunNumber(m_view->newVanadiumNo());

    const std::string ceriaNo = isValidRunNumber(m_view->newCeriaNo());
    const std::string outFilename =
        buildCalibrateSuggestedFilename(vanNo, ceriaNo);
    m_view->newCalibLoaded(vanNo, ceriaNo, outFilename);
    g_log.notice()
        << "Cablibration finished and ready as 'current calibration'."
        << std::endl;
  }
  if (m_workerThread) {
    delete m_workerThread;
    m_workerThread = NULL;
  }
}

/**
* Build a suggested name for a new calibration, by appending instrument name,
* relevant run numbers, etc., like: ENGINX_241391_236516_both_banks.par
*
* @param vanNo number of the Vanadium run
* @param ceriaNo number of the Ceria run
*
* @return Suggested name for a new calibration file, following
* ENGIN-X practices
*/
std::string EnggDiffractionPresenter::buildCalibrateSuggestedFilename(
    const std::string &vanNo, const std::string &ceriaNo) const {
  // default and only one supported
  std::string instStr = g_enginxStr;
  std::string nameAppendix = "_both_banks";
  std::string curInst = m_view->currentInstrument();
  if ("ENGIN-X" != curInst && "ENGINX" != curInst) {
    instStr = "UNKNOWNINST";
    nameAppendix = "_calibration";
  }

  // default extension for calibration files
  const std::string calibExt = ".prm";
  std::string sugg =
      instStr + "_" + vanNo + "_" + ceriaNo + nameAppendix + calibExt;

  return sugg;
}

/**
* Calculate a calibration, responding the the "new calibration"
* action/button.
*
* @param cs user settings
* @param vanNo Vanadium run number
* @param ceriaNo Ceria run number
* @param outFilename output filename chosen by the user
* @param specNos specIDs or bank name to be passed
*/
void EnggDiffractionPresenter::doCalib(const EnggDiffCalibSettings &cs,
                                       const std::string &vanNo,
                                       const std::string &ceriaNo,
                                       const std::string &outFilename,
                                       const std::string &specNos) {
  ITableWorkspace_sptr vanIntegWS;
  MatrixWorkspace_sptr vanCurvesWS;
  MatrixWorkspace_sptr ceriaWS;

  // save vanIntegWS and vanCurvesWS as open genie
  // see where spec number comes from

  loadOrCalcVanadiumWorkspaces(vanNo, cs.m_inputDirCalib, vanIntegWS,
                               vanCurvesWS, cs.m_forceRecalcOverwrite);

  const std::string instStr = m_view->currentInstrument();
  try {
    auto load =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged("Load");
    load->initialize();
    load->setPropertyValue("Filename", instStr + ceriaNo);
    const std::string ceriaWSName = "engggui_calibration_sample_ws";
    load->setPropertyValue("OutputWorkspace", ceriaWSName);
    load->execute();

    AnalysisDataServiceImpl &ADS = Mantid::API::AnalysisDataService::Instance();
    ceriaWS = ADS.retrieveWS<MatrixWorkspace>(ceriaWSName);
  } catch (std::runtime_error &re) {
    g_log.error()
        << "Error while loading calibration sample data. "
           "Could not run the algorithm Load succesfully for the calibration "
           "sample (run number: " +
               ceriaNo + "). Error description: " + re.what() +
               " Please check also the previous log messages for details.";
    throw;
  }

  // Bank 1 and 2 - ENGIN-X
  // bank 1 - loops once & used for cropped calibration
  // bank 2 - loops twice, one with each bank & used for new calibration
  const size_t bankNo1 = 1;
  const size_t bankNo2 = 2;
  std::vector<double> difc, tzero;

  if (specNos != "") {
    difc.resize(bankNo1);
    tzero.resize(bankNo1);
  } else {
    difc.resize(bankNo2);
    tzero.resize(bankNo2);
  }

  for (size_t i = 0; i < difc.size(); i++) {
    auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
        "EnggCalibrate");
    try {
      alg->initialize();
      alg->setProperty("InputWorkspace", ceriaWS);
      alg->setProperty("VanIntegrationWorkspace", vanIntegWS);
      alg->setProperty("VanCurvesWorkspace", vanCurvesWS);
      if (specNos != "")
        alg->setPropertyValue(g_calibCropIdentifier,
                              boost::lexical_cast<std::string>(specNos));
      else
        alg->setPropertyValue("Bank", boost::lexical_cast<std::string>(i + 1));
      // TODO: figure out what should be done about the list of expected peaks
      // to EnggCalibrate => it should be a default, as in EnggFitPeaks, that
      // should be fixed in a nother ticket/issue
      alg->setPropertyValue(
          "ExpectedPeaks",
          "3.1243, 2.7057, 1.9132, 1.6316, 1.5621, "
          "1.3529, 1.2415, 1.2100, 1.1046, 1.0414, 0.9566, 0.9147, 0.9019, "
          "0.8556, 0.8252, 0.8158, 0.7811");
      const std::string outFitParamsTblName =
          "engggui_calibration_bank_" + boost::lexical_cast<std::string>(i + 1);
      alg->setPropertyValue("FittedPeaks", outFitParamsTblName);
      alg->setPropertyValue("OutputParametersTableName", outFitParamsTblName);
      alg->execute();
    } catch (std::runtime_error &re) {
      g_log.error() << "Error in calibration. ",
          "Could not run the algorithm EnggCalibrate succesfully for bank " +
              boost::lexical_cast<std::string>(i) + ". Error description: " +
              re.what() + " Please check also the log messages for details.";
      throw;
    }
    difc[i] = alg->getProperty("Difc");
    tzero[i] = alg->getProperty("Zero");

    g_log.notice() << " * Bank " << i + 1 << " calibrated, "
                   << "difc: " << difc[i] << ", zero: " << tzero[i]
                   << std::endl;
  }

  // Creates appropriate directory
  Poco::Path saveDir = outFilesDir("Calibration");

  // Double horror: 1st use a python script
  // 2nd: because runPythonCode does this by emitting a signal that goes to
  // MantidPlot,
  // it has to be done in the view (which is a UserSubWindow).
  Poco::Path outFullPath(saveDir);
  outFullPath.append(outFilename);

  m_view->writeOutCalibFile(outFullPath.toString(), difc, tzero);
  g_log.notice() << "Calibration file written as " << outFullPath.toString()
                 << std::endl;
}

/**
* Perform checks specific to normal/basic run focusing in addition to
* the general checks for any focusing (as done by
* inputChecksBeforeFocus() which is called from this method). Use
* always before running 'Focus'
*
* @param multi_RunNo vector of run number to focus
* @param banks which banks to consider in the focusing
*
* @throws std::invalid_argument with an informative message.
*/
void EnggDiffractionPresenter::inputChecksBeforeFocusBasic(
    const std::vector<std::string> &multi_RunNo,
    const std::vector<bool> &banks) {
  if (multi_RunNo.size() == 0) {
    const std::string msg = "The sample run number" + g_runNumberErrorStr;
    throw std::invalid_argument(msg);
  }

  inputChecksBanks(banks);

  inputChecksBeforeFocus();
}

/**
* Perform checks specific to focusing in "cropped" mode, in addition
* to the general checks for any focusing (as done by
* inputChecksBeforeFocus() which is called from this method). Use
* always before running 'FocusCropped'
*
* @param multi_RunNo vector of run number to focus
* @param banks which banks to consider in the focusing
* @param specNos list of spectra (as usual csv list of spectra in Mantid)
*
* @throws std::invalid_argument with an informative message.
*/
void EnggDiffractionPresenter::inputChecksBeforeFocusCropped(
    const std::vector<std::string> &multi_RunNo, const std::vector<bool> &banks,
    const std::string &specNos) {
  if (multi_RunNo.size() == 0) {
    throw std::invalid_argument("To focus cropped the sample run number" +
                                g_runNumberErrorStr);
  }

  if (specNos.empty()) {
    throw std::invalid_argument("The list of spectrum IDs cannot be empty when "
                                "focusing in 'cropped' mode.");
  }

  inputChecksBanks(banks);

  inputChecksBeforeFocus();
}

/**
* Perform checks specific to focusing in "texture" mode, in addition
* to the general checks for any focusing (as done by
* inputChecksBeforeFocus() which is called from this method). Use
* always before running 'FocusCropped'
*
* @param multi_RunNo vector of run number to focus
* @param dgFile file with detector grouping info
*
* @throws std::invalid_argument with an informative message.
*/
void EnggDiffractionPresenter::inputChecksBeforeFocusTexture(
    const std::vector<std::string> &multi_RunNo, const std::string &dgFile) {
  if (multi_RunNo.size() == 0) {
    throw std::invalid_argument("To focus texture banks the sample run number" +
                                g_runNumberErrorStr);
  }

  if (dgFile.empty()) {
    throw std::invalid_argument("A detector grouping file needs to be "
                                "specified when focusing texture banks.");
  }
  Poco::File dgf(dgFile);
  if (!dgf.exists()) {
    throw std::invalid_argument(
        "The detector grouping file coult not be found: " + dgFile);
  }

  inputChecksBeforeFocus();
}

void EnggDiffractionPresenter::inputChecksBanks(
    const std::vector<bool> &banks) {
  if (0 == banks.size()) {
    const std::string msg =
        "Error in specification of banks found when starting the "
        "focusing process. Cannot continue.";
    g_log.error() << msg << std::endl;
    throw std::invalid_argument(msg);
  }
  if (banks.end() == std::find(banks.begin(), banks.end(), true)) {
    const std::string msg =
        "EnggDiffraction GUI: not focusing, as none of the banks "
        "have been selected. You probably forgot to select at least one.";
    g_log.warning() << msg << std::endl;
    throw std::invalid_argument(msg);
  }
}

/**
* Performs several checks on the current focusing inputs and
* settings. This should be done before starting any focus work. The
* message return should be shown to the user as a visible message
* (pop-up, error log, etc.)
*
* @throws std::invalid_argument with an informative message.
*/
void EnggDiffractionPresenter::inputChecksBeforeFocus() {
  EnggDiffCalibSettings cs = m_view->currentCalibSettings();
  const std::string pixelCalib = cs.m_pixelCalibFilename;
  if (pixelCalib.empty()) {
    throw std::invalid_argument(
        "You need to set a pixel (full) calibration in settings.");
  }
}

/**
* Builds the names of the output focused files (one per bank), given
* the sample run number and which banks should be focused.
*
* @param runNo number of the run for which we want a focused output
* file name
*
* @param banks for every bank, (true/false) to consider it or not for
* the focusing
*
* @return filenames (without the full path)
*/
std::vector<std::string>
EnggDiffractionPresenter::outputFocusFilenames(const std::string &runNo,
                                               const std::vector<bool> &banks) {
  const std::string instStr = m_view->currentInstrument();
  std::vector<std::string> res;
  res.reserve(banks.size());
  std::string prefix = instStr + "_" + runNo + "_focused_bank_";
  for (size_t b = 1; b <= banks.size(); b++) {
    res.emplace_back(prefix + boost::lexical_cast<std::string>(b) + ".nxs");
  }
  return res;
}

std::string
EnggDiffractionPresenter::outputFocusCroppedFilename(const std::string &runNo) {
  const std::string instStr = m_view->currentInstrument();

  return instStr + "_" + runNo + "_focused_cropped.nxs";
}

std::vector<std::string> EnggDiffractionPresenter::sumOfFilesLoadVec() {
  std::vector<std::string> multi_RunNo;

  if (g_sumOfFilesFocus == "basic")
    multi_RunNo = isValidMultiRunNumber(m_view->focusingRunNo());
  else if (g_sumOfFilesFocus == "cropped")
    multi_RunNo = isValidMultiRunNumber(m_view->focusingCroppedRunNo());
  else if (g_sumOfFilesFocus == "texture")
    multi_RunNo = isValidMultiRunNumber(m_view->focusingTextureRunNo());

  return multi_RunNo;
}

std::vector<std::string> EnggDiffractionPresenter::outputFocusTextureFilenames(
    const std::string &runNo, const std::vector<size_t> &bankIDs) {
  const std::string instStr = m_view->currentInstrument();

  std::vector<std::string> res;
  res.reserve(bankIDs.size());
  std::string prefix = instStr + "_" + runNo + "_focused_texture_bank_";
  for (size_t b = 0; b < bankIDs.size(); b++) {
    res.emplace_back(prefix + boost::lexical_cast<std::string>(bankIDs[b]) +
                     ".nxs");
  }

  return res;
}

/**
* Start the focusing algorithm(s) without blocking the GUI. This is
* based on Qt connect / signals-slots so that it goes in sync with
* the Qt event loop. For that reason this class needs to be a
* Q_OBJECT.
*
* @param dir directory (full path) for the focused output files
* @param multi_RunNo input vector of run number
* @param banks instrument bank to focus
* @param specNos list of spectra (as usual csv list of spectra in Mantid)
* @param dgFile detector grouping file name
*/
void EnggDiffractionPresenter::startAsyncFocusWorker(
    const std::string &dir, const std::vector<std::string> &multi_RunNo,
    const std::vector<bool> &banks, const std::string &dgFile,
    const std::string &specNos) {

  delete m_workerThread;
  m_workerThread = new QThread(this);
  EnggDiffWorker *worker =
      new EnggDiffWorker(this, dir, multi_RunNo, banks, dgFile, specNos);
  worker->moveToThread(m_workerThread);
  connect(m_workerThread, SIGNAL(started()), worker, SLOT(focus()));
  connect(worker, SIGNAL(finished()), this, SLOT(focusingFinished()));
  // early delete of thread and worker
  connect(m_workerThread, SIGNAL(finished()), m_workerThread,
          SLOT(deleteLater()), Qt::DirectConnection);
  connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
  m_workerThread->start();
}

/**
* Produce a new focused output file. This is what threads/workers
* should use to run the calculations required to process a 'focus'
* push or similar from the user.
*
* @param dir directory (full path) for the output focused files
* @param runNo input run number
*
* @param specNos list of spectra to use when focusing. Not empty
* implies focusing in cropped mode.
*
* @param dgFile detector grouping file to define banks (texture). Not
* empty implies focusing in texture mode.
*
* @param banks for every bank, (true/false) to consider it or not for
* the focusing
*/
void EnggDiffractionPresenter::doFocusRun(const std::string &dir,
                                          const std::string &runNo,
                                          const std::vector<bool> &banks,
                                          const std::string &specNos,
                                          const std::string &dgFile) {

  if (!g_abortThread) {

    // to track last valid run
    g_lastValidRun = runNo;

    g_log.notice() << "Generating new focusing workspace(s) and file(s) into "
                      "this directory: "
                   << dir << std::endl;

    // TODO: this is almost 100% common with doNewCalibrate() - refactor
    EnggDiffCalibSettings cs = m_view->currentCalibSettings();
    Mantid::Kernel::ConfigServiceImpl &conf =
        Mantid::Kernel::ConfigService::Instance();
    const std::vector<std::string> tmpDirs = conf.getDataSearchDirs();
    // in principle, the run files will be found from 'DirRaw', and the
    // pre-calculated Vanadium corrections from 'DirCalib'
    if (!cs.m_inputDirCalib.empty() &&
        Poco::File(cs.m_inputDirCalib).exists()) {
      conf.appendDataSearchDir(cs.m_inputDirCalib);
    }
    if (!cs.m_inputDirRaw.empty() && Poco::File(cs.m_inputDirRaw).exists()) {
      conf.appendDataSearchDir(cs.m_inputDirRaw);
    }

    // Prepare special inputs for "texture" focusing
    std::vector<size_t> bankIDs;
    std::vector<std::string> effectiveFilenames;
    std::vector<std::string> specs;
    if (!specNos.empty()) {
      // Cropped focusing
      // just to iterate once, but there's no real bank here
      bankIDs.push_back(0);
      specs.push_back(specNos); // one spectrum IDs list given by the user
      effectiveFilenames.push_back(outputFocusCroppedFilename(runNo));
    } else {
      if (dgFile.empty()) {
        // Basic/normal focusing
        for (size_t bidx = 0; bidx < banks.size(); bidx++) {
          if (banks[bidx]) {
            bankIDs.push_back(bidx + 1);
            specs.emplace_back("");
            effectiveFilenames = outputFocusFilenames(runNo, banks);
          }
        }
      } else {
        // texture focusing
        try {
          loadDetectorGroupingCSV(dgFile, bankIDs, specs);
        } catch (std::runtime_error &re) {
          g_log.error() << "Error loading detector grouping file: " + dgFile +
                               ". Detailed error: " + re.what()
                        << std::endl;
          bankIDs.clear();
          specs.clear();
        }
        effectiveFilenames = outputFocusTextureFilenames(runNo, bankIDs);
      }
    }

    // focus all requested banks
    for (size_t idx = 0; idx < bankIDs.size(); idx++) {

      Poco::Path fpath(dir);
      const std::string fullFilename =
          fpath.append(effectiveFilenames[idx]).toString();
      g_log.notice() << "Generating new focused file (bank " +
                            boost::lexical_cast<std::string>(bankIDs[idx]) +
                            ") for run " + runNo + " into: "
                     << effectiveFilenames[idx] << std::endl;
      try {
        m_focusFinishedOK = false;
        doFocusing(cs, fullFilename, runNo, bankIDs[idx], specs[idx], dgFile);
        m_focusFinishedOK = true;
      } catch (std::runtime_error &) {
        g_log.error()
            << "The focusing calculations failed. One of the algorithms"
               "did not execute correctly. See log messages for details."
            << std::endl;
      } catch (std::invalid_argument &ia) {
        g_log.error() << "The focusing failed. Some input properties "
                         "were not valid. "
                         "See log messages for details. Error: "
                      << ia.what() << std::endl;
      }
    }

    // restore initial data search paths
    conf.setDataSearchDirs(tmpDirs);
  }
}

void EnggDiffractionPresenter::loadDetectorGroupingCSV(
    const std::string &dgFile, std::vector<size_t> &bankIDs,
    std::vector<std::string> &specs) {
  const char commentChar = '#';
  const std::string delim = ",";

  std::ifstream file(dgFile.c_str());
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file.");
  }

  bankIDs.clear();
  specs.clear();
  std::string line;
  for (size_t li = 1; getline(file, line); li++) {
    if (line.empty() || commentChar == line[0])
      continue;

    auto delimPos = line.find_first_of(delim);
    if (std::string::npos == delimPos) {
      throw std::runtime_error(
          "In file '" + dgFile + "', wrong format in line: " +
          boost::lexical_cast<std::string>(li) +
          " which does not containe any delimiters (comma, etc.)");
    }

    try {
      const std::string bstr = line.substr(0, delimPos);
      const std::string spec = line.substr(delimPos + 1, std::string::npos);

      if (bstr.empty()) {
        throw std::runtime_error(
            "In file '" + dgFile + "', wrong format in line: " +
            boost::lexical_cast<std::string>(li) + ", the bank ID is empty!");
      }
      if (spec.empty()) {
        throw std::runtime_error("In file '" + dgFile +
                                 "', wrong format in line: " +
                                 boost::lexical_cast<std::string>(li) +
                                 ", the list of spectrum IDs is empty!");
      }

      size_t bankID = boost::lexical_cast<size_t>(bstr);
      bankIDs.push_back(bankID);
      specs.push_back(spec);
    } catch (std::runtime_error &re) {
      throw std::runtime_error(
          "In file '" + dgFile +
          "', issue found when trying to interpret line: " +
          boost::lexical_cast<std::string>(li) + ". Error description: " +
          re.what());
    }
  }
}

/**
* Method (Qt slot) to call when the focusing work has finished,
* possibly from a separate thread but sometimes not (as in this
* presenter class' test).
*/
void EnggDiffractionPresenter::focusingFinished() {
  if (!m_view)
    return;

  m_view->enableCalibrateAndFocusActions(true);
  if (!m_focusFinishedOK) {
    g_log.warning() << "The cablibration did not finish correctly."
                    << std::endl;
  } else {
    g_log.notice() << "Focusing finished - focused run(s) are ready."
                   << std::endl;
  }
  if (m_workerThread) {
    delete m_workerThread;
    m_workerThread = NULL;
  }

  // display warning and information to the users regarding Stop Focus
  if (g_abortThread) {
    // will get the last number in the list
    std::string last_RunNo = isValidRunNumber(m_view->focusingRunNo());
    double lastRun = boost::lexical_cast<double>(last_RunNo);
    double lastValid = boost::lexical_cast<double>(g_lastValidRun);

    if (lastRun != lastValid) {
      g_log.warning()
          << "Focussing process has been stopped, last successful "
             "run number: "
          << g_lastValidRun
          << " , total number of focus run that could not be processed: "
          << (lastRun - lastValid) << std::endl;
    }
  }
}

/**
* Focuses a run, produces a focused workspace, and saves it into a
* file.
*
* @param cs user settings for calibration (this does not calibrate but
* uses calibration input files such as vanadium runs
*
* @param fullFilename full path for the output (focused) filename
*
* @param runNo input run to focus
*
* @param bank instrument bank number to focus
*
* @param specNos string specifying a list of spectra (for "cropped"
* focusing or "texture" focusing), only considered if not empty
*
* @param dgFile detector grouping file name. If not empty implies
* texture focusing
*/
void EnggDiffractionPresenter::doFocusing(const EnggDiffCalibSettings &cs,
                                          const std::string &fullFilename,
                                          const std::string &runNo, size_t bank,
                                          const std::string &specNos,
                                          const std::string &dgFile) {
  ITableWorkspace_sptr vanIntegWS;
  MatrixWorkspace_sptr vanCurvesWS;
  MatrixWorkspace_sptr inWS;

  const std::string vanNo = m_view->currentVanadiumNo();
  loadOrCalcVanadiumWorkspaces(vanNo, cs.m_inputDirCalib, vanIntegWS,
                               vanCurvesWS, cs.m_forceRecalcOverwrite);

  const std::string inWSName = "engggui_focusing_input_ws";
  const std::string instStr = m_view->currentInstrument();
  std::vector<std::string> multi_RunNo = sumOfFilesLoadVec();
  std::string loadInput = "";

  for (size_t i = 0; i < multi_RunNo.size(); i++) {
    // if last run number in list
    if (i + 1 == multi_RunNo.size())
      loadInput += instStr + multi_RunNo[i];
    else
      loadInput += instStr + multi_RunNo[i] + '+';
  }

  // if its not empty the global variable is set for sumOfFiles
  if (!g_sumOfFilesFocus.empty()) {

    try {
      auto load =
          Mantid::API::AlgorithmManager::Instance().createUnmanaged("Load");
      load->initialize();
      load->setPropertyValue("Filename", loadInput);

      load->setPropertyValue("OutputWorkspace", inWSName);
      load->execute();

      AnalysisDataServiceImpl &ADS =
          Mantid::API::AnalysisDataService::Instance();
      inWS = ADS.retrieveWS<MatrixWorkspace>(inWSName);
    } catch (std::runtime_error &re) {
      g_log.error()
          << "Error while loading files provided. "
             "Could not run the algorithm Load succesfully for the focus "
             "(run number provided. Error description:"
             "Please check also the previous log messages for details." +
                 static_cast<std::string>(re.what());
      throw;
    }

    if (multi_RunNo.size() == 1) {
      g_log.notice() << "Only single file has been listed, the Sum Of Files"
                        "cannot not be processed"
                     << std::endl;
    } else {
      g_log.notice()
          << "Load alogirthm has successfully merged the files provided"
          << std::endl;
    }

  } else {
    try {
      auto load =
          Mantid::API::AlgorithmManager::Instance().createUnmanaged("Load");
      load->initialize();
      load->setPropertyValue("Filename", instStr + runNo);
      load->setPropertyValue("OutputWorkspace", inWSName);
      load->execute();

      AnalysisDataServiceImpl &ADS =
          Mantid::API::AnalysisDataService::Instance();
      inWS = ADS.retrieveWS<MatrixWorkspace>(inWSName);
    } catch (std::runtime_error &re) {
      g_log.error()
          << "Error while loading sample data for focusing. "
             "Could not run the algorithm Load succesfully for the focusing "
             "sample (run number: " +
                 runNo + "). Error description: " + re.what() +
                 " Please check also the previous log messages for details.";
      throw;
    }
  }

  std::string outWSName;
  std::string specNumsOpenGenie;
  if (!dgFile.empty()) {
    // doing focus "texture"
    outWSName = "engggui_focusing_output_ws_texture_bank_" +
                boost::lexical_cast<std::string>(bank);
    specNumsOpenGenie = specNos;
  } else if (specNos.empty()) {
    // doing focus "normal" / by banks
    outWSName = "engggui_focusing_output_ws_bank_" +
                boost::lexical_cast<std::string>(bank);

    // specnum for opengenie according to bank number
    if (boost::lexical_cast<std::string>(bank) == "1") {
      specNumsOpenGenie = "1 - 1200";
    } else if (boost::lexical_cast<std::string>(bank) == "2") {
      specNumsOpenGenie = "1201 - 1400";
    }

  } else {
    // doing focus "cropped"
    outWSName = "engggui_focusing_output_ws_cropped";
    specNumsOpenGenie = specNos;
  }
  try {
    auto alg =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged("EnggFocus");
    alg->initialize();
    alg->setProperty("InputWorkspace", inWSName);
    alg->setProperty("OutputWorkspace", outWSName);
    alg->setProperty("VanIntegrationWorkspace", vanIntegWS);
    alg->setProperty("VanCurvesWorkspace", vanCurvesWS);
    // cropped / normal focusing
    if (specNos.empty()) {
      alg->setPropertyValue("Bank", boost::lexical_cast<std::string>(bank));
    } else {
      alg->setPropertyValue("SpectrumNumbers", specNos);
    }
    // TODO: use detector positions (from calibrate full) when available
    // alg->setProperty(DetectorPositions, TableWorkspace)
    alg->execute();
    g_plottingCounter++;
    plotFocusedWorkspace(outWSName);

  } catch (std::runtime_error &re) {
    g_log.error() << "Error in calibration. ",
        "Could not run the algorithm EnggCalibrate succesfully for bank " +
            boost::lexical_cast<std::string>(bank) + ". Error description: " +
            re.what() + " Please check also the log messages for details.";
    throw;
  }
  g_log.notice() << "Produced focused workspace: " << outWSName << std::endl;

  try {
    g_log.debug() << "Going to save focused output into nexus file: "
                  << fullFilename << std::endl;
    auto alg =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged("SaveNexus");
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", outWSName);
    alg->setPropertyValue("Filename", fullFilename);
    alg->execute();
  } catch (std::runtime_error &re) {
    g_log.error() << "Error in calibration. ",
        "Could not run the algorithm EnggCalibrate succesfully for bank " +
            boost::lexical_cast<std::string>(bank) + ". Error description: " +
            re.what() + " Please check also the log messages for details.";
    throw;
  }
  g_log.notice() << "Saved focused workspace as file: " << fullFilename
                 << std::endl;

  bool saveOutputFiles = m_view->saveOutputFiles();

  if (saveOutputFiles) {
    try {
      saveFocusedXYE(outWSName, boost::lexical_cast<std::string>(bank), runNo);
      saveGSS(outWSName, boost::lexical_cast<std::string>(bank), runNo);
      saveOpenGenie(outWSName, specNumsOpenGenie,
                    boost::lexical_cast<std::string>(bank), runNo);
    } catch (std::runtime_error &re) {
      g_log.error() << "Error saving focused data. ",
          "There was an error while saving focused data. "
          "Error Description: " +
              std::string(re.what()) +
              "Please check log messages for more details.";
      throw;
    }
  }
}

/**
* Produce the two workspaces that are required to apply Vanadium
* corrections. Try to load them if precalculated results are
* available from files, otherwise load the source Vanadium run
* workspace and do the calculations.
*
* @param vanNo Vanadium run number
*
* @param inputDirCalib The 'calibration files' input directory given
* in settings
*
* @param vanIntegWS workspace where to create/load the Vanadium
* spectra integration
*
* @param vanCurvesWS workspace where to create/load the Vanadium
* aggregated per-bank curve
*
* @param forceRecalc whether to calculate Vanadium corrections even
* if the files of pre-calculated results are found
*/
void EnggDiffractionPresenter::loadOrCalcVanadiumWorkspaces(
    const std::string &vanNo, const std::string &inputDirCalib,
    ITableWorkspace_sptr &vanIntegWS, MatrixWorkspace_sptr &vanCurvesWS,
    bool forceRecalc) {
  bool foundPrecalc = false;

  std::string preIntegFilename, preCurvesFilename;
  findPrecalcVanadiumCorrFilenames(vanNo, inputDirCalib, preIntegFilename,
                                   preCurvesFilename, foundPrecalc);

  // if pre caluclated not found ..
  if (forceRecalc || !foundPrecalc) {
    g_log.notice()
        << "Calculating Vanadium corrections. This may take a few seconds..."
        << std::endl;
    try {
      calcVanadiumWorkspaces(vanNo, vanIntegWS, vanCurvesWS);
    } catch (std::invalid_argument &ia) {
      g_log.error() << "Failed to calculate Vanadium corrections. "
                       "There was an error in the execution of the algorithms "
                       "required to calculate Vanadium corrections. Some "
                       "properties passed to the algorithms were invalid. "
                       "This is possibly because some of the settings are not "
                       "consistent. Please check the log messages for "
                       "details. Details: " +
                           std::string(ia.what())
                    << std::endl;
      throw;
    } catch (std::runtime_error &re) {
      g_log.error() << "Failed to calculate Vanadium corrections. "
                       "There was an error while executing one of the "
                       "algorithms used to perform Vanadium corrections. "
                       "There was no obvious error in the input properties "
                       "but the algorithm failed. Please check the log "
                       "messages for details." +
                           std::string(re.what())
                    << std::endl;
      throw;
    }
  } else {
    g_log.notice() << "Found precalculated Vanadium correction features for "
                      "Vanadium run "
                   << vanNo << ". Re-using these files: " << preIntegFilename
                   << ", and " << preCurvesFilename << std::endl;
    try {
      loadVanadiumPrecalcWorkspaces(preIntegFilename, preCurvesFilename,
                                    vanIntegWS, vanCurvesWS, vanNo);
    } catch (std::invalid_argument &ia) {
      g_log.error() << "Error while loading precalculated Vanadium corrections",
          "The files with precalculated Vanadium corection features (spectra "
          "integration and per-bank curves) were found (with names '" +
              preIntegFilename + "' and '" + preCurvesFilename +
              "', respectively, but there was a problem with the inputs to "
              "the "
              "load algorithms to load them: " +
              std::string(ia.what());
      throw;
    } catch (std::runtime_error &re) {
      g_log.error() << "Error while loading precalculated Vanadium corrections",
          "The files with precalculated Vanadium corection features (spectra "
          "integration and per-bank curves) were found (with names '" +
              preIntegFilename + "' and '" + preCurvesFilename +
              "', respectively, but there was a problem while loading them. "
              "Please check the log messages for details. You might want to "
              "delete those files or force recalculations (in settings). "
              "Error "
              "details: " +
              std::string(re.what());
      throw;
    }
  }
}

/**
* builds the expected names of the precalculated Vanadium correction
* files and tells if both files are found, similar to:
* ENGINX_precalculated_vanadium_run000236516_integration.nxs
* ENGINX_precalculated_vanadium_run00236516_bank_curves.nxs
*
* @param vanNo Vanadium run number
* @param inputDirCalib calibration directory in settings
* @param preIntegFilename if not found on disk, the string is set as empty
* @param preCurvesFilename if not found on disk, the string is set as empty
* @param found true if both files are found and (re-)usable
*/
void EnggDiffractionPresenter::findPrecalcVanadiumCorrFilenames(
    const std::string &vanNo, const std::string &inputDirCalib,
    std::string &preIntegFilename, std::string &preCurvesFilename,
    bool &found) {
  found = false;

  const std::string runNo = std::string(2, '0').append(vanNo);

  preIntegFilename =
      g_enginxStr + "_precalculated_vanadium_run" + runNo + "_integration.nxs";

  preCurvesFilename =
      g_enginxStr + "_precalculated_vanadium_run" + runNo + "_bank_curves.nxs";

  Poco::Path pathInteg(inputDirCalib);
  pathInteg.append(preIntegFilename);

  Poco::Path pathCurves(inputDirCalib);
  pathCurves.append(preCurvesFilename);

  if (Poco::File(pathInteg).exists() && Poco::File(pathCurves).exists()) {
    preIntegFilename = pathInteg.toString();
    preCurvesFilename = pathCurves.toString();
    found = true;
  }
}

/**
* Load precalculated results from Vanadium corrections previously
* calculated.
*
* @param preIntegFilename filename (can be full path) where the
* vanadium spectra integration table should be loaded from
*
* @param preCurvesFilename filename (can be full path) where the
* vanadium per-bank curves should be loaded from
*
* @param vanIntegWS output (matrix) workspace loaded from the
* precalculated Vanadium correction file, with the integration
* resutls
*
* @param vanCurvesWS output (matrix) workspace loaded from the
* precalculated Vanadium correction file, with the per-bank curves
*
* @param vanNo the Vanadium run number used for the openGenieAscii
* output label
*/
void EnggDiffractionPresenter::loadVanadiumPrecalcWorkspaces(
    const std::string &preIntegFilename, const std::string &preCurvesFilename,
    ITableWorkspace_sptr &vanIntegWS, MatrixWorkspace_sptr &vanCurvesWS,
    const std::string &vanNo) {
  AnalysisDataServiceImpl &ADS = Mantid::API::AnalysisDataService::Instance();

  auto alg =
      Mantid::API::AlgorithmManager::Instance().createUnmanaged("LoadNexus");
  alg->initialize();
  alg->setPropertyValue("Filename", preIntegFilename);
  std::string integWSName = g_vanIntegrationWSName;
  alg->setPropertyValue("OutputWorkspace", integWSName);
  alg->execute();
  // alg->getProperty("OutputWorkspace");
  vanIntegWS = ADS.retrieveWS<ITableWorkspace>(integWSName);

  auto algCurves =
      Mantid::API::AlgorithmManager::Instance().createUnmanaged("LoadNexus");
  algCurves->initialize();
  algCurves->setPropertyValue("Filename", preCurvesFilename);
  std::string curvesWSName = "engggui_vanadium_curves_ws";
  algCurves->setPropertyValue("OutputWorkspace", curvesWSName);
  algCurves->execute();
  // algCurves->getProperty("OutputWorkspace");
  vanCurvesWS = ADS.retrieveWS<MatrixWorkspace>(curvesWSName);

  saveOpenGenie(curvesWSName, "1-1200", "North", vanNo);
  saveOpenGenie(curvesWSName, "1201-2400", "South", vanNo);
}

/**
* Calculate vanadium corrections (in principle only for when
* pre-calculated results are not available). This is expensive.
*
* @param vanNo Vanadium run number
*
* @param vanIntegWS where to keep the Vanadium run spectra
* integration values
*
* @param vanCurvesWS workspace where to keep the per-bank vanadium
* curves
*/
void EnggDiffractionPresenter::calcVanadiumWorkspaces(
    const std::string &vanNo, ITableWorkspace_sptr &vanIntegWS,
    MatrixWorkspace_sptr &vanCurvesWS) {

  auto load = Mantid::API::AlgorithmManager::Instance().createUnmanaged("Load");
  load->initialize();
  load->setPropertyValue("Filename",
                         vanNo); // TODO more specific build Vanadium filename
  std::string vanWSName = "engggui_vanadium_ws";
  load->setPropertyValue("OutputWorkspace", vanWSName);
  load->execute();
  AnalysisDataServiceImpl &ADS = Mantid::API::AnalysisDataService::Instance();
  MatrixWorkspace_sptr vanWS = ADS.retrieveWS<MatrixWorkspace>(vanWSName);
  // TODO?: maybe use setChild() and then
  // load->getProperty("OutputWorkspace");

  auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
      "EnggVanadiumCorrections");
  alg->initialize();
  alg->setProperty("VanadiumWorkspace", vanWS);
  std::string integName = g_vanIntegrationWSName;
  alg->setPropertyValue("OutIntegrationWorkspace", integName);
  std::string curvesName = "engggui_van_curves_ws";
  alg->setPropertyValue("OutCurvesWorkspace", curvesName);
  alg->execute();

  ADS.remove(vanWSName);

  vanIntegWS = ADS.retrieveWS<ITableWorkspace>(integName);
  vanCurvesWS = ADS.retrieveWS<MatrixWorkspace>(curvesName);
}

/**
* Loads a workspace to pre-process (rebin, etc.). The workspace
* loaded can be a MatrixWorkspace or a group of MatrixWorkspace (for
* multiperiod data).
*
* @param runNo run number to search for the file with 'Load'.
*/
Workspace_sptr
EnggDiffractionPresenter::loadToPreproc(const std::string runNo) {
  const std::string instStr = m_view->currentInstrument();
  Workspace_sptr inWS;

  // this is required when file is selected via browse button
  const auto MultiRunNoDir = m_view->currentPreprocRunNo();
  const auto runNoDir = MultiRunNoDir[0];

  try {
    auto load =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged("Load");
    load->initialize();
    if (Poco::File(runNoDir).exists()) {
      load->setPropertyValue("Filename", runNoDir);
    } else {
      load->setPropertyValue("Filename", instStr + runNo);
    }
    const std::string inWSName = "engggui_preproc_input_ws";
    load->setPropertyValue("OutputWorkspace", inWSName);

    load->execute();

    auto &ADS = Mantid::API::AnalysisDataService::Instance();
    inWS = ADS.retrieveWS<Workspace>(inWSName);
  } catch (std::runtime_error &re) {
    g_log.error()
        << "Error while loading run data to pre-process. "
           "Could not run the algorithm Load succesfully for the run "
           "number: " +
               runNo + "). Error description: " + re.what() +
               " Please check also the previous log messages for details.";
    throw;
  }

  return inWS;
}

void EnggDiffractionPresenter::doRebinningTime(const std::string &runNo,
                                               double bin,
                                               const std::string &outWSName) {

  // Runs something like:
  // Rebin(InputWorkspace='ws_runNo', outputWorkspace=outWSName,Params=bin)

  m_rebinningFinishedOK = false;
  const Workspace_sptr inWS = loadToPreproc(runNo);
  if (!inWS)
    g_log.error() << "Error: could not load the input workspace for rebinning."
                  << std::endl;

  const std::string rebinName = "Rebin";
  try {
    auto alg =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged(rebinName);
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", inWS->name());
    alg->setPropertyValue("OutputWorkspace", outWSName);
    alg->setProperty("Params", boost::lexical_cast<std::string>(bin));

    alg->execute();
  } catch (std::invalid_argument &ia) {
    g_log.error() << "Error when rebinning with a regular bin width in time. "
                     "There was an error in the inputs to the algorithm " +
                         rebinName + ". Error description: " + ia.what() + "."
                  << std::endl;
    return;
  } catch (std::runtime_error &re) {
    g_log.error() << "Error when rebinning with a regular bin width in time. "
                     "Coult not run the algorithm " +
                         rebinName + " successfully. Error description: " +
                         re.what() + "."
                  << std::endl;
    return;
  }

  // succesful completion
  m_rebinningFinishedOK = true;
}

void EnggDiffractionPresenter::inputChecksBeforeRebin(
    const std::string &runNo) {
  if (runNo.empty()) {
    throw std::invalid_argument("The run to pre-process" + g_runNumberErrorStr);
  }
}

void EnggDiffractionPresenter::inputChecksBeforeRebinTime(
    const std::string &runNo, double bin) {
  inputChecksBeforeRebin(runNo);

  if (bin <= 0) {
    throw std::invalid_argument("The bin width must be strictly positive");
  }
}

/**
* Starts the Rebin algorithm(s) without blocking the GUI. This is
* based on Qt connect / signals-slots so that it goes in sync with
* the Qt event loop. For that reason this class needs to be a
* Q_OBJECT.
*
* @param runNo run number(s)
* @param bin bin width parameter for Rebin
* @param outWSName name for the output workspace produced here
*/
void EnggDiffractionPresenter::startAsyncRebinningTimeWorker(
    const std::string &runNo, double bin, const std::string &outWSName) {

  delete m_workerThread;
  m_workerThread = new QThread(this);
  EnggDiffWorker *worker = new EnggDiffWorker(this, runNo, bin, outWSName);
  worker->moveToThread(m_workerThread);

  connect(m_workerThread, SIGNAL(started()), worker, SLOT(rebinTime()));
  connect(worker, SIGNAL(finished()), this, SLOT(rebinningFinished()));
  // early delete of thread and worker
  connect(m_workerThread, SIGNAL(finished()), m_workerThread,
          SLOT(deleteLater()), Qt::DirectConnection);
  connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
  m_workerThread->start();
}

void EnggDiffractionPresenter::inputChecksBeforeRebinPulses(
    const std::string &runNo, size_t nperiods, double timeStep) {
  inputChecksBeforeRebin(runNo);

  if (0 == nperiods) {
    throw std::invalid_argument("The number of periods has been set to 0 so "
                                "none of the periods will be processed");
  }

  if (timeStep <= 0) {
    throw std::invalid_argument(
        "The bin or step for the time axis must be strictly positive");
  }
}

void EnggDiffractionPresenter::doRebinningPulses(const std::string &runNo,
                                                 size_t nperiods,
                                                 double timeStep,
                                                 const std::string &outWSName) {
  // TOOD: not clear what will be the role of this parameter for now
  UNUSED_ARG(nperiods);

  // Runs something like:
  // RebinByPulseTimes(InputWorkspace='ws_runNo', outputWorkspace=outWSName,
  //                   Params=timeStepstep)

  m_rebinningFinishedOK = false;
  const Workspace_sptr inWS = loadToPreproc(runNo);
  if (!inWS)
    g_log.error() << "Error: could not load the input workspace for rebinning."
                  << std::endl;

  const std::string rebinName = "RebinByPulseTimes";
  try {
    auto alg =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged(rebinName);
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", inWS->name());
    alg->setPropertyValue("OutputWorkspace", outWSName);
    alg->setProperty("Params", boost::lexical_cast<std::string>(timeStep));

    alg->execute();
  } catch (std::invalid_argument &ia) {
    g_log.error() << "Error when rebinning by pulse times. "
                     "There was an error in the inputs to the algorithm " +
                         rebinName + ". Error description: " + ia.what() + "."
                  << std::endl;
    return;
  } catch (std::runtime_error &re) {
    g_log.error() << "Error when rebinning by pulse times. "
                     "Coult not run the algorithm " +
                         rebinName + " successfully. Error description: " +
                         re.what() + "."
                  << std::endl;
    return;
  }

  // successful execution
  m_rebinningFinishedOK = true;
}

/**
* Starts the Rebin (by pulses) algorithm(s) without blocking the
* GUI. This is based on Qt connect / signals-slots so that it goes in
* sync with the Qt event loop. For that reason this class needs to be
* a Q_OBJECT.
*
* @param runNo run number(s)
* @param nperiods max number of periods to process
* @param timeStep bin width parameter for the x (time) axis
* @param outWSName name for the output workspace produced here
*/
void EnggDiffractionPresenter::startAsyncRebinningPulsesWorker(
    const std::string &runNo, size_t nperiods, double timeStep,
    const std::string &outWSName) {

  delete m_workerThread;
  m_workerThread = new QThread(this);
  EnggDiffWorker *worker =
      new EnggDiffWorker(this, runNo, nperiods, timeStep, outWSName);
  worker->moveToThread(m_workerThread);

  connect(m_workerThread, SIGNAL(started()), worker, SLOT(rebinPulses()));
  connect(worker, SIGNAL(finished()), this, SLOT(rebinningFinished()));
  // early delete of thread and worker
  connect(m_workerThread, SIGNAL(finished()), m_workerThread,
          SLOT(deleteLater()), Qt::DirectConnection);
  connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
  m_workerThread->start();
}

/**
* Method (Qt slot) to call when the rebin work has finished,
* possibly from a separate thread but sometimes not (as in this
* presenter class' test).
*/
void EnggDiffractionPresenter::rebinningFinished() {
  if (!m_view)
    return;

  m_view->enableCalibrateAndFocusActions(true);
  if (!m_rebinningFinishedOK) {
    g_log.warning()
        << "The pre-processing (re-binning) did not finish correctly."
        << std::endl;
  } else {
    g_log.notice() << "Pre-processing (re-binning) finished - the output "
                      "workspace is ready."
                   << std::endl;
  }
  if (m_workerThread) {
    delete m_workerThread;
    m_workerThread = NULL;
  }
}

/**
* Checks the plot type selected and applies the appropriate
* python function to apply during first bank and second bank
*
* @param outWSName title of the focused workspace
*/
void EnggDiffractionPresenter::plotFocusedWorkspace(std::string outWSName) {
  const bool plotFocusedWS = m_view->focusedOutWorkspace();
  enum PlotMode { REPLACING = 0, WATERFALL = 1, MULTIPLE = 2 };

  int plotType = m_view->currentPlotType();

  if (plotFocusedWS) {
    if (plotType == PlotMode::REPLACING) {
      if (g_plottingCounter == 1)
        m_view->plotFocusedSpectrum(outWSName);
      else
        m_view->plotReplacingWindow(outWSName);

    } else if (plotType == PlotMode::WATERFALL) {
      if (g_plottingCounter == 1)
        m_view->plotFocusedSpectrum(outWSName);
      else
        m_view->plotWaterfallSpectrum(outWSName);

    } else if (plotType == PlotMode::MULTIPLE) {
      m_view->plotFocusedSpectrum(outWSName);
    }
  }
}

/**
* Convert the generated output files and saves them in
* FocusedXYE format
*
* @param inputWorkspace title of the focused workspace
* @param bank the number of the bank as a string
* @param runNo the run number as a string
*/
void EnggDiffractionPresenter::saveFocusedXYE(const std::string inputWorkspace,
                                              std::string bank,
                                              std::string runNo) {

  // Generates the file name in the appropriate format
  std::string fullFilename =
      outFileNameFactory(inputWorkspace, runNo, bank, ".dat");

  // Creates appropriate directory
  Poco::Path saveDir = outFilesDir("Focus");

  // append the full file name in the end
  saveDir.append(fullFilename);

  try {
    g_log.debug() << "Going to save focused output into OpenGenie file: "
                  << fullFilename << std::endl;
    auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
        "SaveFocusedXYE");
    alg->initialize();
    alg->setProperty("InputWorkspace", inputWorkspace);
    std::string filename(saveDir.toString());
    alg->setPropertyValue("Filename", filename);
    alg->setProperty("SplitFiles", false);
    alg->setPropertyValue("StartAtBankNumber", bank);
    alg->execute();
  } catch (std::runtime_error &re) {
    g_log.error() << "Error in saving FocusedXYE format file. ",
        "Could not run the algorithm SaveFocusXYE succesfully for "
        "workspace " +
            inputWorkspace + ". Error description: " + re.what() +
            " Please check also the log messages for details.";
    throw;
  }
  g_log.notice() << "Saved focused workspace as file: " << saveDir.toString()
                 << std::endl;
}

/**
* Convert the generated output files and saves them in
* GSS format
*
* @param inputWorkspace title of the focused workspace
* @param bank the number of the bank as a string
* @param runNo the run number as a string
*/
void EnggDiffractionPresenter::saveGSS(const std::string inputWorkspace,
                                       std::string bank, std::string runNo) {

  // Generates the file name in the appropriate format
  std::string fullFilename =
      outFileNameFactory(inputWorkspace, runNo, bank, ".gss");

  // Creates appropriate directory
  Poco::Path saveDir = outFilesDir("Focus");

  // append the full file name in the end
  saveDir.append(fullFilename);

  try {
    g_log.debug() << "Going to save focused output into OpenGenie file: "
                  << fullFilename << std::endl;
    auto alg =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged("SaveGSS");
    alg->initialize();
    alg->setProperty("InputWorkspace", inputWorkspace);
    std::string filename(saveDir.toString());
    alg->setPropertyValue("Filename", filename);
    alg->setProperty("SplitFiles", false);
    alg->setPropertyValue("Bank", bank);
    alg->execute();
  } catch (std::runtime_error &re) {
    g_log.error() << "Error in saving GSS format file. ",
        "Could not run the algorithm saveGSS succesfully for "
        "workspace " +
            inputWorkspace + ". Error description: " + re.what() +
            " Please check also the log messages for details.";
    throw;
  }
  g_log.notice() << "Saved focused workspace as file: " << saveDir.toString()
                 << std::endl;
}

/**
* Convert the generated output files and saves them in
* OpenGenie format
*
* @param inputWorkspace title of the focused workspace
* @param specNums number of spectrum to display
* @param bank the number of the bank as a string
* @param runNo the run number as a string
*/
void EnggDiffractionPresenter::saveOpenGenie(const std::string inputWorkspace,
                                             std::string specNums,
                                             std::string bank,
                                             std::string runNo) {

  // Generates the file name in the appropriate format
  std::string fullFilename =
      outFileNameFactory(inputWorkspace, runNo, bank, ".his");

  Poco::Path saveDir;
  if (inputWorkspace.std::string::find("curves") != std::string::npos ||
      inputWorkspace.std::string::find("intgration") != std::string::npos) {
    // Creates appropriate directory
    saveDir = outFilesDir("Calibration");
  } else {
    // Creates appropriate directory
    saveDir = outFilesDir("Focus");
  }

  // append the full file name in the end
  saveDir.append(fullFilename);

  try {
    g_log.debug() << "Going to save focused output into OpenGenie file: "
                  << fullFilename << std::endl;
    auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
        "SaveOpenGenieAscii");
    alg->initialize();
    alg->setProperty("InputWorkspace", inputWorkspace);
    std::string filename(saveDir.toString());
    alg->setPropertyValue("Filename", filename);
    alg->setPropertyValue("SpecNumberField", specNums);
    alg->execute();
  } catch (std::runtime_error &re) {
    g_log.error() << "Error in saving OpenGenie format file. ",
        "Could not run the algorithm SaveOpenGenieAscii succesfully for "
        "workspace " +
            inputWorkspace + ". Error description: " + re.what() +
            " Please check also the log messages for details.";
    throw;
  }
  g_log.notice() << "Saved focused workspace as file: " << saveDir.toString()
                 << std::endl;
}

/**
* Generates the required file name of the output files
*
* @param inputWorkspace title of the focused workspace
* @param runNo the run number as a string
* @param bank the number of the bank as a string
* @param format the format of the file to be saved as
*/
std::string EnggDiffractionPresenter::outFileNameFactory(
    std::string inputWorkspace, std::string runNo, std::string bank,
    std::string format) {
  std::string fullFilename;

  // calibration output files
  if (inputWorkspace.std::string::find("curves") != std::string::npos) {
    fullFilename = "ob+ENGINX_" + runNo + "_" + bank + "_bank" + format;

    // focus output files
  } else if (inputWorkspace.std::string::find("texture") != std::string::npos) {
    fullFilename = "ENGINX_" + runNo + "_texture_" + bank + format;
  } else if (inputWorkspace.std::string::find("cropped") != std::string::npos) {
    fullFilename = "ENGINX_" + runNo + "_cropped_" +
                   boost::lexical_cast<std::string>(g_croppedCounter) + format;
    g_croppedCounter++;
  } else {
    fullFilename = "ENGINX_" + runNo + "_bank_" + bank + format;
  }
  return fullFilename;
}

/**
* Generates a directory if not found and handles the path
*
* @param addToDir directs to right dir by passing focus or calibration
*/
Poco::Path EnggDiffractionPresenter::outFilesDir(std::string addToDir) {
  Poco::Path saveDir;
  std::string rbn = m_view->getRBNumber();

  try {

// takes to the root of directory according to the platform
// and appends the following string provided
#ifdef __unix__
    saveDir = Poco::Path().home();
    saveDir.append("EnginX_Mantid");
    saveDir.append("User");
    saveDir.append(rbn);
    saveDir.append(addToDir);
#else
    // else or for windows run this
    saveDir =
        (saveDir).expand("C:/EnginX_Mantid/User/" + rbn + "/" + addToDir + "/");
#endif

    if (!Poco::File(saveDir.toString()).exists()) {
      Poco::File(saveDir.toString()).createDirectories();
    }
  } catch (Poco::FileAccessDeniedException &e) {
    g_log.error() << "error caused by file access/permission: " << e.what();
  } catch (std::runtime_error &re) {
    g_log.error() << "Error while find/creating a path: " << re.what();
  }
  return saveDir;
}

} // namespace CustomInterfaces
} // namespace MantidQt
