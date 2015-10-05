#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtAPI/AlgorithmRunner.h"
#include "MantidQtAPI/PythonRunner.h"
// #include "MantidQtCustomInterfaces/EnggDiffraction/EnggDiffractionModel.h"
#include "MantidQtCustomInterfaces/EnggDiffraction/EnggDiffractionPresenter.h"
#include "MantidQtCustomInterfaces/EnggDiffraction/EnggDiffractionPresWorker.h"
#include "MantidQtCustomInterfaces/EnggDiffraction/IEnggDiffractionView.h"

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

const std::string EnggDiffractionPresenter::g_enginxStr = "ENGINX";
// discouraged at the moment
const bool EnggDiffractionPresenter::g_askUserCalibFilename = false;
const std::string EnggDiffractionPresenter::g_vanIntegrationWSName =
    "engggui_vanadium_integration_ws";

EnggDiffractionPresenter::EnggDiffractionPresenter(IEnggDiffractionView *view)
    : m_workerThread(NULL), m_calibFinishedOK(false), m_focusFinishedOK(false),
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
                        "it down immediately..." << std::endl;
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

  case IEnggDiffractionPresenter::LogMsg:
    processLogMsg();
    break;

  case IEnggDiffractionPresenter::InstrumentChange:
    processInstChange();
    break;

  case IEnggDiffractionPresenter::ShutDown:
    processShutDown();
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
  const std::string vanNo = m_view->newVanadiumNo();
  const std::string ceriaNo = m_view->newCeriaNo();
  try {
    inputChecksBeforeCalibrate(vanNo, ceriaNo);
  } catch (std::invalid_argument &ia) {
    m_view->userWarning("Error in the inputs required for calibrate",
                        ia.what());
    return;
  }
  g_log.notice() << "EnggDiffraction GUI: starting new calibration. This may "
                    "take a few seconds... " << std::endl;

  const std::string outFilename = outputCalibFilename(vanNo, ceriaNo);

  m_view->enableCalibrateAndFocusActions(false);
  // alternatively, this would be GUI-blocking:
  // doNewCalibration(outFilename, vanNo, ceriaNo);
  // calibrationFinished()
  startAsyncCalibWorker(outFilename, vanNo, ceriaNo);
}

void EnggDiffractionPresenter::processFocusBasic() {
  const std::string runNo = m_view->focusingRunNo();
  const std::vector<bool> banks = m_view->focusingBanks();

  try {
    inputChecksBeforeFocusBasic(runNo, banks);
  } catch (std::invalid_argument &ia) {
    m_view->userWarning("Error in the inputs required to focus a run",
                        ia.what());
    return;
  }

  startFocusing(runNo, banks, "", "");
}

void EnggDiffractionPresenter::processFocusCropped() {
  const std::string runNo = m_view->focusingCroppedRunNo();
  const std::vector<bool> banks = m_view->focusingBanks();
  const std::string specNos = m_view->focusingCroppedSpectrumIDs();

  try {
    inputChecksBeforeFocusCropped(runNo, banks, specNos);
  } catch (std::invalid_argument &ia) {
    m_view->userWarning(
        "Error in the inputs required to focus a run (in cropped mode)",
        ia.what());
    return;
  }

  startFocusing(runNo, banks, specNos, "");
}

void EnggDiffractionPresenter::processFocusTexture() {
  const std::string runNo = m_view->focusingTextureRunNo();
  const std::string dgFile = m_view->focusingTextureGroupingFile();

  try {
    inputChecksBeforeFocusTexture(runNo, dgFile);
  } catch (std::invalid_argument &ia) {
    m_view->userWarning(
        "Error in the inputs required to focus a run (in texture mode)",
        ia.what());
    return;
  }

  startFocusing(runNo, std::vector<bool>(), "", dgFile);
}

