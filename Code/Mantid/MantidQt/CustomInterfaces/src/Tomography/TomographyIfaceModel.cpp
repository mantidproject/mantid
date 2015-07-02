#include "MantidKernel/FacilityInfo.h"
#include "MantidQtAPI/AlgorithmRunner.h"
#include "MantidQtCustomInterfaces/Tomography/TomographyIfaceModel.h"

#include <QMutex>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;

namespace MantidQt {
namespace CustomInterfaces {

namespace {
Mantid::Kernel::Logger g_log("TomographyGUI");
}

// names by which we know compute resourcess
const std::string TomographyIfaceModel::g_SCARFName = "SCARF@STFC";

// names by which we know image/tomography reconstruction tools (3rd party)
const std::string TomographyIfaceModel::g_TomoPyTool = "TomoPy";
const std::string TomographyIfaceModel::g_AstraTool = "Astra";
const std::string TomographyIfaceModel::g_CCPiTool = "CCPi CGLS";
const std::string TomographyIfaceModel::g_SavuTool = "Savu";
const std::string TomographyIfaceModel::g_customCmdTool = "Custom command";

/**
 * Default constructor, but note that this currently relies on the
 * SCARF cluster (only in ISIS facility) as the only supported remote
 * compute resource.
 */
TomographyIfaceModel::TomographyIfaceModel()
    : m_loggedInUser(""), m_loggedInComp(""), m_facility("ISIS"),
      m_localCompName("Local"), m_computeRes(), m_computeResStatus(),
      m_reconTools(), m_reconToolsStatus(), m_jobsStatus(), m_SCARFtools(),
      m_toolsSettings(), m_statusMutex(NULL) {

  m_computeRes.push_back(g_SCARFName);
  m_computeRes.push_back(m_localCompName);

  m_SCARFtools.push_back(g_TomoPyTool);
  m_SCARFtools.push_back(g_AstraTool);
  m_SCARFtools.push_back(g_CCPiTool);
  m_SCARFtools.push_back(g_SavuTool);
  m_SCARFtools.push_back(g_customCmdTool);

  m_currentTool = m_SCARFtools.front();

  m_statusMutex = new QMutex();
}

TomographyIfaceModel::~TomographyIfaceModel() {
  if (m_statusMutex)
    delete m_statusMutex;
}

void TomographyIfaceModel::cleanup() {
  const std::string user = loggedIn();
  if (!user.empty()) {
    doLogout(m_loggedInComp, user);
    m_loggedInUser = "";
  }
}

/**
 * Check that the selected compute resource is listed as supported and
 * usable for the remote manager (if it is not local). Local jobs are
 * not supported for the time being, so this currently raises an
 * exception if the local resource has been selected.
 *
 * This should never throw an exception if the
 * construction/initialization and setup steps went fine and the rest
 * of the code is kept consistent with those steps.
 *
 * @param res Name of the compute resource selected in the interface
 *
 * @return Name of a compute resource (which can be the 'Local' one)
 *
 * @throws std::runtime_error on inconsistent selection of compute
 * resource
 */
std::string TomographyIfaceModel::validateCompResource(const std::string &res) {
  if (res == m_localCompName) {
    // Nothing yet
    throw std::runtime_error("There is no support for the local compute "
                             "resource. You should not have got here.");
  }

  if (m_computeRes.size() <= 0) {
    throw std::runtime_error("No compute resource registered in the list "
                             "of supported resources. This graphical interface "
                             "is in an inconsistent status.");
  }

  const std::string supported = m_computeRes.front();
  if (supported.empty()) {
    throw std::runtime_error("The first compute resource registered in this "
                             "interface has an empty name.");
  }

  if (res != supported) {
    throw std::runtime_error("The compute resource selected (" + res +
                             ") is not the one in principle supported by this "
                             "interface: " +
                             supported);
  }

  return supported;
}

/**
 * Sets the compute resource that will be used to run reconstruction
 * or other types of jobs. It checks that the facility and compute
 * resource are fine (the one expected). Otherwise, shows an error
 * and not much can be done.
 */
void TomographyIfaceModel::setupComputeResource() {
  // m_computeRes is initialized in the constructor and doesn't change
  m_computeResStatus.clear();

  const Mantid::Kernel::FacilityInfo &fac =
      Mantid::Kernel::ConfigService::Instance().getFacility();
  if (fac.name() != m_facility) {
    throw std::runtime_error(
        "Failed to initialize because the facility is  " + fac.name() +
        " (and not " + m_facility +
        "). "
        "Facility not supported. This interface is designed "
        "to be used at " +
        m_facility +
        ". You will probably not be able to use it in a useful way "
        "because your facility is " +
        fac.name() +
        ". If you have set that facility by mistake in your settings, "
        "please update it.");
  }

  // this implies a nearly empty / no functionality interface
  if (m_computeRes.size() < 1) {
    return;
  }

  // assume the present reality: just SCARF
  const std::string &required = m_computeRes.front();
  std::vector<std::string> res = Mantid::Kernel::ConfigService::Instance()
                                     .getFacility()
                                     .computeResources();
  if (res.end() == std::find(res.begin(), res.end(), required)) {
    throw std::runtime_error(
        "Required compute resource: '" + required +
        "' not found. "
        "This interface requires the " +
        required + " compute resource. Even though your facility is " +
        fac.name() + ", the compute resource was not found. "
                     "In principle the compute resource should have been "
                     "defined in the facilities file for you facility. "
                     "Please check your settings.");
  }
  m_computeResStatus.push_back(true);

  // put local as second compute resource, but disable, as it's not yet sorted
  // out how it will work
  m_computeResStatus.push_back(false);
}

/**
 * Sets the tools that can be run once we know the compute resource
 * where they're going to run. This is very dependent on how the
 * facility and compute resource is adminstered and what the resource
 * provides.
 *
 * @param compRes compute resource for which the tools have to be set
 * up. If empty, the default resource is assumed
 */
void TomographyIfaceModel::setupRunTool(const std::string &compRes) {
  m_reconToolsStatus.clear();

  // catch all the useable/relevant tools for the compute
  // resources. For the time being this is rather simple (just
  // SCARF) and will probably stay like this for a while.
  if ("ISIS" == m_facility && (compRes.empty() || g_SCARFName == compRes)) {
    m_reconTools = m_SCARFtools;
  } else {
    throw std::runtime_error("Cannot setup this interface for the facility: " +
                             m_facility +
                             ". There is no information about tools for it. ");
  }
  // others would/could come here

  for (size_t i = 0; i < m_reconTools.size(); i++) {
    // put CCPi but disable it, as it's not yet sorted out how it is
    // configured / run
    if (g_CCPiTool == m_reconTools[i] ||
        // also, we cannot run Savu at present, and CCPi is not available
        g_SavuTool == m_reconTools[i]) {
      m_reconToolsStatus.push_back(false);
    } else {
      m_reconToolsStatus.push_back(true);
    }
  }
}

/**
 * Log into remote compute resource.
 *
 * @param compRes Name of the compute resource where to login
 * @param user Username
 * @param pw Password/authentication credentials as a string
 */
void TomographyIfaceModel::doLogin(const std::string &compRes,
                                   const std::string &user,
                                   const std::string &pw) {
  auto alg = Algorithm::fromString("Authenticate");
  alg->initialize();
  alg->setPropertyValue("UserName", user);
  alg->setPropertyValue("ComputeResource", compRes);
  alg->setPropertyValue("Password", pw);
  try {
    alg->execute();
  } catch (std::runtime_error &e) {
    throw std::runtime_error(
        "Error when trying to log into the remote compute resource " + compRes +
        " with username " + user + ": " + e.what());
  }

  m_loggedInUser = user;
  m_loggedInComp = compRes;
}

void TomographyIfaceModel::doLogout(const std::string &compRes,
                                    const std::string &username) {
  auto alg = Algorithm::fromString("Logout");
  alg->initialize();
  alg->setProperty("ComputeResource", compRes);
  alg->setProperty("UserName", username);
  try {
    alg->execute();
  } catch (std::runtime_error &e) {
    throw std::runtime_error(
        "Error when trying to log out from the remote compute resource " +
        compRes + " with username " + username + ": " + e.what());
  }

  m_loggedInUser = "";
}

/**
 * Ping the compute resource / server to check if it's alive and
 * responding.
 *
 * @return True if ping succeeded
 */
bool TomographyIfaceModel::doPing(const std::string &compRes) {
  // This actually does more than a simple ping. Ping and check that a
  // transaction can be created succesfully
  auto alg = Algorithm::fromString("StartRemoteTransaction");
  alg->initialize();
  alg->setProperty("ComputeResource", compRes);
  std::string tid;
  try {
    alg->execute();
    tid = alg->getPropertyValue("TransactionID");
    g_log.information() << "Pinged '" << compRes
                        << "'succesfully. Checked that a transaction could "
                           "be created, with ID: " << tid << std::endl;
  } catch (std::runtime_error &e) {
    throw std::runtime_error("Error. Failed to ping and start a transaction on "
                             "the remote resource." +
                             std::string(e.what()));
  }

  return true;
}

/**
 * Handle the job submission request relies on a submit algorithm.
 */
void
TomographyIfaceModel::doSubmitReconstructionJob(const std::string &compRes) {
  std::string run, opt;
  try {
    makeRunnableWithOptions(compRes, run, opt);
  } catch (std::exception &e) {
    g_log.error() << "Could not prepare the requested reconstruction job "
                     "submission. There was an error: " +
                         std::string(e.what());
    throw;
  }

  // with SCARF we use one (pseudo)-transaction for every submission
  auto transAlg = Algorithm::fromString("StartRemoteTransaction");
  transAlg->initialize();
  transAlg->setProperty("ComputeResource", compRes);
  std::string tid;
  try {
    transAlg->execute();
    tid = transAlg->getPropertyValue("TransactionID");
  } catch (std::runtime_error &e) {
    throw std::runtime_error("Error when trying to start a transaction right "
                             "before submitting a reconstruction job: " +
                             std::string(e.what()));
  }

  auto submitAlg = Algorithm::fromString("SubmitRemoteJob");
  submitAlg->initialize();
  submitAlg->setProperty("ComputeResource", compRes);
  submitAlg->setProperty("TaskName", "Mantid tomographic reconstruction job");
  submitAlg->setProperty("TransactionID", tid);
  submitAlg->setProperty("ScriptName", run);
  submitAlg->setProperty("ScriptParams", opt);
  try {
    submitAlg->execute();
  } catch (std::runtime_error &e) {
    throw std::runtime_error(
        "Error when trying to submit a reconstruction job: " +
        std::string(e.what()));
  }
}

void TomographyIfaceModel::doQueryJobStatus(const std::string &compRes,
                                            std::vector<std::string> &ids,
                                            std::vector<std::string> &names,
                                            std::vector<std::string> &status,
                                            std::vector<std::string> &cmds) {
  auto alg = Algorithm::fromString("QueryAllRemoteJobs");
  alg->initialize();
  alg->setPropertyValue("ComputeResource", compRes);
  try {
    alg->execute();
  } catch (std::runtime_error &e) {
    throw std::runtime_error(
        "Error when trying to query the status of jobs in " + compRes + ": " +
        e.what());
  }
  ids = alg->getProperty("JobId");
  names = alg->getProperty("JobName");
  status = alg->getProperty("JobStatusString");
  cmds = alg->getProperty("CommandLine");
}

void TomographyIfaceModel::doCancelJobs(const std::string &compRes,
                                        const std::vector<std::string> &ids) {
  for (size_t i = 0; i < ids.size(); i++) {
    const std::string id = ids[i];
    auto algJob = Algorithm::fromString("AbortRemoteJob");
    algJob->initialize();
    algJob->setPropertyValue("ComputeResource", compRes);
    algJob->setPropertyValue("JobID", id);
    try {
      algJob->execute();
    } catch (std::runtime_error &e) {
      throw std::runtime_error(
          "Error when trying to cancel a reconstruction job: " +
          std::string(e.what()));
    }
  }
  // doesn't do StopRemoteTransaction. If there are multiple jobs per
  // transaction there could be others that are still running.
}

void TomographyIfaceModel::doRefreshJobsInfo(const std::string &compRes) {
  // get the info from the server into data members. This operation is subject
  // to delays in the connection, etc.
  try {
    getJobStatusInfo(compRes);
  } catch (std::runtime_error &e) {
    g_log.warning() << "There was an issue while trying to retrieve job status "
                       "information from the remote compute resource ("
                    << compRes
                    << "). Stopping periodic (automatic) status update to "
                       "prevent more failures. You can start the automatic "
                       "update mechanism again by logging in, as apparently "
                       "there is some problem with the last session: "
                    << e.what() << std::endl;
  }
}

void TomographyIfaceModel::getJobStatusInfo(const std::string &compRes) {
  if (m_loggedInUser.empty())
    return;

  std::vector<std::string> ids, names, status, cmds;
  doQueryJobStatus(compRes, ids, names, status, cmds);

  size_t jobMax = ids.size();
  if (ids.size() != names.size() || ids.size() != status.size() ||
      ids.size() != cmds.size()) {
    // this should not really happen
    jobMax = std::min(ids.size(), names.size());
    jobMax = std::min(jobMax, status.size());
    jobMax = std::min(jobMax, cmds.size());
    g_log.warning()
        << "Problem retrieving job status information. "
           "The response from the compute resource did not seem "
           "correct. The table of jobs may not be fully up to date.";
  }

  {
    QMutexLocker lockit(m_statusMutex);
    m_jobsStatus.clear();
    // TODO: udate when we update to remote algorithms v2 and more
    // info might become available from SCARF.
    // As SCARF doesn't provide all the info at the moment, the
    // IRemoteJobManager::RemoteJobInfo struct is for now used only
    // partially (cmds out). So this loop feels both incomplete and an
    // unecessary second step that could be avoided.
    for (size_t i = 0; i < ids.size(); ++i) {
      IRemoteJobManager::RemoteJobInfo ji;
      ji.id = ids[i];
      ji.name = names[i];
      ji.status = status[i];
      ji.cmdLine = cmds[i];
      m_jobsStatus.push_back(ji);
    }
  }
}

/**
 * Make sure that the data paths (sample, dark, open beam) make
 * sense. Otherwise, warn the user and log error.
 *
 * @throw std::runtime_error if the required fields are not set
 * properly
 */
void TomographyIfaceModel::checkDataPathsSet() {
  if (!m_pathsConfig.validate()) {
    const std::string detail =
        "Please define the paths to your dataset images. "
        "You have not defined some of the following paths: sample, "
        "dark, or open beam images. "
        "They are all required to run reconstruction jobs. Please "
        "define these paths in the settings of the interface. ";
    throw std::runtime_error(
        "Cannot run any reconstruction job without the "
        "paths to the sample, dark and open beam images. " +
        detail);
  }
}

/**
 * Build the components of the command line to run on the remote
 * compute resource.Produces a (normally full) path to a runnable, and
 * the options (quite like $0 and $* in scripts).
 *
 * @param comp Compute resource for which the command line is being prepared
 * @param run Path to a runnable application (script, python module, etc.)
 * @param opt Command line parameters to the application
 */
void TomographyIfaceModel::makeRunnableWithOptions(const std::string &comp,
                                                   std::string &run,
                                                   std::string &opt) {
  checkDataPathsSet();

  // For now we only know how to 'aproximately' run commands on SCARF
  if (g_SCARFName == comp) {
    const std::string tool = usingTool();
    std::string cmd;

    // TODO this is still incomplete, not all tools ready
    if (tool == g_TomoPyTool) {
      cmd = m_toolsSettings.tomoPy.toCommand();
      // this will make something like:
      // run = "/work/imat/z-tests-fedemp/scripts/tomopy/imat_recon_FBP.py";
      // opt = "--input_dir " + base + currentPathFITS() + " " + "--dark " +
      // base +
      //      currentPathDark() + " " + "--white " + base + currentPathFlat();
    } else if (tool == g_AstraTool) {
      cmd = m_toolsSettings.astra.toCommand();
      // this will produce something like this:
      // run = "/work/imat/scripts/astra/astra-3d-SIRT3D.py";
      // opt = base + currentPathFITS();
    } else if (tool == g_customCmdTool) {
      cmd = m_toolsSettings.custom.toCommand();
    } else {
      throw std::runtime_error(
          "Unable to use this tool. "
          "I do not know how to submit jobs to use this tool: " +
          tool + ". It seems that this interface is "
                 "misconfigured or there has been an unexpected "
                 "failure.");
    }

    splitCmdLine(cmd, run, opt);
    checkWarningToolNotSetup(tool, cmd, run, opt);
  } else {
    run = "error_do_not_know_what_to_do";
    opt = "no_options_known";

    const std::string details =
        "Unrecognized remote compute resource. "
        "The remote compute resource that you are trying not used is "
        "not known: " +
        comp + ". This seems to indicate that this interface is "
               "misconfigured or there has been an unexpected failure.";
    throw std::runtime_error(
        "Could not recognize the remote compute resource: '" + comp + "'. " +
        details);
  }
}

/**
 * This can produce a tool-specific warning that can be shown for
 * different tools when they are not fully setup (any required
 * parameter is missing).
 *
 * @param tool Name of the tool this warning applies to
 * @param settings current settings for the tool
 * @param cmd command/script/executable derived from the settings
 * @param opt options for that command/script/executable derived from the
 * settings
 */
void TomographyIfaceModel::checkWarningToolNotSetup(const std::string &tool,
                                                    const std::string &settings,
                                                    const std::string &cmd,
                                                    const std::string &opt) {
  if (tool.empty() || settings.empty() || cmd.empty() || opt.empty()) {
    const std::string detail =
        "Please define the settings of this tool. "
        "You have not defined any settings for this tool: " +
        tool + ". Before running it you need to define its settings "
               "(parameters). You can do so by clicking on the setup "
               "button.";
    throw std::runtime_error("Cannot run the tool " + tool +
                             " before its settings have been defined." +
                             detail);
  }
}

/**
 * Temporary helper to do an operation that shouldn't be needed any longer when
 * the code is reorganized to use the tool settings objetcs better.
 */
void TomographyIfaceModel::splitCmdLine(const std::string &cmd,
                                        std::string &run, std::string &opts) {
  if (cmd.empty())
    return;

  auto pos = cmd.find(' ');
  if (std::string::npos == pos)
    return;

  run = cmd.substr(0, pos);
  opts = cmd.substr(pos + 1);
}

/**
* Helper to get a FITS image into a workspace. Uses the LoadFITS
* algorithm. If the algorithm throws, this method shows user (pop-up)
* warning/error messages but does not throw.
*
* This method returns a workspace group which most probably you want
* to delete after using the image to draw it.
*
* @param path Path to a FITS image
*
* @return Group Workspace containing a Matrix workspace with a FITS
* image, one pixel per histogram, as loaded by LoadFITS (can be empty
* if the load goes wrong and the workspace is not available from the
* ADS).
*/
WorkspaceGroup_sptr
TomographyIfaceModel::loadFITSImage(const std::string &path) {
  // get fits file into workspace and retrieve it from the ADS
  auto alg = Algorithm::fromString("LoadFITS");
  alg->initialize();
  alg->setPropertyValue("Filename", path);
  std::string wsName = "__fits_ws_imat_tomography_gui";
  alg->setProperty("OutputWorkspace", wsName);
  try {
    alg->execute();
  } catch (std::exception &e) {
    throw std::runtime_error(
        "Failed to load image. Could not load this file as a "
        "FITS image: " +
        std::string(e.what()));
    return WorkspaceGroup_sptr();
  }
  if (!alg->isExecuted()) {
    throw std::runtime_error(
        "Failed to load image correctly. Note that even though "
        "the image file has been loaded it seems to contain errors.");
  }
  WorkspaceGroup_sptr wsg;
  MatrixWorkspace_sptr ws;
  try {
    wsg = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(wsName);
    ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        wsg->getNames()[0]);
  } catch (std::exception &e) {
    throw std::runtime_error(
        "Could not load image contents. An unrecoverable error "
        "happened when trying to load the image contents. Cannot "
        "display it. Error details: " +
        std::string(e.what()));
  }

  // draw image from workspace
  if (wsg && ws &&
      Mantid::API::AnalysisDataService::Instance().doesExist(ws->name())) {
    return wsg;
  } else {
    return WorkspaceGroup_sptr();
  }
}

void TomographyIfaceModel::logMsg(const std::string &msg) {
  g_log.notice() << msg << std::endl;
}

} // namespace CustomInterfaces
} // namespace MantidQt
