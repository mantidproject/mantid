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
}

void EnggDiffractionPresenter::processCalcCalib() {
  EnggDiffCalibSettings cs = m_view->currentCalibSettings();

  std::string vanNo = m_view->newVanadiumNo();
  if (vanNo.empty()) {
    m_view->userWarning(
        "Error in the inputs",
        "The Vanadium number cannot be empty and must be an integer number");
  }
  std::string ceriaNo = m_view->newCeriaNo();
  if (ceriaNo.empty()) {
    m_view->userWarning(
        "Error in the inputs",
        "The Ceria number cannot be empty and must be an integer number");
  }

  std::string sugg = buildCalibrateSuggestedFilename(vanNo, ceriaNo);

  std::string outFilename = m_view->askNewCalibrationFilename(sugg);
  doCalib(cs, vanNo, ceriaNo, outFilename);
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

void EnggDiffractionPresenter::doCalib(const EnggDiffCalibSettings &cs,
                                       const std::string &vanNo,
                                       const std::string &ceriaNo,
                                       const std::string &outFilename) {

  MatrixWorkspace_sptr vanIntegWS;
  MatrixWorkspace_sptr vanCurvesWS;
  MatrixWorkspace_sptr ceriaWS;

  loadOrCalcanadiumWorkspaces(vanNo, cs.m_inputDirCalib, vanIntegWS,
                              vanCurvesWS, cs.m_forceRecalcOverwrite);

  // Bank 1 and 2 - ENGIN-X
  const size_t numBanks = 2;
  std::vector<double> difc, tzero;
  difc.reserve(numBanks);
  tzero.reserve(numBanks);
  for (size_t i = 0; i < difc.size(); i++) {
    auto alg = Algorithm::fromString("EnggCalibrate");
    alg->initialize();
    alg->setProperty("InputWorkspace", ceriaWS);

    try {
      alg->execute();
    } catch (std::runtime_error &re) {
      m_view->userError("Error in calibration",
                        "Could not run EnggCalibrate succesfully for bank " +
                            boost::lexical_cast<std::string>(i) +
                            ". Error description: " + re.what() +
                            " Please check also the log messages for details.");
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
  pyCode += "GSAS_iparm_fname=" + outFilename + "\n";
  pyCode += "Difc = []\n";
  pyCode += "Zero = []\n";
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
                                              false).toStdString();
  g_log.information() << "Written calibration file as " << outFilename
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
  std::string sugg = instStr + "_" + vanNo + "_" + ceriaNo + "_"
                                                             "_both_banks" +
                     "." + calibExt;

  return sugg;
}

/**
 *
 * @param dir directory where the vanadium run should be looked for
 * (normally the input calibration directory chosen in the settings of
 * the GUI
 *
 * @param vanIntegWS output (matrix) workspace loaded from the
 * precalculated Vanadium correction file, with the integration
 * resutls
 *
 * @param vanCurvesWS output (matrix) workspace loaded from the
 * precalculated Vanadium correction file, with the per-bank curves
 */
void EnggDiffractionPresenter::loadVanadiumWorkspaces(
    const std::string &vanNo, const std::string &dir,
    MatrixWorkspace_sptr &vanIntegWS, MatrixWorkspace_sptr &vanCurvesWS) {
  std::string integFullPath = "";
  std::string curvesFullPath = "";

  auto alg = Algorithm::fromString("LoadNexus");
  alg->initialize();
  alg->setPropertyValue("Filename", integFullPath);
  alg->execute();
  vanIntegWS = alg->getProperty("OutputWorkspace");

  auto algCurves = Algorithm::fromString("LoadNexus");
  algCurves->initialize();
  algCurves->setPropertyValue("Filename", curvesFullPath);
  algCurves->execute();
  vanCurvesWS = algCurves->getProperty("OutputWorkspace");
}

void EnggDiffractionPresenter::calcVanadiumWorkspaces(
    const std::string &vanNo, MatrixWorkspace_sptr &vanIntegWS,
    MatrixWorkspace_sptr &vanCurvesWS) {
  MatrixWorkspace_sptr vanWS;
  auto load = Algorithm::fromString("EnggVanadiumCorrections");
  load->initialize();
  load->setProperty("Filename", vanNo); // TODO build Vanadium filename

  auto alg = Algorithm::fromString("EnggVanadiumCorrections");
  alg->initialize();
  alg->setProperty("VanadiumWorkspace", vanWS);
  alg->execute();

  vanIntegWS = alg->getProperty("IntegrationWorkspace");
  vanCurvesWS = alg->getProperty("CurvesWorkspace");
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

  preIntegFilename =
      g_enginxStr + "_precalculated_vanadium_run" + vanNo + "_bank_curves.nxs";

  preCurvesFilename =
      g_enginxStr + "_precalculated_vanadium_run" + vanNo + "_bank_curves.nxs";

  Poco::Path path(inputDirCalib);
  path.append(preIntegFilename);
  Poco::File(path).exists();

  found = true;
}

/**
 *
 * @param vanNo
 */
void EnggDiffractionPresenter::loadOrCalcanadiumWorkspaces(
    const std::string &vanNo, const std::string &inputDirCalib,
    MatrixWorkspace_sptr &vanIntegWS, MatrixWorkspace_sptr &vanCurvesWS,
    bool forceRecalc) {
  bool foundPrecalc = false;

  std::string preIntegFilename, preCurvesFilename;
  findPrecalcVanadiumCorrFilenames(vanNo, inputDirCalib, preIntegFilename,
                                   preCurvesFilename, foundPrecalc);

  if (forceRecalc || !foundPrecalc) {
    g_log.notice()
        << "Calculating Vanadium corrections. This may take a few seconds..."
        << std::endl;
    calcVanadiumWorkspaces(vanNo, vanIntegWS, vanCurvesWS);
  } else {
    g_log.notice()
        << "Found precalculated Vanadium correction features for Vanadium run "
        << vanNo << ". Re-using them." << std::endl;
    loadVanadiumWorkspaces(vanNo, inputDirCalib, vanIntegWS, vanCurvesWS);
  }
}

} // namespace CustomInterfaces
} // namespace MantidQt