/**
 * Starts a focusing worker, for different modes depending on the
 * inputs provided. Assumes that the inputs have been checked by the
 * respective specific processFocus methods (for normal, cropped,
 * texture, etc. focusing).
 *
 * @param runNo run/file number to focus
 * @param banks banks to include in the focusing, processed one at a time
 *
 * @param specNos list of spectra to use when focusing. If not empty
 * this implies focusing in cropped mode.
 *
 * @param dgFile detector grouping file to define banks (texture). If
 * not empty, this implies focusing in texture mode.
 */
void EnggDiffractionPresenter::startFocusing(const std::string &runNo,
                                             const std::vector<bool> &banks,
                                             const std::string &specNos,
                                             const std::string &dgFile) {

  std::string optMsg = "";
  if (!specNos.empty()) {
    optMsg = " (cropped)";
  } else if (!dgFile.empty()) {
    optMsg = " (texture)";
  }
  g_log.notice() << "EnggDiffraction GUI: starting new focusing" << optMsg
                 << ". This may take some seconds... " << std::endl;

  const std::string focusDir = m_view->focusingDir();
  const std::vector<std::string> outFilenames =
      outputFocusFilenames(runNo, banks);

  m_view->enableCalibrateAndFocusActions(false);
  // GUI-blocking alternative:
  // doFocusRun(focusDir, outFilenames, runNo, banks, specNos, dgFile)
  // focusingFinished()
  startAsyncFocusWorker(focusDir, outFilenames, runNo, banks, specNos, dgFile);
}

void EnggDiffractionPresenter::processResetFocus() { m_view->resetFocus(); }

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

