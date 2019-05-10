// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "EnggDiffractionPresenter.h"
#include "EnggDiffractionPresWorker.h"
#include "EnggVanadiumCorrectionsModel.h"
#include "IEnggDiffractionView.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidQtWidgets/Common/PythonRunner.h"

#include <algorithm>
#include <cctype>
#include <fstream>

#include <boost/lexical_cast.hpp>

#include <Poco/File.h>
#include <Poco/Path.h>

#include <QThread>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;

namespace MantidQt {
namespace CustomInterfaces {

namespace {
Mantid::Kernel::Logger g_log("EngineeringDiffractionGUI");
}

const std::string EnggDiffractionPresenter::g_runNumberErrorStr =
    " cannot be empty, must be an integer number, valid ENGINX run number/s "
    "or "
    "valid directory/directories.";

// discouraged at the moment
const bool EnggDiffractionPresenter::g_askUserCalibFilename = false;

const std::string EnggDiffractionPresenter::g_calibBanksParms =
    "engggui_calibration_banks_parameters";

int EnggDiffractionPresenter::g_croppedCounter = 0;
int EnggDiffractionPresenter::g_plottingCounter = 0;
bool EnggDiffractionPresenter::g_abortThread = false;
std::string EnggDiffractionPresenter::g_lastValidRun = "";
std::string EnggDiffractionPresenter::g_calibCropIdentifier = "SpectrumNumbers";
std::string EnggDiffractionPresenter::g_sumOfFilesFocus = "";

EnggDiffractionPresenter::EnggDiffractionPresenter(IEnggDiffractionView *view)
    : m_workerThread(nullptr), m_calibFinishedOK(false),
      m_focusFinishedOK(false), m_rebinningFinishedOK(false), m_view(view),
      m_viewHasClosed(false),
      m_vanadiumCorrectionsModel(
          boost::make_shared<EnggVanadiumCorrectionsModel>(
              m_view->currentCalibSettings(), m_view->currentInstrument())) {
  if (!m_view) {
    throw std::runtime_error(
        "Severe inconsistency found. Presenter created "
        "with an empty/null view (Engineering diffraction interface). "
        "Cannot continue.");
  }

  m_currentInst = m_view->currentInstrument();
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
                        "it down immediately...\n";
      m_workerThread->wait(10);
    }
    delete m_workerThread;
    m_workerThread = nullptr;
  }

  // Remove the workspace which is loaded when the interface starts
  auto &ADS = Mantid::API::AnalysisDataService::Instance();
  if (ADS.doesExist(g_calibBanksParms)) {
    ADS.remove(g_calibBanksParms);
  }
}

void EnggDiffractionPresenter::notify(
    IEnggDiffractionPresenter::Notification notif) {

  // Check the view is valid - QT can send multiple notification
  // signals in any order at any time. This means that it is possible
  // to receive a shutdown signal and subsequently an input example
  // for example. As we can't guarantee the state of the viewer
  // after calling shutdown instead we shouldn't do anything after
  if (m_viewHasClosed) {
    return;
  }

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

void EnggDiffractionPresenter::processStart() { m_view->showStatus("Ready"); }

void EnggDiffractionPresenter::processLoadExistingCalib() {
  const std::string fname = m_view->askExistingCalibFilename();
  if (fname.empty()) {
    return;
  }

  updateNewCalib(fname);
}

/**
 * Grab a calibration from a (GSAS calibration) file
 * (.prm/.par/.iparm) and set/use it as current calibration.
 *
 * @param fname name/path of the calibration file
 */

void EnggDiffractionPresenter::updateNewCalib(const std::string &fname) {

  Poco::Path pocoPath;
  const bool pathValid = pocoPath.tryParse(fname);

  if (!pathValid || fname.empty() || pocoPath.isDirectory()) {
    // Log that we couldn't open the file - its probably and invalid
    // path which will be regenerated
    g_log.warning("Could not open GSAS calibration file: " + fname);
    return;
  }

  std::string instName, vanNo, ceriaNo;
  try {
    parseCalibrateFilename(fname, instName, vanNo, ceriaNo);
  } catch (std::invalid_argument &ia) {
    m_view->userWarning("Invalid calibration filename : " + fname, ia.what());
    return;
  }

  try {
    grabCalibParms(fname, vanNo, ceriaNo);
    updateCalibParmsTable();
    m_view->newCalibLoaded(vanNo, ceriaNo, fname);
  } catch (std::runtime_error &rexc) {
    m_view->userWarning("Problem while updating calibration.", rexc.what());
  }
}

/**
 * Get from a calibration file (GSAS instrument parameters file) the
 * DIFC, DIFA, TZERO calibration parameters used for units
 * conversions. Normally this is used on the ...all_banks.prm file
 * which has the parameters for every bank included in the calibration
 * process.
 *
 * @param fname name of the calibration/GSAS iparm file
 */
void EnggDiffractionPresenter::grabCalibParms(const std::string &fname,
                                              std::string &vanNo,
                                              std::string &ceriaNo) {
  std::vector<GSASCalibrationParms> parms;

  // To grab the bank indices, lines like "INS   BANK     2"
  // To grab the difc,difa,tzero parameters, lines like:
  // "INS  2 ICONS  18388.00    0.00    -6.76"
  try {
    std::ifstream prmFile(fname);
    std::string line;
    int opts = Mantid::Kernel::StringTokenizer::TOK_TRIM +
               Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY;
    while (std::getline(prmFile, line)) {
      if (line.find("ICONS") != std::string::npos) {
        Mantid::Kernel::StringTokenizer tokenizer(line, " ", opts);
        const size_t numElems = 6;
        if (tokenizer.count() == numElems) {
          try {
            size_t bid = boost::lexical_cast<size_t>(tokenizer[1]);
            double difc = boost::lexical_cast<double>(tokenizer[3]);
            double difa = boost::lexical_cast<double>(tokenizer[4]);
            double tzero = boost::lexical_cast<double>(tokenizer[5]);
            parms.emplace_back(GSASCalibrationParms(bid, difc, difa, tzero));
          } catch (std::runtime_error &rexc) {
            g_log.warning()
                << "Error when trying to extract parameters from this line:  "
                << line
                << ". This calibration file may not load correctly. "
                   "Error details: "
                << rexc.what() << '\n';
          }
        } else {
          g_log.warning() << "Could not parse correctly a parameters "
                             "definition line in this calibration file    ("
                          << fname << "). Did not find  " << numElems
                          << " elements as expected. The calibration may not "
                             "load correctly\n";
        }
      } else if (line.find("CALIB") != std::string::npos) {
        Mantid::Kernel::StringTokenizer tokenizer(line, " ", opts);
        ceriaNo = tokenizer[2];
        vanNo = tokenizer[3];
      }
    }

  } catch (std::runtime_error &rexc) {
    g_log.error()
        << "Error while loading calibration / GSAS IPARM file (" << fname
        << "). Could not parse the file. Please check its contents. Details: "
        << rexc.what() << '\n';
  }

  m_currentCalibParms = parms;
}

/**
 * Puts in a table workspace, visible in the ADS, the per-bank
 * calibration parameters for the current calibration.
 */
void EnggDiffractionPresenter::updateCalibParmsTable() {
  if (m_currentCalibParms.empty()) {
    return;
  }

  ITableWorkspace_sptr parmsTbl;
  AnalysisDataServiceImpl &ADS = Mantid::API::AnalysisDataService::Instance();
  if (ADS.doesExist(g_calibBanksParms)) {
    parmsTbl = ADS.retrieveWS<ITableWorkspace>(g_calibBanksParms);
    parmsTbl->setRowCount(0);
  } else {
    auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
        "CreateEmptyTableWorkspace");
    alg->initialize();
    alg->setPropertyValue("OutputWorkspace", g_calibBanksParms);
    alg->execute();

    parmsTbl = ADS.retrieveWS<ITableWorkspace>(g_calibBanksParms);
    parmsTbl->addColumn("int", "bankid");
    parmsTbl->addColumn("double", "difc");
    parmsTbl->addColumn("double", "difa");
    parmsTbl->addColumn("double", "tzero");
  }

  for (const auto &parms : m_currentCalibParms) {
    // ADS.remove(FocusedFitPeaksTableName);
    TableRow row = parmsTbl->appendRow();
    row << static_cast<int>(parms.bankid) << parms.difc << parms.difa
        << parms.tzero;
  }
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
                    "take a few seconds... \n";

  const std::string outFilename = outputCalibFilename(vanNo, ceriaNo);

  m_view->showStatus("Calculating calibration...");
  m_view->enableCalibrateFocusFitUserActions(false);
  // alternatively, this would be GUI-blocking:
  // doNewCalibration(outFilename, vanNo, ceriaNo, "");
  // calibrationFinished();
  startAsyncCalibWorker(outFilename, vanNo, ceriaNo, "");
}

