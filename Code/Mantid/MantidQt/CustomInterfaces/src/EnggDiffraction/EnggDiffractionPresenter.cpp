#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtAPI/AlgorithmRunner.h"
#include "MantidQtAPI/PythonRunner.h"
// #include "MantidQtCustomInterfaces/EnggDiffraction/EnggDiffractionModel.h"
#include "MantidQtCustomInterfaces/EnggDiffraction/EnggDiffractionPresenter.h"
#include "MantidQtCustomInterfaces/EnggDiffraction/IEnggDiffractionView.h"

#include <boost/lexical_cast.hpp>

#include <Poco/File.h>
#include <Poco/Path.h>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;

namespace MantidQt {
namespace CustomInterfaces {

namespace {
Mantid::Kernel::Logger g_log("EngineeringDiffractionGUI");
}

const std::string EnggDiffractionPresenter::g_enginxStr = "ENGINX";

EnggDiffractionPresenter::EnggDiffractionPresenter(IEnggDiffractionView *view)
    : m_view(view) /*, m_model(new EnggDiffractionModel()), */ {
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

  case IEnggDiffractionPresenter::LogMsg:
    processLogMsg();
    break;

  case IEnggDiffractionPresenter::InstrumentChange:
    processInstChange();

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
  parseCalibrateFilename(fname, instName, vanNo, ceriaNo);

  m_view->newCalibLoaded(vanNo, ceriaNo, fname);
}

void EnggDiffractionPresenter::processCalcCalib() {
  EnggDiffCalibSettings cs = m_view->currentCalibSettings();

  std::string vanNo = m_view->newVanadiumNo();
  if (vanNo.empty()) {
    m_view->userWarning(
        "Error in the inputs",
        "The Vanadium number cannot be empty and must be an integer number");
    return;
  }
  std::string ceriaNo = m_view->newCeriaNo();
  if (ceriaNo.empty()) {
    m_view->userWarning(
        "Error in the inputs",
        "The Ceria number cannot be empty and must be an integer number");
    return;
  }

  std::string sugg = buildCalibrateSuggestedFilename(vanNo, ceriaNo);

  std::string outFilename = m_view->askNewCalibrationFilename(sugg);
  if (outFilename.empty()) {
    return;
  }

  try {
    doCalib(cs, vanNo, ceriaNo, outFilename);
    m_view->newCalibLoaded(vanNo, ceriaNo, outFilename);
  } catch (std::runtime_error &re) {
    g_log.error() << "The calibration calculations failed. One of the "
                     "algorithms did not execute correctly. See log messages "
                     "for details. " << std::endl;
  } catch (std::invalid_argument &re) {
    g_log.error()
        << "The calibration calculations failed. Some input properties "
           "were not valid. See log messages for details. " << std::endl;
  }
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

void EnggDiffractionPresenter::processShutDown() {
  m_view->saveSettings();
  cleanup();
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

  try {
    loadOrCalcVanadiumWorkspaces(vanNo, cs.m_inputDirCalib, vanIntegWS,
                                 vanCurvesWS, cs.m_forceRecalcOverwrite);
  } catch (std::runtime_error &re) {
    g_log.error() << "Unable to load or calculate Vanadium corrections. Giving "
                     "up. There was a problem while executing algorithms: " +
                         std::string(re.what());
    throw;
  } catch (std::invalid_argument &ia) {
    g_log.error() << "Unable to load or calculate Vanadium corrections. Giving "
                     "up. There was a problem with the inputs to the "
                     "correction algorithms: " +
                         std::string(ia.what());
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
      auto load = Algorithm::fromString("Load");
      load->initialize();
      load->setPropertyValue(
          "Filename",
          ceriaNo); // TODO more specific build Ceria filename
      std::string ceriaWSName = "engggui_calibration_ceria_ws";
      load->setPropertyValue("OutputWorkspace", ceriaWSName);
      load->execute();
      AnalysisDataServiceImpl &ADS =
          Mantid::API::AnalysisDataService::Instance();
      MatrixWorkspace_sptr ceriaWS =
          ADS.retrieveWS<MatrixWorkspace>(ceriaWSName);

      alg->initialize();
      alg->setProperty("InputWorkspace", ceriaWS);
      alg->setProperty("VanIntegrationWorkspace", vanIntegWS);
      alg->setProperty("VanCurvesWorkspace", vanCurvesWS);
      alg->setPropertyValue("Bank", boost::lexical_cast<std::string>(i + 1));
      // TODO: figure out what should be done about the list of expected peaks
      // to
      // EnggCalibrate
      alg->setPropertyValue("ExpectedPeaks", "0.9566, 2.7057");
      alg->execute();
    } catch (std::runtime_error &re) {
      m_view->userError("Error in calibration",
                        "Could not run EnggCalibrate succesfully for bank " +
                            boost::lexical_cast<std::string>(i) +
                            ". Error description: " + re.what() +
                            " Please check also the log messages for details.");
      g_log.information() << "Could not write calibration file because of the "
                             "errors (see log). " << std::endl;
      throw;
    }
    difc[i] = alg->getProperty("Difc");
    tzero[i] = alg->getProperty("Zero");
  }

  // From EnggFitPeaks:
  //         defaultPeaks = [3.1243, 2.7057, 1.9132, 1.6316, 1.5621, 1.3529,
  //         1.2415,
  //                      1.2100, 1.1046, 1.0414, 0.9566, 0.9147, 0.9019,
  //                      0.8556,
  //                      0.8252, 0.8158, 0.7811]

  // TODO: this is horrible and should not last much here.
  // Avoid running Python code
  // Update this as soon as we have a more stable way of generating IPARM files
  // Writes a file doing this:
  // write_ENGINX_GSAS_iparam_file(output_file, difc, zero, ceria_run=241391,
  // vanadium_run=236516, template_file=None):

  std::string pyCode = "import EnggUtils\n";
  pyCode += "GSAS_iparm_fname= '" + outFilename + "'\n";
  pyCode += "Difcs = []\n";
  pyCode += "Zeros = []\n";
  for (size_t i = 0; i < difc.size(); i++) {
    pyCode +=
        "Difcs.append(" + boost::lexical_cast<std::string>(difc[i]) + ")\n";
    pyCode +=
        "Zeros.append(" + boost::lexical_cast<std::string>(tzero[i]) + ")\n";
  }
  pyCode += "EnggUtils.write_ENGINX_GSAS_iparam_file(GSAS_iparm_fname, Difcs, "
            "Zeros) \n";

  MantidQt::API::PythonRunner pyRunner;
  std::string status = pyRunner.runPythonCode(QString::fromStdString(pyCode),
                                              true).toStdString();
  g_log.information()
      << "Saved output calibration file through Python. Status: " << status
      << std::endl;
  g_log.information() << "Calibration file written as " << outFilename
                      << std::endl;
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
 * @return Suggested name for a new calibration file, following
 * ENGIN-X practices
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

  std::vector<std::string> parts;
  boost::split(parts, filename, boost::is_any_of("_"));
  if (parts.size() < 4) {
    return;
  }

  instName = parts[0];
  vanNo = parts[1];
  ceriaNo = parts[2];
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
  if ("ENGIN-X" != m_view->currentInstrument()) {
    instStr = "UNKNOWN_INST";
  }

  // default extension for calibration files
  const std::string calibExt = ".prm";
  std::string sugg =
      instStr + "_" + vanNo + "_" + ceriaNo + "_both_banks" + calibExt;

  return sugg;
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
  std::string integWSName = "engggui_vanadium_integration_ws";
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
  // TODO: maybe use setChild() and then load->getProperty("OutputWorkspace");

  auto alg = Algorithm::fromString("EnggVanadiumCorrections");
  alg->initialize();
  alg->setProperty("VanadiumWorkspace", vanWS);
  std::string integName = "engggui_van_integration_ws";
  alg->setPropertyValue("IntegrationWorkspace", integName);
  std::string curvesName = "engggui_van_curves_ws";
  alg->setPropertyValue("CurvesWorkspace", curvesName);
  alg->execute();

  ADS.remove(vanWSName);

  vanIntegWS = ADS.retrieveWS<ITableWorkspace>(integName);
  vanCurvesWS = ADS.retrieveWS<MatrixWorkspace>(curvesName);
}

/**
 * builds the expected names of the precalculated Vanadium correction
 * files and tells if both files are found, similar to:
 * ENGINX_precalculated_vanadium_run000236516_integration.nxs
 * ENGINX_precalculated_vanadium_run00236516_bank_curves.nxs
 *
 * @param vanNo
 * @param inputDirCalib
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
      m_view->userError("Failed to calculate Vanadium corrections",
                        "There was an error in the execution of the algorithms "
                        "required to calculate Vanadium corrections. Some "
                        "properties passed to the algorithms were invalid. "
                        "This is possibly because some of the settings are not "
                        "consistent. Please check the log messages for "
                        "details. Details: " + std::string(ia.what()));
      throw;
    } catch (std::runtime_error &re) {
      m_view->userError("Failed to calculate Vanadium corrections",
                        "There was an error while executing one of the "
                        "algorithms used to perform Vanadium corrections. "
                        "There was no obvious error in the input properties "
                        "but the algorithm failed. Please check the log "
                        "messages for details." + std::string(re.what()));
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
    } catch (std::runtime_error &) {
      m_view->userError(
          "Error while loading precalculated Vanadium corrections",
          "The files with precalculated Vanadium corection features (spectra "
          "integration and per-bank curves) were found (with names '" +
              preIntegFilename + "' and '" + preCurvesFilename +
              "', respectively, but there was a problem while loading them. "
              "Please check the log messages for details. You might want to "
              "delete those files or force recalculations (in settings).");
      throw;
    }
  }
}

} // namespace CustomInterfaces
} // namespace MantidQt