void EnggDiffractionPresenter::processShutDown() {
  m_view->saveSettings();
  cleanup();
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
    throw std::invalid_argument(
        "The Vanadium number cannot be empty and must be an integer number.");
  }
  if (newCeriaNo.empty()) {
    throw std::invalid_argument(
        "The Ceria number cannot be empty and must be an integer number.");
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
 */
void
EnggDiffractionPresenter::startAsyncCalibWorker(const std::string &outFilename,
                                                const std::string &vanNo,
                                                const std::string &ceriaNo) {
  delete m_workerThread;
  m_workerThread = new QThread(this);
  EnggDiffWorker *worker =
      new EnggDiffWorker(this, outFilename, vanNo, ceriaNo);
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
 */
void EnggDiffractionPresenter::doNewCalibration(const std::string &outFilename,
                                                const std::string &vanNo,
                                                const std::string &ceriaNo) {
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
    doCalib(cs, vanNo, ceriaNo, outFilename);
    m_calibFinishedOK = true;
  } catch (std::runtime_error &) {
    g_log.error() << "The calibration calculations failed. One of the "
                     "algorithms did not execute correctly. See log messages "
                     "for details. " << std::endl;
  } catch (std::invalid_argument &) {
    g_log.error()
        << "The calibration calculations failed. Some input properties "
           "were not valid. See log messages for details. " << std::endl;
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
    g_log.warning() << "The cablibration did not finished correctly."
                    << std::endl;
  } else {
    const std::string vanNo = m_view->newVanadiumNo();
    const std::string ceriaNo = m_view->newCeriaNo();
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
  if ("ENGIN-X" != m_view->currentInstrument()) {
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
 */
void EnggDiffractionPresenter::doCalib(const EnggDiffCalibSettings &cs,
                                       const std::string &vanNo,
                                       const std::string &ceriaNo,
                                       const std::string &outFilename) {
  ITableWorkspace_sptr vanIntegWS;
  MatrixWorkspace_sptr vanCurvesWS;
  MatrixWorkspace_sptr ceriaWS;

  loadOrCalcVanadiumWorkspaces(vanNo, cs.m_inputDirCalib, vanIntegWS,
                               vanCurvesWS, cs.m_forceRecalcOverwrite);

  const std::string instStr = m_view->currentInstrument();
  try {
    auto load = Algorithm::fromString("Load");
    load->initialize();
    load->setPropertyValue("Filename", instStr + ceriaNo);
    std::string ceriaWSName = "engggui_calibration_sample_ws";
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
  const size_t numBanks = 2;
  std::vector<double> difc, tzero;
  difc.resize(numBanks);
  tzero.resize(numBanks);
  for (size_t i = 0; i < difc.size(); i++) {
    auto alg = Algorithm::fromString("EnggCalibrate");
    try {
      alg->initialize();
      alg->setProperty("InputWorkspace", ceriaWS);
      alg->setProperty("VanIntegrationWorkspace", vanIntegWS);
      alg->setProperty("VanCurvesWorkspace", vanCurvesWS);
      alg->setPropertyValue("Bank", boost::lexical_cast<std::string>(i + 1));
      // TODO: figure out what should be done about the list of expected peaks
      // to EnggCalibrate => it should be a default, as in EnggFitPeaks, that
      // should be fixed in a nother ticket/issue
      alg->setPropertyValue(
          "ExpectedPeaks",
          "3.1243, 2.7057, 1.9132, 1.6316, 1.5621, "
          "1.3529, 1.2415, 1.2100, 1.1046, 1.0414, 0.9566, 0.9147, 0.9019, "
          "0.8556, 0.8252, 0.8158, 0.7811");
      alg->setPropertyValue("OutputParametersTableName",
                            "engggui_calibration_bank_" +
                                boost::lexical_cast<std::string>(i + 1));
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

  // Double horror: 1st use a python script
  // 2nd: because runPythonCode does this by emitting a signal that goes to
  // MantidPlot,
  // it has to be done in the view (which is a UserSubWindow).
  Poco::Path outFullPath(cs.m_inputDirCalib);
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
 * @param runNo run number to focus
 * @param banks which banks to consider in the focusing
 *
 * @throws std::invalid_argument with an informative message.
 */
void EnggDiffractionPresenter::inputChecksBeforeFocusBasic(
    const std::string &runNo, const std::vector<bool> &banks) {
  if (runNo.empty()) {
    const std::string msg = "The sample run number to focus cannot be "
                            "empty and must be an integer number.";
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
 * @param runNo run number to focus
 * @param banks which banks to consider in the focusing
 * @param specNos list of spectra (as usual csv list of spectra in Mantid)
 *
 * @throws std::invalid_argument with an informative message.
 */
void EnggDiffractionPresenter::inputChecksBeforeFocusCropped(
    const std::string &runNo, const std::vector<bool> &banks,
    const std::string &specNos) {
  if (runNo.empty()) {
    throw std::invalid_argument("To focus cropped the sample run number cannot "
                                "be empty and must be an integer number.");
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
 * @param runNo run number to focus
 * @param dgFile file with detector grouping info
 *
 * @throws std::invalid_argument with an informative message.
 */
void EnggDiffractionPresenter::inputChecksBeforeFocusTexture(
    const std::string &runNo, const std::string &dgFile) {
  if (runNo.empty()) {
    throw std::invalid_argument("To focus texture banks the sample run number "
                                "cannot be empty and must be an integer "
                                "number.");
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

void
EnggDiffractionPresenter::inputChecksBanks(const std::vector<bool> &banks) {
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
  for (size_t b = 1; b <= banks.size(); b++) {
    res.push_back(instStr + "_" + runNo + "_focused_bank_" +
                  boost::lexical_cast<std::string>(b) + ".nxs");
  }

  return res;
}

std::string
EnggDiffractionPresenter::outputFocusCroppedFilename(const std::string &runNo) {
  const std::string instStr = m_view->currentInstrument();

  return instStr + "_" + runNo + "_focused_cropped.nxs";
}

std::vector<std::string> EnggDiffractionPresenter::outputFocusTextureFilenames(
    const std::string &runNo, const std::vector<size_t> &bankIDs) {
  const std::string instStr = m_view->currentInstrument();

  std::vector<std::string> res;
  for (size_t b = 0; b < bankIDs.size(); b++) {
    res.push_back(instStr + "_" + runNo + "_focused_texture_bank_" +
                  boost::lexical_cast<std::string>(bankIDs[b]) + ".nxs");
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
 * @param outFilenames full names for the output focused runs
 * @param runNo input run number
 * @param banks instrument bank to focus
 * @param specNos list of spectra (as usual csv list of spectra in Mantid)
 * @param dgFile detector grouping file name
 */
void EnggDiffractionPresenter::startAsyncFocusWorker(
    const std::string &dir, const std::vector<std::string> &outFilenames,
    const std::string &runNo, const std::vector<bool> &banks,
    const std::string &specNos, const std::string &dgFile) {

  delete m_workerThread;
  m_workerThread = new QThread(this);
  EnggDiffWorker *worker = new EnggDiffWorker(this, dir, outFilenames, runNo,
                                              banks, specNos, dgFile);
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
 * @param outFilenames names for the output focused files (one per bank)
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
void EnggDiffractionPresenter::doFocusRun(
    const std::string &dir, const std::vector<std::string> &outFilenames,
    const std::string &runNo, const std::vector<bool> &banks,
    const std::string &specNos, const std::string &dgFile) {

  g_log.notice() << "Generating new focusing workspace(s) and file(s) into "
                    "this directory: " << dir << std::endl;

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

  // Prepare special inputs for "texture" focusing
  std::vector<size_t> bankIDs;
  std::vector<std::string> effectiveFilenames;
  std::vector<std::string> specs;
  if (!specNos.empty()) {
    // just to iterate once, but there's no real bank here
    bankIDs.push_back(0);
    specs.push_back(specNos); // one spectrum IDs list given by the user
    effectiveFilenames.push_back(outputFocusCroppedFilename(runNo));
  } else {
    if (dgFile.empty()) {
      for (size_t bidx = 0; bidx < banks.size(); bidx++) {
        if (banks[bidx]) {
          bankIDs.push_back(bidx + 1);
          specs.push_back("");
          effectiveFilenames.push_back(outFilenames[bidx]);
        }
      }
    } else {
      try {
        loadDetectorGroupingCSV(dgFile, bankIDs, specs);
      } catch (std::runtime_error &re) {
        g_log.error() << "Error loading detector grouping file: " + dgFile +
                             ". Detailed error: " + re.what() << std::endl;
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
                          ") for run " + runNo +
                          " into: " << effectiveFilenames[idx] << std::endl;
    try {
      doFocusing(cs, fullFilename, runNo, bankIDs[idx], specs[idx], dgFile);
      m_focusFinishedOK = true;
    } catch (std::runtime_error &) {
      g_log.error()
          << "The focusing calculations failed. One of the algorithms"
             "did not execute correctly. See log messages for details."
          << std::endl;
    } catch (std::invalid_argument &ia) {
      g_log.error()
          << "The focusing failed. Some input properties were not valid. "
             "See log messages for details. Error: " << ia.what() << std::endl;
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
    g_log.warning() << "The cablibration did not finished correctly."
                    << std::endl;
  } else {
    g_log.notice() << "Focusing finished - focused run(s) are ready."
                   << std::endl;
  }
  if (m_workerThread) {
    delete m_workerThread;
    m_workerThread = NULL;
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
 * @param specNos string specifying a list of spectra (for cropped
 * focusing), only considered if not empty
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
  try {
    auto load = Algorithm::fromString("Load");
    load->initialize();
    load->setPropertyValue("Filename", instStr + runNo);
    load->setPropertyValue("OutputWorkspace", inWSName);
    load->execute();

    AnalysisDataServiceImpl &ADS = Mantid::API::AnalysisDataService::Instance();
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

  std::string outWSName;
  if (!dgFile.empty()) {
    outWSName = "engggui_focusing_output_ws_texture_bank_" +
                boost::lexical_cast<std::string>(bank);
  } else if (specNos.empty()) {
    outWSName = "engggui_focusing_output_ws_bank_" +
                boost::lexical_cast<std::string>(bank);
  } else {
    outWSName = "engggui_focusing_output_ws_cropped";
  }

  try {
    auto alg = Algorithm::fromString("EnggFocus");
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

    const bool plotFocusedWS = m_view->focusedOutWorkspace();
    if (plotFocusedWS == true) {
      m_view->plotFocusedSpectrum(outWSName);
    }

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
    auto alg = Algorithm::fromString("SaveNexus");
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
                           std::string(ia.what()) << std::endl;
      throw;
    } catch (std::runtime_error &re) {
      g_log.error() << "Failed to calculate Vanadium corrections. "
                       "There was an error while executing one of the "
                       "algorithms used to perform Vanadium corrections. "
                       "There was no obvious error in the input properties "
                       "but the algorithm failed. Please check the log "
                       "messages for details." +
                           std::string(re.what()) << std::endl;
      throw;
    }
  } else {
    g_log.notice()
        << "Found precalculated Vanadium correction features for Vanadium run "
        << vanNo << ". Re-using these files: " << preIntegFilename << ", and "
        << preCurvesFilename << std::endl;
    try {
      loadVanadiumPrecalcWorkspaces(preIntegFilename, preCurvesFilename,
                                    vanIntegWS, vanCurvesWS);
    } catch (std::invalid_argument &ia) {
      g_log.error() << "Error while loading precalculated Vanadium corrections",
          "The files with precalculated Vanadium corection features (spectra "
          "integration and per-bank curves) were found (with names '" +
              preIntegFilename + "' and '" + preCurvesFilename +
              "', respectively, but there was a problem with the inputs to the "
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
              "delete those files or force recalculations (in settings). Error "
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
 */
void EnggDiffractionPresenter::loadVanadiumPrecalcWorkspaces(
    const std::string &preIntegFilename, const std::string &preCurvesFilename,
    ITableWorkspace_sptr &vanIntegWS, MatrixWorkspace_sptr &vanCurvesWS) {
  AnalysisDataServiceImpl &ADS = Mantid::API::AnalysisDataService::Instance();

  auto alg = Algorithm::fromString("LoadNexus");
  alg->initialize();
  alg->setPropertyValue("Filename", preIntegFilename);
  std::string integWSName = g_vanIntegrationWSName;
  alg->setPropertyValue("OutputWorkspace", integWSName);
  alg->execute();
  // alg->getProperty("OutputWorkspace");
  vanIntegWS = ADS.retrieveWS<ITableWorkspace>(integWSName);

  auto algCurves = Algorithm::fromString("LoadNexus");
  algCurves->initialize();
  algCurves->setPropertyValue("Filename", preCurvesFilename);
  std::string curvesWSName = "engggui_vanadium_curves_ws";
  algCurves->setPropertyValue("OutputWorkspace", curvesWSName);
  algCurves->execute();
  // algCurves->getProperty("OutputWorkspace");
  vanCurvesWS = ADS.retrieveWS<MatrixWorkspace>(curvesWSName);
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

  auto load = Algorithm::fromString("Load");
  load->initialize();
  load->setPropertyValue("Filename",
                         vanNo); // TODO more specific build Vanadium filename
  std::string vanWSName = "engggui_vanadium_ws";
  load->setPropertyValue("OutputWorkspace", vanWSName);
  load->execute();
  AnalysisDataServiceImpl &ADS = Mantid::API::AnalysisDataService::Instance();
  MatrixWorkspace_sptr vanWS = ADS.retrieveWS<MatrixWorkspace>(vanWSName);
  // TODO?: maybe use setChild() and then load->getProperty("OutputWorkspace");

  auto alg = Algorithm::fromString("EnggVanadiumCorrections");
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

} // namespace CustomInterfaces
} // namespace MantidQt