void EnggDiffractionPresenter::ProcessCropCalib() {
  const std::string vanNo = isValidRunNumber(m_view->newVanadiumNo());
  const std::string ceriaNo = isValidRunNumber(m_view->newCeriaNo());
  int specNoNum = m_view->currentCropCalibBankName();
  enum BankMode { SPECNOS = 0, NORTH = 1, SOUTH = 2 };

  try {
    inputChecksBeforeCalibrate(vanNo, ceriaNo);
    if (m_view->currentCalibSpecNos().empty() &&
        specNoNum == BankMode::SPECNOS) {
      throw std::invalid_argument(
          "The Spectrum Numbers field cannot be empty, must be a "
          "valid range or a Bank Name can be selected instead.");
    }
  } catch (std::invalid_argument &ia) {
    m_view->userWarning("Error in the inputs required for cropped calibration",
                        ia.what());
    return;
  }

  g_log.notice()
      << "EnggDiffraction GUI: starting cropped calibration. This may "
         "take a few seconds... \n";

  const std::string outFilename = outputCalibFilename(vanNo, ceriaNo);

  std::string specNo = "";
  if (specNoNum == BankMode::NORTH) {
    specNo = "North";
    g_calibCropIdentifier = "Bank";

  } else if (specNoNum == BankMode::SOUTH) {
    specNo = "South";
    g_calibCropIdentifier = "Bank";

  } else if (specNoNum == BankMode::SPECNOS) {
    g_calibCropIdentifier = "SpectrumNumbers";
    specNo = m_view->currentCalibSpecNos();
  }

  m_view->showStatus("Calculating cropped calibration...");
  m_view->enableCalibrateFocusFitUserActions(false);
  startAsyncCalibWorker(outFilename, vanNo, ceriaNo, specNo);
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
    g_log.debug() << " focus mode selected Individual Run Files Separately \n";

    // start focusing
    startFocusing(multi_RunNo, banks, "", "");

  } else if (focusMode == 1) {
    g_log.debug() << " focus mode selected Focus Sum Of Files \n";
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
  const std::string specNos = m_view->focusingCroppedSpectrumNos();

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
    g_log.debug() << " focus mode selected Individual Run Files Separately \n";

    startFocusing(multi_RunNo, banks, specNos, "");

  } else if (focusMode == 1) {
    g_log.debug() << " focus mode selected Focus Sum Of Files \n";
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
    g_log.debug() << " focus mode selected Individual Run Files Separately \n";
    startFocusing(multi_RunNo, std::vector<bool>(), "", dgFile);

  } else if (focusMode == 1) {
    g_log.debug() << " focus mode selected Focus Sum Of Files \n";
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
                 << ". This may take some seconds... \n";

  m_view->showStatus("Focusing...");
  m_view->enableCalibrateFocusFitUserActions(false);
  startAsyncFocusWorker(multi_RunNo, banks, specNos, dgFile);
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
                        outWSName +
                        "'. This "
                        "may take some seconds... \n";

  m_view->showStatus("Rebinning by time...");
  m_view->enableCalibrateFocusFitUserActions(false);
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
                        outWSName +
                        "'. This "
                        "may take some seconds... \n";

  m_view->showStatus("Rebinning by pulses...");
  m_view->enableCalibrateFocusFitUserActions(false);
  startAsyncRebinningPulsesWorker(runNo, nperiods, timeStep, outWSName);
}

void EnggDiffractionPresenter::processLogMsg() {
  std::vector<std::string> msgs = m_view->logMsgs();
  for (const auto &msg : msgs) {
    g_log.information() << msg << '\n';
  }
}

void EnggDiffractionPresenter::processInstChange() {
  m_currentInst = m_view->currentInstrument();
  m_view->updateTabsInstrument(m_currentInst);
}

void EnggDiffractionPresenter::processRBNumberChange() {
  const std::string rbn = m_view->getRBNumber();
  auto valid = validateRBNumber(rbn);
  m_view->enableTabs(valid);
  m_view->showInvalidRBNumber(valid);
  if (!valid) {
    m_view->showStatus("Valid RB number required");
  } else {
    m_view->showStatus("Ready");
  }
}

void EnggDiffractionPresenter::processShutDown() {
  // Set that the view has closed in case QT attempts to fire another
  // signal whilst we are shutting down. This stops notify before
  // it hits the switch statement as the view could be in any state.
  m_viewHasClosed = true;
  m_view->showStatus("Closing...");
  m_view->saveSettings();
  cleanup();
}

void EnggDiffractionPresenter::processStopFocus() {
  if (m_workerThread) {
    if (m_workerThread->isRunning()) {
      g_log.notice() << "A focus process is currently running, shutting "
                        "it down as soon as possible...\n";

      g_abortThread = true;
      g_log.warning() << "Focus Stop has been clicked, please wait until "
                         "current focus run process has been completed. \n";
    }
  }
}

/**
 * Check if an RB number is valid to work with it (retrieve data,
 * calibrate, focus, etc.). For now this will accept any non-empty
 * string. Later on we might be more strict about valid RB numbers /
 * experiment IDs.
 *
 * @param rbn RB number as given by the user
 */
bool EnggDiffractionPresenter::validateRBNumber(const std::string &rbn) const {
  return !rbn.empty();
}

/**
 * Checks if the provided run number is valid and if a directory is
 * provided it will convert it to a run number. It also records the
 * paths the user has browsed to.
 *
 * @param userPaths the input/directory given by the user via the "browse"
 * button
 *
 * @return run_number 6 character string of a run number
 */
std::string EnggDiffractionPresenter::isValidRunNumber(
    const std::vector<std::string> &userPaths) {

  std::string run_number;
  if (userPaths.empty() || userPaths.front().empty()) {
    return run_number;
  }
  return userPaths[0];
}

/**
 * Checks if the provided run number is valid and if a directory is provided
 *
 * @param paths takes the input/paths of the user
 *
 * @return vector of string multi_run_number, 6 character string of a run number
 */
std::vector<std::string> EnggDiffractionPresenter::isValidMultiRunNumber(
    const std::vector<std::string> &paths) {

  std::vector<std::string> multi_run_number;
  if (paths.empty() || paths.front().empty())
    return multi_run_number;
  return paths;
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

  const auto &cs = m_view->currentCalibSettings();
  const auto &pixelCalib = cs.m_pixelCalibFilename;
  if (pixelCalib.empty()) {
    throw std::invalid_argument(
        "You need to set a pixel (full) calibration in settings.");
  }
  const auto &templGSAS = cs.m_templateGSAS_PRM;
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
 * @param bankName bank name when generating a file for an individual
 * bank. Leave empty to use a generic name for all banks
 *
 * @return filename (without the full path)
 */
std::string
EnggDiffractionPresenter::outputCalibFilename(const std::string &vanNo,
                                              const std::string &ceriaNo,
                                              const std::string &bankName) {
  std::string outFilename = "";
  const std::string sugg =
      buildCalibrateSuggestedFilename(vanNo, ceriaNo, bankName);
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
  const std::string &filename = fullPath.getFileName();
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
  if (m_currentInst != parts[0]) {
    throw std::invalid_argument("The first component of the file name is not "
                                "the expected instrument name: " +
                                m_currentInst + ".\n\n" + explMsg);
  }

  instName = parts[0];
}

/**
 * Start the calibration work without blocking the GUI. This uses
 * connect for Qt signals/slots so that it runs well with the Qt event
 * loop. Because of that this class needs to be a Q_OBJECT.
 *
 * @param outFilename name for the output GSAS calibration file
 * @param vanNo vanadium run number
 * @param ceriaNo ceria run number
 * @param specNos SpecNos or bank name to be passed
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
 * @param specNos SpecNos or bank name to be passed
 */
void EnggDiffractionPresenter::doNewCalibration(const std::string &outFilename,
                                                const std::string &vanNo,
                                                const std::string &ceriaNo,
                                                const std::string &specNos) {
  g_log.notice() << "Generating new calibration file: " << outFilename << '\n';

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
  for (const auto &browsed : m_browsedToPaths) {
    conf.appendDataSearchDir(browsed);
  }

  try {
    m_calibFinishedOK = true;
    doCalib(cs, vanNo, ceriaNo, outFilename, specNos);
  } catch (std::runtime_error &rexc) {
    m_calibFinishedOK = false;
    m_calibError = "The calibration calculations failed. One of the "
                   "algorithms did not execute correctly. See log messages for "
                   "further details.";
    g_log.error()
        << "The calibration calculations failed. One of the "
           "algorithms did not execute correctly. See log messages for "
           "further details. Error: " +
               std::string(rexc.what())
        << '\n';
  } catch (std::invalid_argument &iaexc) {
    m_calibFinishedOK = false;
    m_calibError = "The calibration calculations failed. Some input properties "
                   "were not valid. See log messages for details. \n Error: " +
                   std::string(iaexc.what());
    g_log.error()
        << "The calibration calculations failed. Some input properties "
           "were not valid. See log messages for details. \n";
  } catch (Mantid::API::Algorithm::CancelException &) {
    m_calibFinishedOK = false;
    m_cancelled = true;
    g_log.error() << "Execution terminated by user. \n";
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

  m_view->enableCalibrateFocusFitUserActions(true);
  if (!m_calibFinishedOK) {
    if (!m_cancelled) {
      m_view->userWarning("Calibration Error", m_calibError);
    }
    m_cancelled = false;

    m_view->showStatus("Calibration didn't finish succesfully. Ready");
  } else {
    const std::string vanNo = isValidRunNumber(m_view->newVanadiumNo());

    const std::string ceriaNo = isValidRunNumber(m_view->newCeriaNo());
    updateCalibParmsTable();
    m_view->newCalibLoaded(vanNo, ceriaNo, m_calibFullPath);
    g_log.notice()
        << "Calibration finished and ready as 'current calibration'.\n";
    m_view->showStatus("Calibration finished succesfully. Ready");
  }
  if (m_workerThread) {
    delete m_workerThread;
    m_workerThread = nullptr;
  }
}

/**
 * Build a suggested name for a new calibration, by appending instrument name,
 * relevant run numbers, etc., like: ENGINX_241391_236516_all_banks.par
 *
 * @param vanNo number of the Vanadium run
 * @param ceriaNo number of the Ceria run
 * @param bankName bank name when generating a file for an individual
 * bank. Leave empty to use a generic name for all banks
 *
 * @return Suggested name for a new calibration file, following
 * ENGIN-X practices
 */
std::string EnggDiffractionPresenter::buildCalibrateSuggestedFilename(
    const std::string &vanNo, const std::string &ceriaNo,
    const std::string &bankName) const {
  // default and only one supported instrument for now
  std::string instStr = m_currentInst;
  std::string nameAppendix = "_all_banks";
  if (!bankName.empty()) {
    nameAppendix = "_" + bankName;
  }

  // default extension for calibration files
  const std::string calibExt = ".prm";
  std::string vanFilename = Poco::Path(vanNo).getBaseName();
  std::string ceriaFilename = Poco::Path(ceriaNo).getBaseName();
  std::string vanRun =
      vanFilename.substr(vanFilename.find(instStr) + instStr.size());
  std::string ceriaRun =
      ceriaFilename.substr(ceriaFilename.find(instStr) + instStr.size());
  vanRun.erase(0, std::min(vanRun.find_first_not_of('0'), vanRun.size() - 1));
  ceriaRun.erase(
      0, std::min(ceriaRun.find_first_not_of('0'), ceriaRun.size() - 1));
  std::string sugg =
      instStr + "_" + vanRun + "_" + ceriaRun + nameAppendix + calibExt;

  return sugg;
}

std::vector<GSASCalibrationParms>
EnggDiffractionPresenter::currentCalibration() const {
  return m_currentCalibParms;
}

/**
 * Calculate a calibration, responding the the "new calibration"
 * action/button.
 *
 * @param cs user settings
 * @param vanNo Vanadium run number
 * @param ceriaNo Ceria run number
 * @param outFilename output filename chosen by the user
 * @param specNos SpecNos or bank name to be passed
 */
void EnggDiffractionPresenter::doCalib(const EnggDiffCalibSettings &cs,
                                       const std::string &vanNo,
                                       const std::string &ceriaNo,
                                       const std::string &outFilename,
                                       const std::string &specNos) {
  if (cs.m_inputDirCalib.empty()) {
    m_calibError =
        "No calibration directory selected. Please select a calibration "
        "directory in Settings. This will be used to "
        "cache Vanadium calibration data";
    g_log.warning("No calibration directory selected. Please select a "
                  "calibration directory in Settings. This will be used to "
                  "cache Vanadium calibration data");
    // This should be a userWarning once the threading problem has been dealt
    // with
    m_calibFinishedOK = false;
    return;
  }

  // TODO: the settings tab should emit a signal when these are changed, on
  // which the vanadium corrections model should be updated automatically
  m_vanadiumCorrectionsModel->setCalibSettings(cs);
  m_vanadiumCorrectionsModel->setCurrentInstrument(m_view->currentInstrument());
  const auto vanadiumCorrectionWorkspaces =
      m_vanadiumCorrectionsModel->fetchCorrectionWorkspaces(vanNo);
  const auto &vanIntegWS = vanadiumCorrectionWorkspaces.first;
  const auto &vanCurvesWS = vanadiumCorrectionWorkspaces.second;

  MatrixWorkspace_sptr ceriaWS;
  try {
    auto load = Mantid::API::AlgorithmManager::Instance().create("Load");
    load->initialize();
    load->setPropertyValue("Filename", ceriaNo);
    const std::string ceriaWSName = "engggui_calibration_sample_ws";
    load->setPropertyValue("OutputWorkspace", ceriaWSName);
    load->execute();

    AnalysisDataServiceImpl &ADS = Mantid::API::AnalysisDataService::Instance();
    ceriaWS = ADS.retrieveWS<MatrixWorkspace>(ceriaWSName);
  } catch (std::runtime_error &re) {
    g_log.error()
        << "Error while loading calibration sample data. "
           "Could not run the algorithm Load succesfully for the "
           "calibration "
           "sample (run number: " +
               ceriaNo + "). Error description: " + re.what() +
               " Please check also the previous log messages for details.";
    throw;
  }

  // Bank 1 and 2 - ENGIN-X
  // bank 1 - loops once & used for cropped calibration
  // bank 2 - loops twice, one with each bank & used for new calibration
  std::vector<double> difc, tzero;
  // for the names of the output files
  std::vector<std::string> bankNames;

  bool specNumUsed = specNos != "";
  // TODO: this needs sanitizing, it should be simpler
  if (specNumUsed) {
    constexpr size_t bankNo1 = 1;

    difc.resize(bankNo1);
    tzero.resize(bankNo1);
    int selection = m_view->currentCropCalibBankName();
    if (0 == selection) {
      // user selected "custom" name
      const std::string customName = m_view->currentCalibCustomisedBankName();
      if (customName.empty()) {
        bankNames.emplace_back("cropped");
      } else {
        bankNames.push_back(customName);
      }
    } else if (1 == selection) {
      bankNames.push_back("North");
    } else {
      bankNames.push_back("South");
    }
  } else {
    constexpr size_t bankNo2 = 2;

    difc.resize(bankNo2);
    tzero.resize(bankNo2);
    bankNames = {"North", "South"};
  }

  for (size_t i = 0; i < difc.size(); i++) {
    auto alg =
        Mantid::API::AlgorithmManager::Instance().create("EnggCalibrate");

    alg->initialize();
    alg->setProperty("InputWorkspace", ceriaWS);
    alg->setProperty("VanIntegrationWorkspace", vanIntegWS);
    alg->setProperty("VanCurvesWorkspace", vanCurvesWS);
    if (specNumUsed) {
      alg->setPropertyValue(g_calibCropIdentifier,
                            boost::lexical_cast<std::string>(specNos));
    } else {
      alg->setPropertyValue("Bank", boost::lexical_cast<std::string>(i + 1));
    }
    const std::string outFitParamsTblName =
        outFitParamsTblNameGenerator(specNos, i);
    alg->setPropertyValue("FittedPeaks", outFitParamsTblName);
    alg->setPropertyValue("OutputParametersTableName", outFitParamsTblName);
    alg->execute();
    if (!alg->isExecuted()) {
      g_log.error() << "Error in calibration. ",
          "Could not run the algorithm EnggCalibrate successfully for bank " +
              boost::lexical_cast<std::string>(i);
      throw std::runtime_error("EnggCalibrate failed");
    }

    difc[i] = alg->getProperty("DIFC");
    tzero[i] = alg->getProperty("TZERO");

    g_log.information() << " * Bank " << i + 1 << " calibrated, "
                        << "difc: " << difc[i] << ", zero: " << tzero[i]
                        << '\n';
  }

  // Creates appropriate output directory
  const std::string calibrationComp = "Calibration";
  const Poco::Path userCalSaveDir = outFilesUserDir(calibrationComp);
  const Poco::Path generalCalSaveDir = outFilesGeneralDir(calibrationComp);

  // Use poco to append filename so it is OS independent
  std::string userCalFullPath =
      appendToPath(userCalSaveDir.toString(), outFilename);
  std::string generalCalFullPath =
      appendToPath(generalCalSaveDir.toString(), outFilename);

  // Double horror: 1st use a python script
  // 2nd: because runPythonCode does this by emitting a signal that goes to
  // MantidPlot, it has to be done in the view (which is a UserSubWindow).
  // First write the all banks parameters file
  m_calibFullPath = generalCalSaveDir.toString();
  writeOutCalibFile(userCalFullPath, difc, tzero, bankNames, ceriaNo, vanNo);
  writeOutCalibFile(generalCalFullPath, difc, tzero, bankNames, ceriaNo, vanNo);

  m_currentCalibParms.clear();

  // Then write one individual file per bank, using different templates and the
  // specific bank name as suffix
  for (size_t bankIdx = 0; bankIdx < difc.size(); ++bankIdx) {
    // Need to use van number not file name here else it will be
    // "ENGINX_ENGINX12345_ENGINX12345...." as out name
    const std::string bankFilename = buildCalibrateSuggestedFilename(
        vanNo, ceriaNo, "bank_" + bankNames[bankIdx]);

    // Regenerate both full paths for each bank now
    userCalFullPath = appendToPath(userCalSaveDir.toString(), bankFilename);
    generalCalFullPath =
        appendToPath(generalCalSaveDir.toString(), bankFilename);

    std::string templateFile = "template_ENGINX_241391_236516_North_bank.prm";
    if (1 == bankIdx) {
      templateFile = "template_ENGINX_241391_236516_South_bank.prm";
    }

    writeOutCalibFile(userCalFullPath, {difc[bankIdx]}, {tzero[bankIdx]},
                      {bankNames[bankIdx]}, ceriaNo, vanNo, templateFile);
    writeOutCalibFile(generalCalFullPath, {difc[bankIdx]}, {tzero[bankIdx]},
                      {bankNames[bankIdx]}, ceriaNo, vanNo, templateFile);

    m_currentCalibParms.emplace_back(
        GSASCalibrationParms(bankIdx, difc[bankIdx], 0.0, tzero[bankIdx]));
    if (1 == difc.size()) {
      // it is a  single bank or cropped calibration, so take its specific name
      m_calibFullPath = generalCalFullPath;
    }
  }
  g_log.notice() << "Calibration file written as " << generalCalFullPath << '\n'
                 << "And: " << userCalFullPath;

  // plots the calibrated workspaces.
  g_plottingCounter++;
  plotCalibWorkspace(difc, tzero, specNos);
}

/**
 * Appends the current instrument as a filename prefix for numeric
 * only inputs of the Vanadium run so Load can find the file
 *
 * @param vanNo The user input for the vanadium run
 * @param outVanName The fixed filename for the vanadium run
 */
void EnggDiffractionPresenter::appendCalibInstPrefix(
    const std::string &vanNo, std::string &outVanName) const {
  // Use a single non numeric digit so we are guaranteed to skip
  // generating cerium file names
  const std::string cer = "-";
  std::string outCerName;
  appendCalibInstPrefix(vanNo, cer, outVanName, outCerName);
}

/**
 * Appends the current instrument as a filename prefix for numeric
 * only inputs of both the Vanadium and Cerium Oxide runs so Load
 * can find the files.
 *
 * @param vanNo The user input for the vanadium run
 * @param cerNo The user input for the cerium run
 * @param outVanName The fixed filename for the vanadium run
 * @param outCerName The fixed filename for the cerium run
 */
void EnggDiffractionPresenter::appendCalibInstPrefix(
    const std::string &vanNo, const std::string &cerNo, std::string &outVanName,
    std::string &outCerName) const {

  // Load uses the default instrument if we don't give it the name of the
  // instrument as a prefix (m_currentInst), when one isn't set or is set
  // incorrectly, we prepend the name of the instrument to the vanadium number
  // so that load can find the file and not cause a crash in Mantid
  // Vanadium file
  if (std::all_of(vanNo.begin(), vanNo.end(), ::isdigit)) {
    // This only has digits - append prefix
    outVanName = m_currentInst + vanNo;
  }

  // Cerium file
  if (std::all_of(cerNo.begin(), cerNo.end(), ::isdigit)) {
    // All digits - append inst prefix
    outCerName = m_currentInst + cerNo;
  }
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
    throw std::invalid_argument(
        "The Spectrum Numbers field cannot be empty when "
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
    g_log.error() << msg << '\n';
    throw std::invalid_argument(msg);
  }
  if (banks.end() == std::find(banks.begin(), banks.end(), true)) {
    const std::string msg =
        "EnggDiffraction GUI: not focusing, as none of the banks "
        "have been selected. You probably forgot to select at least one.";
    g_log.warning() << msg << '\n';
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
  const std::string runNumber = Poco::Path(runNo).getBaseName();
  std::vector<std::string> res;
  res.reserve(banks.size());
  const auto instrumentPresent = runNumber.find(instStr);
  std::string runName =
      (instrumentPresent != std::string::npos)
          ? runNumber.substr(instrumentPresent + instStr.size())
          : runNumber;

  std::string prefix = instStr + "_" +
                       runName.erase(0, std::min(runName.find_first_not_of('0'),
                                                 runName.size() - 1)) +
                       "_focused_bank_";

  for (size_t b = 1; b <= banks.size(); b++) {
    res.emplace_back(prefix + boost::lexical_cast<std::string>(b) + ".nxs");
  }
  return res;
}

std::string
EnggDiffractionPresenter::outputFocusCroppedFilename(const std::string &runNo) {
  const std::string instStr = m_view->currentInstrument();
  const std::string runNumber = Poco::Path(runNo).getBaseName();
  const auto instrumentPresent = runNumber.find(instStr);
  std::string runName =
      (instrumentPresent != std::string::npos)
          ? runNumber.substr(instrumentPresent + instStr.size())
          : runNumber;
  return instStr + "_" + runName + "_focused_cropped.nxs";
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
  const std::string runNumber = Poco::Path(runNo).getBaseName();
  const std::string runName =
      runNumber.substr(runNumber.find(instStr) + instStr.size());
  std::vector<std::string> res;
  res.reserve(bankIDs.size());
  std::string prefix = instStr + "_" + runName + "_focused_texture_bank_";
  for (auto bankID : bankIDs) {
    res.emplace_back(prefix + boost::lexical_cast<std::string>(bankID) +
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
 * @param multi_RunNo input vector of run number
 * @param banks instrument bank to focus
 * @param specNos list of spectra (as usual csv list of spectra in Mantid)
 * @param dgFile detector grouping file name
 */
void EnggDiffractionPresenter::startAsyncFocusWorker(
    const std::vector<std::string> &multi_RunNo, const std::vector<bool> &banks,
    const std::string &dgFile, const std::string &specNos) {

  delete m_workerThread;
  m_workerThread = new QThread(this);
  EnggDiffWorker *worker =
      new EnggDiffWorker(this, multi_RunNo, banks, dgFile, specNos);
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
void EnggDiffractionPresenter::doFocusRun(const std::string &runNo,
                                          const std::vector<bool> &banks,
                                          const std::string &specNos,
                                          const std::string &dgFile) {

  if (g_abortThread) {
    return;
  }

  // to track last valid run
  g_lastValidRun = runNo;

  g_log.notice() << "Generating new focusing workspace(s) and file(s)";

  // TODO: this is almost 100% common with doNewCalibrate() - refactor
  EnggDiffCalibSettings cs = m_view->currentCalibSettings();
  Mantid::Kernel::ConfigServiceImpl &conf =
      Mantid::Kernel::ConfigService::Instance();
  const std::vector<std::string> tmpDirs = conf.getDataSearchDirs();
  // in principle, the run files will be found from 'DirRaw', and the
  // pre-calculated Vanadium corrections from 'DirCalib'
  if (!cs.m_inputDirCalib.empty() && Poco::File(cs.m_inputDirCalib).exists()) {
    conf.appendDataSearchDir(cs.m_inputDirCalib);
  }
  if (!cs.m_inputDirRaw.empty() && Poco::File(cs.m_inputDirRaw).exists()) {
    conf.appendDataSearchDir(cs.m_inputDirRaw);
  }
  for (const auto &browsed : m_browsedToPaths) {
    conf.appendDataSearchDir(browsed);
  }

  // Prepare special inputs for "texture" focusing
  std::vector<size_t> bankIDs;
  std::vector<std::string> effectiveFilenames;
  std::vector<std::string> specs;
  if (!specNos.empty()) {
    // Cropped focusing
    // just to iterate once, but there's no real bank here
    bankIDs.push_back(0);
    specs.push_back(specNos); // one spectrum Nos list given by the user
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
                      << '\n';
        bankIDs.clear();
        specs.clear();
      }
      effectiveFilenames = outputFocusTextureFilenames(runNo, bankIDs);
    }
  }

  // focus all requested banks
  for (size_t idx = 0; idx < bankIDs.size(); idx++) {
    g_log.notice() << "Generating new focused file (bank " +
                          boost::lexical_cast<std::string>(bankIDs[idx]) +
                          ") for run " + runNo + " into: "
                   << effectiveFilenames[idx] << '\n';
    try {

      doFocusing(cs, runNo, bankIDs[idx], specs[idx], dgFile);
      m_focusFinishedOK = true;
    } catch (std::runtime_error &rexc) {
      m_focusFinishedOK = false;
      g_log.error() << "The focusing calculations failed. One of the algorithms"
                       "did not execute correctly. See log messages for "
                       "further details. Error: " +
                           std::string(rexc.what())
                    << '\n';
    } catch (std::invalid_argument &ia) {
      m_focusFinishedOK = false;
      g_log.error() << "The focusing failed. Some input properties "
                       "were not valid. "
                       "See log messages for details. Error: "
                    << ia.what() << '\n';
    } catch (Mantid::API::Algorithm::CancelException) {
      m_focusFinishedOK = false;
      g_log.error() << "Focus terminated by user.\n";
    }
  }

  // restore initial data search paths
  conf.setDataSearchDirs(tmpDirs);
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
          "In file '" + dgFile +
          "', wrong format in line: " + boost::lexical_cast<std::string>(li) +
          " which does not contain any delimiters (comma, etc.)");
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
        throw std::runtime_error(
            "In file '" + dgFile +
            "', wrong format in line: " + boost::lexical_cast<std::string>(li) +
            ", the list of spectrum Nos is empty!");
      }

      size_t bankID = boost::lexical_cast<size_t>(bstr);
      bankIDs.push_back(bankID);
      specs.push_back(spec);
    } catch (std::runtime_error &re) {
      throw std::runtime_error(
          "In file '" + dgFile +
          "', issue found when trying to interpret line: " +
          boost::lexical_cast<std::string>(li) +
          ". Error description: " + re.what());
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

  if (!m_focusFinishedOK) {
    g_log.warning() << "The focusing did not finish correctly. Check previous "
                       "log messages for details\n";
    m_view->showStatus("Focusing didn't finish succesfully. Ready");
  } else {
    g_log.notice() << "Focusing finished - focused run(s) are ready.\n";
    m_view->showStatus("Focusing finished succesfully. Ready");
  }
  if (m_workerThread) {
    delete m_workerThread;
    m_workerThread = nullptr;
  }

  m_view->enableCalibrateFocusFitUserActions(true);

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
          << (lastRun - lastValid) << '\n';
      m_view->showStatus("Focusing stopped. Ready");
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
 * @param runLabel run number and bank ID of the run to focus
 *
 * @param specNos string specifying a list of spectra (for "cropped"
 * focusing or "texture" focusing), only considered if not empty
 *
 * @param dgFile detector grouping file name. If not empty implies
 * texture focusing
 */
void EnggDiffractionPresenter::doFocusing(const EnggDiffCalibSettings &cs,
                                          const std::string &runLabel,
                                          const size_t bank,
                                          const std::string &specNos,
                                          const std::string &dgFile) {
  MatrixWorkspace_sptr inWS;

  m_vanadiumCorrectionsModel->setCalibSettings(cs);
  m_vanadiumCorrectionsModel->setCurrentInstrument(m_view->currentInstrument());
  const auto vanadiumCorrectionWorkspaces =
      m_vanadiumCorrectionsModel->fetchCorrectionWorkspaces(
          m_view->currentVanadiumNo());
  const auto &vanIntegWS = vanadiumCorrectionWorkspaces.first;
  const auto &vanCurvesWS = vanadiumCorrectionWorkspaces.second;

  const std::string inWSName = "engggui_focusing_input_ws";
  const std::string instStr = m_view->currentInstrument();
  std::vector<std::string> multi_RunNo = sumOfFilesLoadVec();
  std::string loadInput = "";

  for (size_t i = 0; i < multi_RunNo.size(); i++) {
    // if last run number in list
    if (i + 1 == multi_RunNo.size())
      loadInput += multi_RunNo[i];
    else
      loadInput += multi_RunNo[i] + '+';
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
                        "cannot not be processed\n";
    } else {
      g_log.notice()
          << "Load alogirthm has successfully merged the files provided\n";
    }

  } else {
    try {
      auto load = Mantid::API::AlgorithmManager::Instance().create("Load");
      load->initialize();
      load->setPropertyValue("Filename", runLabel);
      load->setPropertyValue("OutputWorkspace", inWSName);
      load->execute();

      AnalysisDataServiceImpl &ADS =
          Mantid::API::AnalysisDataService::Instance();
      inWS = ADS.retrieveWS<MatrixWorkspace>(inWSName);
    } catch (std::runtime_error &re) {
      g_log.error() << "Error while loading sample data for focusing. "
                       "Could not run the algorithm Load succesfully for "
                       "the focusing "
                       "sample (run number: " +
                           runLabel + "). Error description: " + re.what() +
                           " Please check also the previous log messages "
                           "for details.";
      throw;
    }
  }
  const auto bankString = std::to_string(bank);
  std::string outWSName;
  if (!dgFile.empty()) {
    // doing focus "texture"
    outWSName = "engggui_focusing_output_ws_texture_bank_" + bankString;
  } else if (specNos.empty()) {
    // doing focus "normal" / by banks
    outWSName = "engggui_focusing_output_ws_bank_" + bankString;
  } else {
    // doing focus "cropped"
    outWSName = "engggui_focusing_output_ws_cropped";
  }
  try {
    auto alg = Mantid::API::AlgorithmManager::Instance().create("EnggFocus");
    alg->initialize();
    alg->setProperty("InputWorkspace", inWSName);
    alg->setProperty("OutputWorkspace", outWSName);
    alg->setProperty("VanIntegrationWorkspace", vanIntegWS);
    alg->setProperty("VanCurvesWorkspace", vanCurvesWS);
    // cropped / normal focusing
    if (specNos.empty()) {
      alg->setPropertyValue("Bank", bankString);
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
        "Could not run the algorithm EnggCalibrate successfully for bank " +
            bankString + ". Error description: " + re.what() +
            " Please check also the log messages for details.";
    throw;
  }
  g_log.notice() << "Produced focused workspace: " << outWSName << '\n';

  const bool saveOutputFiles = m_view->saveFocusedOutputFiles();
  if (saveOutputFiles) {
    try {
      const auto runNo =
          runLabel.substr(runLabel.rfind(instStr) + instStr.size());
      RunLabel label(runNo, bank);
      saveFocusedXYE(label, outWSName);
      saveGSS(label, outWSName);
      saveOpenGenie(label, outWSName);
      saveNexus(label, outWSName);
      exportSampleLogsToHDF5(outWSName, userHDFRunFilename(runNo));
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
 * Loads a workspace to pre-process (rebin, etc.). The workspace
 * loaded can be a MatrixWorkspace or a group of MatrixWorkspace (for
 * multiperiod data).
 *
 * @param runNo run number to search for the file with 'Load'.
 */
Workspace_sptr
EnggDiffractionPresenter::loadToPreproc(const std::string &runNo) {
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
    g_log.error()
        << "Error: could not load the input workspace for rebinning.\n";

  const std::string rebinName = "Rebin";
  try {
    auto alg =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged(rebinName);
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", inWS->getName());
    alg->setPropertyValue("OutputWorkspace", outWSName);
    alg->setProperty("Params", boost::lexical_cast<std::string>(bin));

    alg->execute();
  } catch (std::invalid_argument &ia) {
    g_log.error() << "Error when rebinning with a regular bin width in time. "
                     "There was an error in the inputs to the algorithm " +
                         rebinName + ". Error description: " + ia.what() +
                         ".\n";
    return;
  } catch (std::runtime_error &re) {
    g_log.error() << "Error when rebinning with a regular bin width in time. "
                     "Coult not run the algorithm " +
                         rebinName +
                         " successfully. Error description: " + re.what() +
                         ".\n";
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
    g_log.error()
        << "Error: could not load the input workspace for rebinning.\n";

  const std::string rebinName = "RebinByPulseTimes";
  try {
    auto alg =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged(rebinName);
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", inWS->getName());
    alg->setPropertyValue("OutputWorkspace", outWSName);
    alg->setProperty("Params", boost::lexical_cast<std::string>(timeStep));

    alg->execute();
  } catch (std::invalid_argument &ia) {
    g_log.error() << "Error when rebinning by pulse times. "
                     "There was an error in the inputs to the algorithm " +
                         rebinName + ". Error description: " + ia.what() +
                         ".\n";
    return;
  } catch (std::runtime_error &re) {
    g_log.error() << "Error when rebinning by pulse times. "
                     "Coult not run the algorithm " +
                         rebinName +
                         " successfully. Error description: " + re.what() +
                         ".\n";
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

  if (!m_rebinningFinishedOK) {
    g_log.warning() << "The pre-processing (re-binning) did not finish "
                       "correctly. Check previous log messages for details\n";
    m_view->showStatus("Rebinning didn't finish succesfully. Ready");
  } else {
    g_log.notice() << "Pre-processing (re-binning) finished - the output "
                      "workspace is ready.\n";
    m_view->showStatus("Rebinning finished succesfully. Ready");
  }
  if (m_workerThread) {
    delete m_workerThread;
    m_workerThread = nullptr;
  }

  m_view->enableCalibrateFocusFitUserActions(true);
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
        m_view->plotReplacingWindow(outWSName, "0", "0");

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
 * Check if the plot calibration check-box is ticked
 * python script is passed on to mantid python window
 * which plots the workspaces with customisation
 *
 * @param difc vector of double passed on to py script
 * @param tzero vector of double to plot graph
 * @param specNos string carrying cropped calib info
 */
void EnggDiffractionPresenter::plotCalibWorkspace(std::vector<double> difc,
                                                  std::vector<double> tzero,
                                                  std::string specNos) {
  const bool plotCalibWS = m_view->plotCalibWorkspace();
  if (plotCalibWS) {
    std::string pyCode = vanadiumCurvesPlotFactory();
    m_view->plotCalibOutput(pyCode);

    // Get the Customised Bank Name text-ield string from qt
    std::string CustomisedBankName = m_view->currentCalibCustomisedBankName();
    if (CustomisedBankName.empty())
      CustomisedBankName = "cropped";
    const std::string pythonCode =
        DifcZeroWorkspaceFactory(difc, tzero, specNos, CustomisedBankName) +
        plotDifcZeroWorkspace(CustomisedBankName);
    m_view->plotCalibOutput(pythonCode);
  }
}

/**
 * Convert the generated output files and saves them in
 * FocusedXYE format
 *
 * @param runLabel run number and bank ID of the workspace to save
 * @param inputWorkspace title of the focused workspace
 */
void EnggDiffractionPresenter::saveFocusedXYE(
    const RunLabel &runLabel, const std::string &inputWorkspace) {

  // Generates the file name in the appropriate format
  std::string fullFilename =
      outFileNameFactory(inputWorkspace, runLabel, ".dat");

  const std::string focusingComp = "Focus";
  // Creates appropriate directory
  auto saveDir = outFilesUserDir(focusingComp);

  // append the full file name in the end
  saveDir.append(fullFilename);

  try {
    g_log.debug() << "Going to save focused output into OpenGenie file: "
                  << fullFilename << '\n';
    auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
        "SaveFocusedXYE");
    alg->initialize();
    alg->setProperty("InputWorkspace", inputWorkspace);
    const std::string filename(saveDir.toString());
    alg->setPropertyValue("Filename", filename);
    alg->setProperty("SplitFiles", false);
    alg->setPropertyValue("StartAtBankNumber", std::to_string(runLabel.bank));
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
                 << '\n';
  copyToGeneral(saveDir, focusingComp);
}

/**
 * Convert the generated output files and saves them in
 * GSS format
 *
 * @param runLabel run number and bank ID the workspace to save
 * @param inputWorkspace title of the focused workspace
 */
void EnggDiffractionPresenter::saveGSS(const RunLabel &runLabel,
                                       const std::string &inputWorkspace) {

  // Generates the file name in the appropriate format
  std::string fullFilename =
      outFileNameFactory(inputWorkspace, runLabel, ".gss");

  const std::string focusingComp = "Focus";
  // Creates appropriate directory
  auto saveDir = outFilesUserDir(focusingComp);

  // append the full file name in the end
  saveDir.append(fullFilename);

  try {
    g_log.debug() << "Going to save focused output into OpenGenie file: "
                  << fullFilename << '\n';
    auto alg =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged("SaveGSS");
    alg->initialize();
    alg->setProperty("InputWorkspace", inputWorkspace);
    std::string filename(saveDir.toString());
    alg->setPropertyValue("Filename", filename);
    alg->setProperty("SplitFiles", false);
    alg->setPropertyValue("Bank", std::to_string(runLabel.bank));
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
                 << '\n';
  copyToGeneral(saveDir, focusingComp);
}

void EnggDiffractionPresenter::saveNexus(const RunLabel &runLabel,
                                         const std::string &inputWorkspace) {
  const auto filename = outFileNameFactory(inputWorkspace, runLabel, ".nxs");
  auto saveDirectory = outFilesUserDir("Focus");
  saveDirectory.append(filename);
  const auto fullOutFileName = saveDirectory.toString();

  try {
    g_log.debug() << "Going to save focused output into OpenGenie file: "
                  << fullOutFileName << "\n";
    auto alg =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged("SaveNexus");
    alg->initialize();
    alg->setProperty("InputWorkspace", inputWorkspace);
    alg->setProperty("Filename", fullOutFileName);
    alg->execute();
  } catch (std::runtime_error &re) {
    g_log.error() << "Error in save NXS format file. Could not run the "
                     "algorithm SaveNexus successfully for workspace "
                  << inputWorkspace << ". Error description: " << re.what()
                  << ". Please also check the log message for details.";
    throw;
  }
  g_log.notice() << "Saved focused workspace as file: " << fullOutFileName
                 << "\n";
  copyToGeneral(saveDirectory, "Focus");
}

/**
 * Convert the generated output files and saves them in
 * OpenGenie format
 *
 * @param runLabel run number and bank ID of the workspace to save
 * @param inputWorkspace title of the focused workspace
 */
void EnggDiffractionPresenter::saveOpenGenie(
    const RunLabel &runLabel, const std::string &inputWorkspace) {

  // Generates the file name in the appropriate format
  std::string fullFilename =
      outFileNameFactory(inputWorkspace, runLabel, ".his");

  std::string comp;
  Poco::Path saveDir;
  if (inputWorkspace.std::string::find("curves") != std::string::npos ||
      inputWorkspace.std::string::find("intgration") != std::string::npos) {
    // Creates appropriate directory
    comp = "Calibration";
    saveDir = outFilesUserDir(comp);
  } else {
    // Creates appropriate directory
    comp = "Focus";
    saveDir = outFilesUserDir(comp);
  }

  // append the full file name in the end
  saveDir.append(fullFilename);

  try {
    g_log.debug() << "Going to save focused output into OpenGenie file: "
                  << fullFilename << '\n';
    auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged(
        "SaveOpenGenieAscii");
    alg->initialize();
    alg->setProperty("InputWorkspace", inputWorkspace);
    std::string filename(saveDir.toString());
    alg->setPropertyValue("Filename", filename);
    alg->setPropertyValue("OpenGenieFormat", "ENGIN-X Format");
    alg->execute();
  } catch (std::runtime_error &re) {
    g_log.error() << "Error in saving OpenGenie format file. ",
        "Could not run the algorithm SaveOpenGenieAscii succesfully for "
        "workspace " +
            inputWorkspace + ". Error description: " + re.what() +
            " Please check also the log messages for details.";
    throw;
  }
  g_log.notice() << "Saves OpenGenieAscii (.his) file written as: "
                 << saveDir.toString() << '\n';
  copyToGeneral(saveDir, comp);
}

void EnggDiffractionPresenter::exportSampleLogsToHDF5(
    const std::string &inputWorkspace, const std::string &filename) const {
  auto saveAlg = Mantid::API::AlgorithmManager::Instance().create(
      "ExportSampleLogsToHDF5");
  saveAlg->initialize();
  saveAlg->setProperty("InputWorkspace", inputWorkspace);
  saveAlg->setProperty("Filename", filename);
  saveAlg->setProperty("Blacklist", "bankid");
  saveAlg->execute();
}

/**
 * Generates the required file name of the output files
 *
 * @param inputWorkspace title of the focused workspace
 * @param runLabel run number and bank ID of the workspace to save
 * @param format the format of the file to be saved as
 */
std::string
EnggDiffractionPresenter::outFileNameFactory(const std::string &inputWorkspace,
                                             const RunLabel &runLabel,
                                             const std::string &format) {
  std::string fullFilename;

  const auto runNo = runLabel.runNumber;
  const auto bank = std::to_string(runLabel.bank);

  // calibration output files
  if (inputWorkspace.std::string::find("curves") != std::string::npos) {
    fullFilename =
        "ob+" + m_currentInst + "_" + runNo + "_" + bank + "_bank" + format;

    // focus output files
  } else if (inputWorkspace.std::string::find("texture") != std::string::npos) {
    fullFilename = m_currentInst + "_" + runNo + "_texture_" + bank + format;
  } else if (inputWorkspace.std::string::find("cropped") != std::string::npos) {
    fullFilename = m_currentInst + "_" + runNo + "_cropped_" +
                   boost::lexical_cast<std::string>(g_croppedCounter) + format;
    g_croppedCounter++;
  } else {
    fullFilename = m_currentInst + "_" + runNo + "_bank_" + bank + format;
  }
  return fullFilename;
}

std::string EnggDiffractionPresenter::vanadiumCurvesPlotFactory() {
  std::string pyCode =

      "van_curve_twin_ws = \"__engggui_vanadium_curves_twin_ws\"\n"

      "if(mtd.doesExist(van_curve_twin_ws)):\n"
      " DeleteWorkspace(van_curve_twin_ws)\n"

      "CloneWorkspace(InputWorkspace = \"engggui_vanadium_curves\", "
      "OutputWorkspace = van_curve_twin_ws)\n"

      "van_curves_ws = workspace(van_curve_twin_ws)\n"
      "for i in range(1, 3):\n"
      " if (i == 1):\n"
      "  curve_plot_bank_1 = plotSpectrum(van_curves_ws, [0, 1, "
      "2]).activeLayer()\n"
      "  curve_plot_bank_1.setTitle(\"Engg GUI Vanadium Curves Bank 1\")\n"

      " if (i == 2):\n"
      "  curve_plot_bank_2 = plotSpectrum(van_curves_ws, [3, 4, "
      "5]).activeLayer()\n"
      "  curve_plot_bank_2.setTitle(\"Engg GUI Vanadium Curves Bank 2\")\n";

  return pyCode;
}

/**
 * Generates the workspace with difc/zero according to the selected bank
 *
 * @param difc vector containing constants difc value of each bank
 * @param tzero vector containing constants tzero value of each bank
 * @param specNo used to set range for Calibration Cropped
 * @param customisedBankName used to set the file and workspace name
 *
 * @return string with a python script
 */
std::string EnggDiffractionPresenter::DifcZeroWorkspaceFactory(
    const std::vector<double> &difc, const std::vector<double> &tzero,
    const std::string &specNo, const std::string &customisedBankName) const {

  size_t bank1 = size_t(0);
  size_t bank2 = size_t(1);
  std::string pyRange;
  std::string plotSpecNum = "False";

  // sets the range to plot appropriate graph for the particular bank
  if (specNo == "North") {
    // only enable script to plot bank 1
    pyRange = "1, 2";
  } else if (specNo == "South") {
    // only enables python script to plot bank 2
    // as bank 2 data will be located in difc[0] & tzero[0] - refactor
    pyRange = "2, 3";
    bank2 = size_t(0);
  } else if (specNo != "") {
    pyRange = "1, 2";
    plotSpecNum = "True";
  } else {
    // enables python script to plot bank 1 & 2
    pyRange = "1, 3";
  }

  std::string pyCode =
      "plotSpecNum = " + plotSpecNum +
      "\n"
      "for i in range(" +
      pyRange +
      "):\n"

      " if (plotSpecNum == False):\n"
      "  bank_ws = workspace(\"engggui_calibration_bank_\" + str(i))\n"
      " else:\n"
      "  bank_ws = workspace(\"engggui_calibration_bank_" +
      customisedBankName +
      "\")\n"

      " xVal = []\n"
      " yVal = []\n"
      " y2Val = []\n"

      " if (i == 1):\n"
      "  difc=" +
      boost::lexical_cast<std::string>(difc[bank1]) + "\n" +
      "  tzero=" + boost::lexical_cast<std::string>(tzero[bank1]) + "\n" +
      " else:\n"

      "  difc=" +
      boost::lexical_cast<std::string>(difc[bank2]) + "\n" +
      "  tzero=" + boost::lexical_cast<std::string>(tzero[bank2]) + "\n" +

      " for irow in range(0, bank_ws.rowCount()):\n"
      "  xVal.append(bank_ws.cell(irow, 0))\n"
      "  yVal.append(bank_ws.cell(irow, 5))\n"

      "  y2Val.append(xVal[irow] * difc + tzero)\n"

      " ws1 = CreateWorkspace(DataX=xVal, DataY=yVal, UnitX=\"Expected "
      "Peaks "
      " Centre(dSpacing, A)\", YUnitLabel = \"Fitted Peaks Centre(TOF, "
      "us)\")\n"
      " ws2 = CreateWorkspace(DataX=xVal, DataY=y2Val)\n";
  return pyCode;
}

/**
* Plot the workspace with difc/zero acordding to selected bank
*
* @param customisedBankName used to set the file and workspace name
*
* @return string with a python script which will merge with
*

*/
std::string EnggDiffractionPresenter::plotDifcZeroWorkspace(
    const std::string &customisedBankName) const {
  std::string pyCode =
      // plotSpecNum is true when SpectrumNos being used
      " if (plotSpecNum == False):\n"
      "  output_ws = \"engggui_difc_zero_peaks_bank_\" + str(i)\n"
      " else:\n"
      "  output_ws = \"engggui_difc_zero_peaks_" +
      customisedBankName +
      "\"\n"

      // delete workspace if exists within ADS already
      " if(mtd.doesExist(output_ws)):\n"
      "  DeleteWorkspace(output_ws)\n"

      // append workspace with peaks data for Peaks Fitted
      // and Difc/TZero Straight line
      " AppendSpectra(ws1, ws2, OutputWorkspace=output_ws)\n"
      " DeleteWorkspace(ws1)\n"
      " DeleteWorkspace(ws2)\n"

      " if (plotSpecNum == False):\n"
      "  DifcZero = \"engggui_difc_zero_peaks_bank_\" + str(i)\n"
      " else:\n"
      "  DifcZero = \"engggui_difc_zero_peaks_" +
      customisedBankName +
      "\"\n"

      " DifcZeroWs = workspace(DifcZero)\n"
      " DifcZeroPlot = plotSpectrum(DifcZeroWs, [0, 1]).activeLayer()\n"

      " if (plotSpecNum == False):\n"
      "  DifcZeroPlot.setTitle(\"Engg Gui Difc Zero Peaks Bank \" + "
      "str(i))\n"
      " else:\n"
      "  DifcZeroPlot.setTitle(\"Engg Gui Difc Zero Peaks " +
      customisedBankName +
      "\")\n"

      // set the legend title
      " DifcZeroPlot.setCurveTitle(0, \"Peaks Fitted\")\n"
      " DifcZeroPlot.setCurveTitle(1, \"DifC/TZero Fitted Straight Line\")\n"
      " DifcZeroPlot.setAxisTitle(Layer.Bottom, \"Expected Peaks "
      "Centre(dSpacing, "
      " A)\")\n"
      " DifcZeroPlot.setCurveLineStyle(0, QtCore.Qt.DotLine)\n";

  return pyCode;
}

/**
 * Generates appropriate names for table workspaces
 *
 * @param specNos SpecNos or bank name to be passed
 * @param bank_i current loop of the bank during calibration
 */
std::string EnggDiffractionPresenter::outFitParamsTblNameGenerator(
    const std::string &specNos, const size_t bank_i) const {
  std::string outFitParamsTblName;
  bool specNumUsed = specNos != "";

  if (specNumUsed) {
    if (specNos == "North")
      outFitParamsTblName = "engggui_calibration_bank_1";
    else if (specNos == "South")
      outFitParamsTblName = "engggui_calibration_bank_2";
    else {
      // Get the Customised Bank Name text-ield string from qt
      std::string CustomisedBankName = m_view->currentCalibCustomisedBankName();

      if (CustomisedBankName.empty())
        outFitParamsTblName = "engggui_calibration_bank_cropped";
      else
        outFitParamsTblName = "engggui_calibration_bank_" + CustomisedBankName;
    }
  } else {
    outFitParamsTblName = "engggui_calibration_bank_" +
                          boost::lexical_cast<std::string>(bank_i + 1);
  }
  return outFitParamsTblName;
}

/**
 * Produces a path to the output directory where files are going to be
 * written for a specific user + RB number / experiment ID. It creates
 * the output directory if not found, and checks if it is ok and readable.
 *
 * @param addToDir adds a component to a specific directory for
 * focusing, calibration or other files, for example "Calibration" or
 * "Focus"
 */
Poco::Path
EnggDiffractionPresenter::outFilesUserDir(const std::string &addToDir) const {
  std::string rbn = m_view->getRBNumber();
  Poco::Path dir = outFilesRootDir();

  try {
    dir.append("User");
    dir.append(rbn);
    dir.append(addToDir);

    Poco::File dirFile(dir);
    if (!dirFile.exists()) {
      dirFile.createDirectories();
    }
  } catch (Poco::FileAccessDeniedException &e) {
    g_log.error() << "Error caused by file access/permission, path to user "
                     "directory: "
                  << dir.toString() << ". Error details: " << e.what() << '\n';
  } catch (std::runtime_error &re) {
    g_log.error() << "Error while finding/creating a user path: "
                  << dir.toString() << ". Error details: " << re.what() << '\n';
  }
  return dir;
}

std::string EnggDiffractionPresenter::userHDFRunFilename(
    const std::string runNumber) const {
  auto userOutputDir = outFilesUserDir("Runs");
  userOutputDir.append(runNumber + ".hdf5");
  return userOutputDir.toString();
}

std::string EnggDiffractionPresenter::userHDFMultiRunFilename(
    const std::vector<RunLabel> &runLabels) const {
  const auto &begin = runLabels.cbegin();
  const auto &end = runLabels.cend();
  const auto minLabel = std::min_element(begin, end);
  const auto maxLabel = std::max_element(begin, end);
  auto userOutputDir = outFilesUserDir("Runs");
  userOutputDir.append((minLabel->runNumber) + "_" +
                       (maxLabel->runNumber) + ".hdf5");
  return userOutputDir.toString();
}

/**
 * Produces a path to the output directory where files are going to be
 * written for a specific user + RB number / experiment ID. It creates
 * the output directory if not found. See outFilesUserDir() for the
 * sibling method that produces user/rb number-specific directories.
 *
 * @param addComponent path component to add to the root of general
 * files, for example "Calibration" or "Focus"
 */
Poco::Path
EnggDiffractionPresenter::outFilesGeneralDir(const std::string &addComponent) {
  Poco::Path dir = outFilesRootDir();

  try {

    dir.append(addComponent);

    Poco::File dirFile(dir);
    if (!dirFile.exists()) {
      dirFile.createDirectories();
    }
  } catch (Poco::FileAccessDeniedException &e) {
    g_log.error() << "Error caused by file access/permission, path to "
                     "general directory: "
                  << dir.toString() << ". Error details: " << e.what() << '\n';
  } catch (std::runtime_error &re) {
    g_log.error() << "Error while finding/creating a general path: "
                  << dir.toString() << ". Error details: " << re.what() << '\n';
  }
  return dir;
}

/**
 * Produces the root path where output files are going to be written.
 */
Poco::Path EnggDiffractionPresenter::outFilesRootDir() const {
  // TODO decide whether to move into settings or use mantid's default directory
  // after discussion with users
  const std::string rootDir = "EnginX_Mantid";
  Poco::Path dir;

  try {
// takes to the root of directory according to the platform
#ifdef _WIN32
    const std::string ROOT_DRIVE = "C:/";
    dir.assign(ROOT_DRIVE);
#else
    dir = Poco::Path().home();
#endif
    dir.append(rootDir);

    Poco::File dirFile(dir);
    if (!dirFile.exists()) {
      dirFile.createDirectories();
      g_log.notice() << "Creating output directory root for the first time: "
                     << dir.toString() << '\n';
    }

  } catch (Poco::FileAccessDeniedException &e) {
    g_log.error() << "Error, access/permission denied for root directory: "
                  << dir.toString()
                  << ". This is a severe error. The interface will not behave "
                     "correctly when generating files. Error details: "
                  << e.what() << '\n';
  } catch (std::runtime_error &re) {
    g_log.error() << "Error while finding/creating the root directory: "
                  << dir.toString()
                  << ". This is a severe error. Details: " << re.what() << '\n';
  }

  return dir;
}

/*
 * Provides a small wrapper function that appends the given string
 * to the given path in an OS independent manner and returns the
 * resulting path as a string.
 *
 * @param currentPath The path to be appended to
 * @param toAppend The string to append to the path (note '/' or '\\'
 * characters should not be included
 *
 * @return String with the two parts of the path appended
 */
std::string
EnggDiffractionPresenter::appendToPath(const std::string &currentPath,
                                       const std::string &toAppend) const {
  // Uses poco to handle to operation to ensure OS independence
  Poco::Path newPath(currentPath);
  newPath.append(toAppend);
  return newPath.toString();
}

/**
 * Copy files to the general directories. Normally files are produced
 * in the user/RB number specific directories and then can be copied
 * to the general/all directories using this method.
 *
 * @param source path to the file to copy
 *
 * @param pathComp path component to use for the copy file in the
 * general directories, for example "Calibration" or "Focus"
 */
void EnggDiffractionPresenter::copyToGeneral(const Poco::Path &source,
                                             const std::string &pathComp) {
  Poco::File file(source);
  if (!file.exists() || !file.canRead()) {
    g_log.warning() << "Cannot copy the file " << source.toString()
                    << " to the general/all users directories because it "
                       "cannot be read.\n";
    return;
  }

  auto destDir = outFilesGeneralDir(pathComp);
  try {
    Poco::File destDirFile(destDir);
    if (!destDirFile.exists()) {
      destDirFile.createDirectories();
    }
  } catch (std::runtime_error &rexc) {
    g_log.error() << "Could not create output directory for the general/all "
                     "files. Cannot copy the user files there:  "
                  << destDir.toString() << ". Error details: " << rexc.what()
                  << '\n';

    return;
  }

  try {
    file.copyTo(destDir.toString());
  } catch (std::runtime_error &rexc) {
    g_log.error() << " Could not copy the file '" << file.path() << "' to "
                  << destDir.toString() << ". Error details: " << rexc.what()
                  << '\n';
  }

  g_log.information() << "Copied file '" << source.toString()
                      << "'to general/all directory: " << destDir.toString()
                      << '\n';
}

/**
 * Copy files to the user/RB number directories.
 *
 * @param source path to the file to copy
 *
 * @param pathComp path component to use for the copy file in the
 * general directories, for example "Calibration" or "Focus"
 */
void EnggDiffractionPresenter::copyToUser(const Poco::Path &source,
                                          const std::string &pathComp) {
  Poco::File file(source);
  if (!file.exists() || !file.canRead()) {
    g_log.warning() << "Cannot copy the file " << source.toString()
                    << " to the user directories because it cannot be read.\n";
    return;
  }

  auto destDir = outFilesUserDir(pathComp);
  try {
    Poco::File destDirFile(destDir);
    if (!destDirFile.exists()) {
      destDirFile.createDirectories();
    }
  } catch (std::runtime_error &rexc) {
    g_log.error() << "Could not create output directory for the user "
                     "files. Cannot copy the user files there:  "
                  << destDir.toString() << ". Error details: " << rexc.what()
                  << '\n';

    return;
  }

  try {
    file.copyTo(destDir.toString());
  } catch (std::runtime_error &rexc) {
    g_log.error() << " Could not copy the file '" << file.path() << "' to "
                  << destDir.toString() << ". Error details: " << rexc.what()
                  << '\n';
  }

  g_log.information() << "Copied file '" << source.toString()
                      << "'to user directory: " << destDir.toString() << '\n';
}

/**
 * Copies a file from a third location to the standard user/RB number
 * and the general/all directories. This just uses copyToUser() and
 * copyToGeneral().
 *
 * @param fullFilename full path to the origin file
 */
void EnggDiffractionPresenter::copyFocusedToUserAndAll(
    const std::string &fullFilename) {
  // The files are saved by SaveNexus in the Settings/Focusing output folder.
  // Then they need to go to the user and 'all' directories.
  // The "Settings/Focusing output folder" may go away in the future
  Poco::Path nxsPath(fullFilename);
  const std::string focusingComp = "Focus";
  auto saveDir = outFilesUserDir(focusingComp);
  Poco::Path outFullPath(saveDir);
  outFullPath.append(nxsPath.getFileName());
  copyToUser(nxsPath, focusingComp);
  copyToGeneral(nxsPath, focusingComp);
}

/**
 * To write the calibration/instrument parameter for GSAS.
 *
 * @param outFilename name of the output .par/.prm/.iparm file for GSAS
 * @param difc list of GSAS DIFC values to include in the file
 * @param tzero list of GSAS TZERO values to include in the file
 * @param bankNames list of bank names corresponding the the difc/tzero
 *
 * @param ceriaNo ceria/calibration run number, to be replaced in the
 * template file
 *
 * @param vanNo vanadium run number, to be replaced in the template file
 *
 * @param templateFile a template file where to replace the difc/zero
 * values. An empty default implies using an "all-banks" template.
 */
void EnggDiffractionPresenter::writeOutCalibFile(
    const std::string &outFilename, const std::vector<double> &difc,
    const std::vector<double> &tzero, const std::vector<std::string> &bankNames,
    const std::string &ceriaNo, const std::string &vanNo,
    const std::string &templateFile) {
  // TODO: this is horrible and should be changed to avoid running
  // Python code. Update this as soon as we have a more stable way of
  // generating IPARM/PRM files.

  // Writes a file doing this:
  // write_ENGINX_GSAS_iparam_file(output_file, difc, zero, ceria_run=241391,
  // vanadium_run=236516, template_file=None):

  // this replace is to prevent issues with network drives on windows:
  const std::string safeOutFname =
      boost::replace_all_copy(outFilename, "\\", "/");
  std::string pyCode = "import EnggUtils\n";
  pyCode += "import os\n";
  // normalize apparently not needed after the replace, but to be double-safe:
  pyCode += "GSAS_iparm_fname = os.path.normpath('" + safeOutFname + "')\n";
  pyCode += "bank_names = []\n";
  pyCode += "ceria_number = \"" + ceriaNo + "\"\n";
  pyCode += "van_number = \"" + vanNo + "\"\n";
  pyCode += "Difcs = []\n";
  pyCode += "Zeros = []\n";
  std::string templateFileVal = "None";
  if (!templateFile.empty()) {
    templateFileVal = "'" + templateFile + "'";
  }
  pyCode += "template_file = " + templateFileVal + "\n";
  for (size_t i = 0; i < difc.size(); ++i) {
    pyCode += "bank_names.append('" + bankNames[i] + "')\n";
    pyCode +=
        "Difcs.append(" + boost::lexical_cast<std::string>(difc[i]) + ")\n";
    pyCode +=
        "Zeros.append(" + boost::lexical_cast<std::string>(tzero[i]) + ")\n";
  }
  pyCode +=
      "EnggUtils.write_ENGINX_GSAS_iparam_file(output_file=GSAS_iparm_fname, "
      "bank_names=bank_names, difc=Difcs, tzero=Zeros, ceria_run=ceria_number, "
      "vanadium_run=van_number, template_file=template_file) \n";

  const auto status = m_view->enggRunPythonCode(pyCode);
  g_log.information() << "Saved output calibration file via Python. Status: "
                      << status << '\n';
}

/**
 * Note down a directory that needs to be added to the data search
 * path when looking for run files. This simply uses a vector and adds
 * all the paths, as the ConfigService will take care of duplicates,
 * invalid directories, etc.
 *
 * @param filename (full) path to a file
 */
void EnggDiffractionPresenter::recordPathBrowsedTo(
    const std::string &filename) {

  Poco::File file(filename);
  if (!file.exists() || !file.isFile())
    return;

  Poco::Path path(filename);
  Poco::File directory(path.parent());
  if (!directory.exists() || !directory.isDirectory())
    return;

  m_browsedToPaths.push_back(directory.path());
}

} // namespace CustomInterfaces
} // namespace MantidQt
